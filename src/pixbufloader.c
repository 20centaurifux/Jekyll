/***************************************************************************
    begin........: February 2011
    copyright....: Sebastian Fedrau
    email........: lord-kefir@arcor.de
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 ***************************************************************************/
/*!
 * \file pixbufloader.c
 * \brief An asynchronous pixbuf loader.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 2. March 2012
 */

#include <gio/gio.h>

#include "pixbufloader.h"
#include "net/uri.h"
#include "net/httpclient.h"

/**
 * @addtogroup Core
 * @{
 */

/*! Defines PixbufLoader. */
G_DEFINE_TYPE(PixbufLoader, pixbuf_loader, G_TYPE_OBJECT);

enum
{
	PROP_0,
	PROP_RUNNING,
	PROP_CACHE_DIR
};

/**
 * \struct _PixbufLoaderPrivate
 * \brief Private _PixbufLoader data.
 */
struct _PixbufLoaderPrivate
{
	/*! Indicates if the loader is running. */
	gboolean running;
	/*! Directory used for caching. */
	gchar *cache_dir;
	/*! Mutex protecing the status. */
	GMutex *mutex_running;
	/*! A mutex protecting the callback collection. */
	GMutex *mutex_callbacks;
	/*! A table containing callbacks. */
	GHashTable *callbacks;
	/*! Background worker. */
	GThread *thread;
	/*! Queue containing urls to fetch. */
	GAsyncQueue *queue;
	/*! A cancellable. */
	GCancellable *cancellable;
};

/**
 * \struct _PixbufLoaderCallbackData
 * \brief Holds callback data.
 */
typedef struct
{
	/*! Id of the callback. */
	guint id;
	/*! The callback data. */
	PixbufLoaderCallback callback;
	/*! User data. */
	gpointer user_data;
	/*! Function to free user data. */
	GFreeFunc free_func;
	/*! Group identifier. */
	GQuark group;
	/*! Url identifier. */
	GQuark url;
} _PixbufLoaderCallbackData;

/*
 *	table functions:
 */
static void
_pixbuf_loader_free_callback_table_value(_PixbufLoaderCallbackData *callback)
{
	if(callback->free_func && callback->user_data)
	{
		callback->free_func(callback->user_data);
	}

	g_slice_free1(sizeof(_PixbufLoaderCallbackData), callback);
}

static GHashTable *
_pixbuf_loader_callback_table_create(void)
{
	return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)_pixbuf_loader_free_callback_table_value);
}

static _PixbufLoaderCallbackData *
_pixbuf_loader_callback_table_insert(GHashTable *table, PixbufLoaderCallback callback, gpointer user_data, GFreeFunc free_func, const gchar *group, const gchar *url)
{
	static guint id = 0;
	_PixbufLoaderCallbackData *item;

	g_assert(callback != NULL);
	g_assert(url != NULL);

	if(id == G_MAXUINT)
	{
		id = 0;
	}

	/* create new item for table */
	item = (_PixbufLoaderCallbackData *)g_slice_alloc(sizeof(_PixbufLoaderCallbackData));
	item->id = ++id;
	item->callback = callback;
	item->user_data = user_data;
	item->free_func = free_func;
	item->group = group ? g_quark_from_string(group) : 0;
	item->url = g_quark_from_string(url);

	/* insert item into table */
	g_hash_table_insert(table, GINT_TO_POINTER(item->id), item);

	return item;
}

static GList *
_pixbuf_loader_callback_table_get_by_url(GHashTable *table, const gchar *url)
{
	GList *keys, *iter;
	GList *list = NULL;
	_PixbufLoaderCallbackData *callback;
	GQuark url_quark = g_quark_from_string(url);

	g_assert(url != NULL);

	keys = iter = g_hash_table_get_keys(table);

	while(iter)
	{
		callback = g_hash_table_lookup(table, iter->data);

		if(callback->url == url_quark)
		{
			list = g_list_append(list, GINT_TO_POINTER(callback->id));
		}

		iter = iter->next;
	}

	g_list_free(keys);

	return list;
}

static gboolean
_pixbuf_loader_callback_table_remove_group(gint id, _PixbufLoaderCallbackData *callback, const gchar *group)
{
	return (callback->group == g_quark_from_string(group)) ? TRUE : FALSE;
}

/*
 *	cache functions:
 */
static gboolean
_pixbuf_loader_prepare_cache_dir(const gchar *path)
{
	gboolean result = FALSE;

	g_assert(path != NULL);

	g_debug("Preparing pixbuf loader cache directory: \"%s\"", path);

	/* check if specified directory does already exist */
	if(!(result = g_file_test(path, G_FILE_TEST_IS_DIR)))
	{
		g_debug("Trying to create image folder: \"%s\"", path);
		if(g_mkdir_with_parents(path, 500))
		{
			g_warning("Couldn't create image folder: \"%s\"", path);
		}
	}

	return result;
}

static GdkPixbuf *
_pixbuf_loader_get_from_cache_dir(const gchar *cache_dir, const gchar *url)
{
	GChecksum *checksum;
	gchar *path;
	GdkPixbuf *pixbuf = NULL;
	GError *err = NULL;

	g_assert(cache_dir != NULL);
	g_assert(url != NULL);

	/* build filename */
	checksum = g_checksum_new(G_CHECKSUM_SHA1);
	g_checksum_update(checksum, (const guchar *)url, -1);
	path = g_build_filename(cache_dir, G_DIR_SEPARATOR_S, g_checksum_get_string(checksum), NULL);

	/* try to load image from cache directory */
	if(g_file_test(path, G_FILE_TEST_IS_REGULAR))
	{
		//g_debug("Loading image from cache: \"%s\"", path);
		pixbuf = gdk_pixbuf_new_from_file_at_size(path, 48, 48, &err);

		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	/* cleanup */
	g_free(path);
	g_checksum_free(checksum);

	return pixbuf;
}

static gboolean
_pixbuf_loader_save_image(const gchar *cache_dir, const gchar *url, const gchar *buffer, gint length)
{
	GChecksum *checksum;
	gchar *path;
	GError *err = NULL;
	gboolean ret = FALSE;

	g_assert(cache_dir != NULL);
	g_assert(url != NULL);
	g_assert(buffer != NULL);
	g_assert(length > 0);

	/* build filename */
	checksum = g_checksum_new(G_CHECKSUM_SHA1);
	g_checksum_update(checksum, (const guchar *)url, -1);
	path = g_build_filename(cache_dir, G_DIR_SEPARATOR_S, g_checksum_get_string(checksum), NULL);

	/* save image */
	g_debug("Saving file: \"%s\" => \"%s\"", url, path);
	if(!(ret = g_file_set_contents(path, buffer, length, &err)))
	{	
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	/* cleanup */
	g_free(path);
	g_checksum_free(checksum);

	return ret;
}

/*
 *	HTTP functions:
 */
static GdkPixbuf *
_pixbuf_loader_get_from_server(const gchar *cache_dir, const gchar *url)
{
	gchar *scheme = NULL;
	gchar *hostname = NULL;
	gchar *path = NULL;
	HttpClient *client;
	gchar *buffer = NULL;
	gint length;
	GError *err = NULL;
	GdkPixbuf *pixbuf = NULL;

	g_assert(cache_dir != NULL);
	g_assert(url != NULL);

	if(uri_parse(url, &scheme, &hostname, &path))
	{
		client = http_client_new("hostname", hostname, "port", HTTP_DEFAULT_PORT, NULL);
		if(http_client_get(client, path, &err) == HTTP_OK)
		{
			http_client_read_content(client, &buffer, &length);
			if(_pixbuf_loader_save_image(cache_dir, url, buffer, length))
			{
				pixbuf = _pixbuf_loader_get_from_cache_dir(cache_dir, url);
			}

			if(buffer)
			{
				g_free(buffer);
			}
		}

		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}

		g_object_unref(client);
		g_free(scheme);
		g_free(hostname);
		g_free(path);
	}

	return pixbuf;
}

/*
 *	worker:
 */
static gpointer
_pixbuf_loader_worker(PixbufLoader *pixbuf_loader)
{
	PixbufLoaderPrivate *priv = pixbuf_loader->priv;
	gchar *url;
	GTimeVal tv;
	gboolean use_cache_dir;
	GdkPixbuf *pixbuf = NULL;
	GHashTable *pixbufs;
	GList *callbacks, *iter;
	gpointer id;
	_PixbufLoaderCallbackData *callback;
	GQueue *urls = NULL;
	gboolean from_queue;
	gboolean free_url;

	use_cache_dir = _pixbuf_loader_prepare_cache_dir(priv->cache_dir);
	urls = g_queue_new();

	pixbufs = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)g_object_unref); /* a temporarily pixbuf cache */

	while(!g_cancellable_is_cancelled(priv->cancellable))
	{
		g_get_current_time(&tv);
		g_time_val_add(&tv, 2500000);

		/* reveive message/url */
		url = g_async_queue_timed_pop(priv->queue, &tv);
		from_queue = FALSE;
		free_url = TRUE;

		/* try to get url from queue if message is empty */
		if(!url && !g_queue_is_empty(urls))
		{
			url = g_queue_pop_head(urls);
			from_queue = TRUE;
		}

		if(url)
		{
			/* find callbacks assigned to url */
			g_mutex_lock(priv->mutex_callbacks);
			callbacks = iter = _pixbuf_loader_callback_table_get_by_url(priv->callbacks, url);
			g_mutex_unlock(priv->mutex_callbacks);

			if(callbacks)
			{
				/* try to load image from table */
				if(!(pixbuf = g_hash_table_lookup(pixbufs, url)))
				{
					if(use_cache_dir)
					{
						/* try to load image from cache directory */
						if(!(pixbuf = _pixbuf_loader_get_from_cache_dir(priv->cache_dir, url)))
						{
							if(!from_queue)
							{
								/* push url to queue if it cannot be found in cache directory */
								g_queue_push_head(urls, url);
								free_url = FALSE;
							}
							else
							{
								/* get image from server */
								pixbuf = _pixbuf_loader_get_from_server(priv->cache_dir, url);
							}
						}
					}
					else
					{
						/* get image from server */
						pixbuf = _pixbuf_loader_get_from_server(priv->cache_dir, url);
					}

					if(pixbuf)
					{
						g_hash_table_insert(pixbufs, url, pixbuf);
					}
				}

				if(pixbuf)
				{
					g_mutex_lock(priv->mutex_callbacks);

					/* invoke callback functions assigned to given url */
					while(iter)
					{
						id = iter->data;

						if((callback = (_PixbufLoaderCallbackData *)g_hash_table_lookup(priv->callbacks, id)))
						{
							callback->callback(pixbuf, callback->user_data);
							g_hash_table_remove(priv->callbacks, id);
						}
		
						iter = iter->next;
					}

					g_mutex_unlock(priv->mutex_callbacks);
				}

				g_list_free(callbacks);
			}

			if(free_url)
			{
				g_free(url);
			}
		}
		else
		{
			/* clear hash table */
			g_hash_table_remove_all(pixbufs);
		}
	}

	if(urls)
	{
		g_queue_free(urls);
	}

	g_hash_table_destroy(pixbufs);

	return NULL;
}

static void
_pixbuf_loader_worker_destroy_message(gchar *message)
{
	g_free(message);
}

/*
 *	status:
 */
static gboolean 
_pixbuf_loader_get_running(PixbufLoader *pixbuf_loader)
{
	PixbufLoaderPrivate *priv = pixbuf_loader->priv;
	gboolean running = FALSE;

	g_mutex_lock(priv->mutex_running);
	running = priv->running;
	g_mutex_unlock(priv->mutex_running);

	return running;
}

/*
 *	implementation:
 */
static guint
_pixbuf_loader_add_callback(PixbufLoader *pixbuf_loader, const gchar *url, PixbufLoaderCallback callback, const gchar *group, gpointer user_data, GFreeFunc free_func)
{
	guint id = 0;
	PixbufLoaderPrivate *priv = pixbuf_loader->priv;
	_PixbufLoaderCallbackData *item;

	g_assert(url != NULL);
	g_assert(callback != NULL);

	g_mutex_lock(priv->mutex_running);

	if(priv->running == TRUE)
	{
		/* create callback data */
		g_mutex_lock(pixbuf_loader->priv->mutex_callbacks);
		item = _pixbuf_loader_callback_table_insert(priv->callbacks, callback, user_data, free_func, group, url);
		//g_debug("Pixbuf callback created: url=\"%s\", id=%d", url, item->id);
		g_mutex_unlock(priv->mutex_callbacks);

		/* push url */
		g_async_queue_push_sorted(priv->queue, g_strdup(url), (GCompareDataFunc)g_strcmp0, NULL);

		id = item->id;
	}

	g_mutex_unlock(priv->mutex_running);

	return id;
}

static void
_pixbuf_loader_remove_callback(PixbufLoader *pixbuf_loader, guint id)
{
	PixbufLoaderPrivate *priv = pixbuf_loader->priv;

	g_return_if_fail(_pixbuf_loader_get_running(pixbuf_loader));

	g_mutex_lock(pixbuf_loader->priv->mutex_callbacks);
	g_hash_table_remove(priv->callbacks, GINT_TO_POINTER(id));
	g_mutex_unlock(pixbuf_loader->priv->mutex_callbacks);
}

static void
_pixbuf_loader_remove_group(PixbufLoader *pixbuf_loader, const gchar *group)
{
	g_assert(group != NULL);

	g_return_if_fail(_pixbuf_loader_get_running(pixbuf_loader));

	g_debug("Removing callback group \"%s\" from pixbuf loader", group);
	g_mutex_lock(pixbuf_loader->priv->mutex_callbacks);
	g_hash_table_foreach_remove(pixbuf_loader->priv->callbacks, (GHRFunc)_pixbuf_loader_callback_table_remove_group, (gpointer)group);
	g_mutex_unlock(pixbuf_loader->priv->mutex_callbacks);

	return;
}

static void
_pixbuf_loader_start(PixbufLoader *pixbuf_loader)
{
	PixbufLoaderPrivate *priv = pixbuf_loader->priv;
	GError *err = NULL;

	g_mutex_lock(priv->mutex_running);

	/* create message queue & start worker */
	g_assert(priv->running == FALSE);
	g_assert(priv->thread == NULL);
	g_assert(priv->queue == NULL);
	g_assert(priv->cancellable == NULL);

	priv->queue = g_async_queue_new_full((GDestroyNotify)_pixbuf_loader_worker_destroy_message);
	priv->cancellable = g_cancellable_new();

	if((priv->thread = g_thread_create((GThreadFunc)_pixbuf_loader_worker, pixbuf_loader, TRUE, &err)))
	{
		priv->running = TRUE;
	}
	else
	{
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	g_mutex_unlock(priv->mutex_running);
}

static void
_pixbuf_loader_stop(PixbufLoader *pixbuf_loader)
{
	PixbufLoaderPrivate *priv = pixbuf_loader->priv;

	g_mutex_lock(priv->mutex_running);

	/* create message queue & start worker */
	g_assert(priv->running == TRUE);
	g_assert(priv->thread != NULL);
	g_assert(priv->queue != NULL);
	g_assert(priv->cancellable != NULL);

	g_debug("Joining pixbuf worker...");
	g_cancellable_cancel(priv->cancellable);
	g_thread_join(priv->thread);

	g_debug("Cleaning up");
	priv->thread = NULL;
	g_object_unref(priv->cancellable);
	priv->cancellable = NULL;
	g_async_queue_unref(priv->queue);
	priv->queue = NULL;

	g_mutex_unlock(priv->mutex_running);
}

static void
_pixbuf_loader_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	PixbufLoader *pixbuf_loader = PIXBUF_LOADER(object);

	switch(prop_id)
	{
		case PROP_CACHE_DIR:
			g_value_set_string(value, pixbuf_loader->priv->cache_dir);
			break;

		case PROP_RUNNING:
			g_value_set_boolean(value, _pixbuf_loader_get_running(pixbuf_loader));

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
_pixbuf_loader_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	PixbufLoader *pixbuf_loader = PIXBUF_LOADER(object);

	switch(prop_id)
	{
		case PROP_CACHE_DIR:
			if(pixbuf_loader->priv->cache_dir)
			{
				g_free(pixbuf_loader->priv->cache_dir);
			}
			pixbuf_loader->priv->cache_dir = g_value_dup_string(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/*
 *	public:
 */
guint
pixbuf_loader_add_callback(PixbufLoader *pixbuf_loader, const gchar *url, PixbufLoaderCallback callback, const gchar *group, gpointer user_data, GFreeFunc free_func)
{
	return PIXBUF_LOADER_GET_CLASS(pixbuf_loader)->add_callback(pixbuf_loader, url, callback, group, user_data, free_func);
}

void
pixbuf_loader_remove_callback(PixbufLoader *pixbuf_loader, guint id)
{
	PIXBUF_LOADER_GET_CLASS(pixbuf_loader)->remove_callback(pixbuf_loader, id);
}

void
pixbuf_loader_remove_group(PixbufLoader *pixbuf_loader, const gchar *group)
{
	PIXBUF_LOADER_GET_CLASS(pixbuf_loader)->remove_group(pixbuf_loader, group);
}

void
pixbuf_loader_start(PixbufLoader *pixbuf_loader)
{
	PIXBUF_LOADER_GET_CLASS(pixbuf_loader)->start(pixbuf_loader);
}

void
pixbuf_loader_stop(PixbufLoader *pixbuf_loader)
{
	PIXBUF_LOADER_GET_CLASS(pixbuf_loader)->stop(pixbuf_loader);
}


PixbufLoader *
pixbuf_loader_new(const gchar *cache_dir)
{
	PixbufLoader *pixbuf_loader;

	pixbuf_loader = (PixbufLoader *)g_object_new(PIXBUF_LOADER_TYPE, "cache-dir", cache_dir, NULL);

	return pixbuf_loader;
}

/*
 *	initialization/finalization:
 */
static void
_pixbuf_loader_finalize(GObject *object)
{
	PixbufLoader *pixbuf_loader = PIXBUF_LOADER(object);

	if(pixbuf_loader->priv->cache_dir)
	{
		g_free(pixbuf_loader->priv->cache_dir);
	}

	if(pixbuf_loader->priv->mutex_running)
	{
		g_mutex_free(pixbuf_loader->priv->mutex_running);
	}

	if(pixbuf_loader->priv->callbacks)
	{
		g_hash_table_destroy(pixbuf_loader->priv->callbacks);
	}

	if(pixbuf_loader->priv->mutex_callbacks)
	{
		g_mutex_free(pixbuf_loader->priv->mutex_callbacks);
	}

	if(pixbuf_loader->priv->cancellable)
	{
		g_object_unref(pixbuf_loader->priv->cancellable);
	}

	if(pixbuf_loader->priv->queue)
	{
		g_async_queue_unref(pixbuf_loader->priv->queue);
	}

	if(G_OBJECT_CLASS(pixbuf_loader_parent_class)->finalize)
	{
		(*G_OBJECT_CLASS(pixbuf_loader_parent_class)->finalize)(object);
	}
}

static void
pixbuf_loader_class_init(PixbufLoaderClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(PixbufLoaderClass));

	gobject_class->finalize = _pixbuf_loader_finalize;
	gobject_class->get_property = _pixbuf_loader_get_property;
	gobject_class->set_property = _pixbuf_loader_set_property;

	klass->add_callback = _pixbuf_loader_add_callback;
	klass->remove_callback = _pixbuf_loader_remove_callback;
	klass->remove_group = _pixbuf_loader_remove_group;
	klass->start = _pixbuf_loader_start;
	klass->stop = _pixbuf_loader_stop;

	g_object_class_install_property(gobject_class, PROP_RUNNING,
	                                g_param_spec_boolean("running", NULL, NULL, FALSE, G_PARAM_READABLE));
	g_object_class_install_property(gobject_class, PROP_CACHE_DIR,
	                                g_param_spec_string("cache-dir", NULL, NULL, NULL, G_PARAM_READWRITE));
}

static void
pixbuf_loader_init(PixbufLoader *pixbuf_loader)
{
	/* register private data */
	pixbuf_loader->priv = G_TYPE_INSTANCE_GET_PRIVATE(pixbuf_loader, PIXBUF_LOADER_TYPE, PixbufLoaderPrivate);

	/* create mutex to protect status */
	pixbuf_loader->priv->mutex_running = g_mutex_new();

	/* create tables */
	pixbuf_loader->priv->callbacks = _pixbuf_loader_callback_table_create();

	/* create mutex */
	pixbuf_loader->priv->mutex_callbacks = g_mutex_new();
}

/**
 * @}
 */


/***************************************************************************
    begin........: April 2010
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
 * \file cache.c
 * \brief Threadsafe caching.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 28. December 2010
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"

/**
 * @addtogroup Core
 * @{
 */

/*! Defines the type. */
G_DEFINE_TYPE(Cache, cache, G_TYPE_OBJECT);

enum
{
	PROP_0,
	PROP_CACHE_LIMIT,
	PROP_ENABLE_SWAP,
	PROP_SWAP_DIRECTORY
};

/**
 * \struct _CachePrivate
 * \brief Private _Cache data.
 */
struct _CachePrivate
{
	/*! Maximum size of the cache. */
	gint cache_limit;
	/*! Enable swapping, */
	gboolean enable_swap;
	/*! Location of swap files. */
	gchar *swap_directory;
	/*! A mutex for threadsafe access. */
	GMutex *mutex;
	/*! Size of the cache. */
	gint size;
	/*! FALSE until cache_initialize_swap_folder has been called. */
	gboolean swap_initialized;
	/*! A dictionary storing keys and values. */
	GHashTable *table;
	/*! A dictionary storing keys and values. */
	GHashTable *swap_table;
};

/**
 * \struct _CacheItem 
 * \brief Holds item data.
 */
typedef struct
{
	/*! Key assigned to the item. */
	const gchar *key;
	/*! Data of the item. */
	gchar *data;
	/*! Size of the stored data */
	gint size;
	/*! The lifetime. */
	gint lifetime;
	/*! Timestamp of the last modification. */
	gint64 mtime;
	/*! Amount of read accesses. */
	guint read_count;
} _CacheItem;

/*
 *	helpers:
 */
static inline gboolean
_cache_item_expired(_CacheItem *item)
{
	gint64 usec;

	g_debug("Checking item's lifetime: %d", item->lifetime);

	if(item->lifetime == CACHE_INFINITE_LIFETIME)
	{
		return FALSE;
	}

	/* get current time */
	usec = g_get_monotonic_time();

	/* check if lifetime has been expired */
	g_debug("Testing: (usec[%ld] - item->mtime[%ld]) / 1000000 [%ld] > item->lifetime [%d]", usec, item->mtime, (usec - item->mtime) / 1000000, item->lifetime);

	if((gint)((usec - item->mtime) / 1000000) > item->lifetime)
	{
		return TRUE;
	}

	return FALSE;
}

static void
_cache_free_key(gpointer data)
{
	g_debug("Destroying key: \"%s\"", (gchar *)data);
	g_free(data);
}

static void
_cache_remove_item_from_hashtable(gpointer data)
{
	_CacheItem *item = (_CacheItem *)data;

	g_debug("%s", __func__);

	if(item->data)
	{
		g_free(item->data);
	}
	g_free(item);
}

static gint
_cache_compare_hashed_items(const void *elem1, const void *elem2)
{
	_CacheItem *a = *(_CacheItem **)elem1;
	_CacheItem *b = *(_CacheItem **)elem2;

	if(a->read_count == b->read_count)
	{
		return a->mtime - b->mtime;
	}
	else
	{
		return a->read_count - b->read_count;
	}
}

static gboolean
_cache_write_data_to_disk(const gchar * restrict swap_directory, const gchar * restrict key, const gchar * restrict data, gint size)
{
	gchar *path;
	gchar *filename;
	GIOChannel *out;
	gsize written;
	GIOError error;
	GError *err = NULL;
	gboolean ret = FALSE;

	/* build path */
	g_debug("Writing \"%s\" to disk", key);
	filename = g_base64_encode((const guchar *)key, strlen(key));
	path = g_build_filename(swap_directory, G_DIR_SEPARATOR_S, filename, NULL);
	g_free(filename);

	/* write file */
	g_debug("Creating swap file: \"%s\"", path);

	if((out = g_io_channel_new_file(path, "w", &err)))
	{
		g_io_channel_set_encoding(out, NULL, NULL);
		if((error = g_io_channel_write_chars(out, data, size, &written, &err)) == (GIOError)G_IO_STATUS_NORMAL)
		{
			if(size == written)
			{
				ret = TRUE;
			}
			else
			{
				g_warning("Couldn't write swap file, size(%d) != written (%d)", size, (gint)written);
			}
		}
		else
		{
			g_warning("Couldn't write swap file, GError: %d", error);
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
		}

		/* close channel */
		if((error = g_io_channel_shutdown(out, TRUE, &err)) != (GIOError)G_IO_STATUS_NORMAL)
		{
			g_warning("Couldn't close swap file, GError: %d", error);
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
			ret = FALSE;
		}
		g_io_channel_unref(out);
	}
	else
	{
		g_warning("Couldn't create swap file: \"%s\"", path);
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	g_free(path);

	return ret;
}

static gboolean
_cache_write_item_to_disk(Cache *cache, _CacheItem *item)
{
	gboolean ret;

	ret = _cache_write_data_to_disk(cache->priv->swap_directory, item->key, item->data, item->size); 
	g_free(item->data);
	item->data = NULL;

	return ret;
}

static gboolean
_cache_load_item_from_disk(Cache *cache, _CacheItem *item)
{
	gchar *path;
	gchar *filename;
	GIOChannel *in;
	gsize read;
	GIOError error;
	GError *err = NULL;
	gboolean ret = FALSE;

	/* build path */
	g_debug("Reading \"%s\" from disk", item->key);
	filename = g_base64_encode((const guchar *)item->key, strlen(item->key));
	path = g_build_filename(cache->priv->swap_directory, G_DIR_SEPARATOR_S, filename, NULL);
	g_free(filename);

	/* read file */
	g_debug("Reading swap file: \"%s\"", path);
	if((in = g_io_channel_new_file(path, "r", &err)))
	{
		g_io_channel_set_encoding(in, NULL, NULL);
		item->data = (gchar *)g_malloc(item->size);
		if((error = g_io_channel_read_chars(in, item->data, item->size, &read, &err)) == (GIOError)G_IO_STATUS_NORMAL)
		{
			if(read == item->size)
			{
				ret = TRUE;
			}
			else
			{
				g_warning("Couldn't read swap file, item->size(%d) != read (%d)", item->size, (gint)read);
			}
		}

		/* close channel */
		if((error = g_io_channel_shutdown(in, TRUE, &err)) != (GIOError)G_IO_STATUS_NORMAL)
		{
			g_warning("Couldn't close swap file, GError: %d", error);
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
			ret = FALSE;
		 }
		g_io_channel_unref(in);

		/* free data on failure */
		if(!ret && item->data)
		{
			g_free(item->data);
			item->data = NULL;
		}
	}

	g_free(path);

	return ret;
}

static void
_cache_shrink(Cache *cache, gint required_size)
{
	GHashTableIter iter;
	const gchar *key;
	_CacheItem *item;
	gint i = 0;
	gint length = 0;
	_CacheItem **items;
	gboolean remove_item;

	/*
	 *	remove expired items from cache
	 */
	g_hash_table_iter_init(&iter, cache->priv->table);
	while(g_hash_table_iter_next(&iter, (gpointer)&key, (gpointer)&item))
	{
		if(_cache_item_expired(item))
		{
			/* remove item from cache */
			cache->priv->size -= item->size;
			remove_item = TRUE;

			/* test if swap support is enabled & initialized */
			if(cache->priv->enable_swap && cache->priv->swap_initialized && FALSE)
			{
				/* try to write item to disk */
				item->key = key;
				if(_cache_write_item_to_disk(cache, item))
				{
					g_free(item->data);
					item->data = NULL;
					g_debug("Stealing \"%s\" from cache table", item->key);
					g_hash_table_iter_steal(&iter);
					g_debug("Saving \"%s\" in cache swap-table", item->key);
					g_hash_table_insert(cache->priv->swap_table, (gpointer)item->key, (gpointer)item);
					remove_item = FALSE;
				}
				else
				{
					g_warning("Couldn't write item (\"%s\") to disk", key);
				}
			}

			if(remove_item)
			{
				g_debug("Removing \"%s\" from cache", key);
				g_hash_table_iter_remove(&iter);
			}

			/* check if cache is large enough to store an element with the desired size */
			if(cache->priv->cache_limit >= (required_size + cache->priv->size))
			{
				return;
			}
		}
		else
		{
			/* count length of the hashtable */
			++length;
		}
	}

	/*
	 *	the item still exceeds the available cache limit => remove old items
	 */

	/* store items in an array and sort it by read access and modification time */
	g_hash_table_iter_init(&iter, cache->priv->table);
	items = (_CacheItem **)g_malloc(sizeof(_CacheItem *) * length);
	i = 0;

	while(g_hash_table_iter_next(&iter, (gpointer)&key, (gpointer)&item))
	{
		item->key = key;
		items[i] = item;
		++i;
	}

	qsort(items, length, sizeof(_CacheItem *), _cache_compare_hashed_items);

	/* remove items */
	for(i = 0; i < length; ++i)
	{
		cache->priv->size -= items[i]->size;
		remove_item = TRUE;

		/* test if swap support is enabled & initialized */
		if(cache->priv->enable_swap && cache->priv->swap_initialized)
		{
			/* try to write item to disk */
			if(_cache_write_item_to_disk(cache, items[i]))
			{
				g_debug("Stealing \"%s\" from cache table", items[i]->key);
				g_hash_table_steal(cache->priv->table, (gconstpointer)items[i]->key);
				g_debug("Saving \"%s\" to swap-table", items[i]->key);
				g_hash_table_insert(cache->priv->swap_table, (gpointer)(items[i]->key), (gpointer)items[i]);
				remove_item = FALSE;
			}
			else
			{
				g_warning("Couldn't write item (\"%s\") to disk", key);
			}
		}

		if(remove_item)
		{
			g_debug("Removing \"%s\" from cache", items[i]->key);
			g_hash_table_remove(cache->priv->table, (gconstpointer)items[i]->key);
		}
			
		if(cache->priv->cache_limit >= (required_size + cache->priv->size))
		{
			break;
		}
	}

	/* cleanup */
	g_free(items);
}

/*
 *	implementation:
 */
static gboolean
_cache_clear_swap_folder(Cache *cache)
{
	GDir *dir;
	const gchar *entry;
	gchar *filename;
	GError *err = NULL;
	gboolean ret = FALSE;

	g_return_val_if_fail(cache->priv->enable_swap == TRUE, FALSE);
	g_return_val_if_fail(cache->priv->swap_directory != NULL, FALSE);

	g_debug("Removing old files from swap directory");
	if((dir = g_dir_open(cache->priv->swap_directory, 0, &err)))
	{
		ret = TRUE;
		while((entry = g_dir_read_name(dir)))
		{
			filename = g_build_filename(cache->priv->swap_directory, G_DIR_SEPARATOR_S, entry, NULL);
			if(g_file_test(filename, G_FILE_TEST_IS_REGULAR))
			{
				g_debug("Removing regular file: \"%s\"", filename);
				if(g_remove(filename))
				{
					g_warning("Couldn't remove regular file: \"%s\"", filename);
				}
			}
			else
			{
				g_debug("Ignoring non-regular file: \"%s\"", filename);
			}

			g_free(filename);
		}

		g_dir_close(dir);
	}
	else
	{
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	return ret;
}

static gboolean
_cache_initialize_cache_folder(Cache *cache)
{
	gboolean ret = FALSE;

	g_return_val_if_fail(cache->priv->enable_swap == TRUE, FALSE);
	g_return_val_if_fail(cache->priv->swap_directory != NULL, FALSE);
	g_return_val_if_fail(cache->priv->swap_initialized == FALSE, FALSE);

	g_debug("Initializing swap directory: \"%s\"", cache->priv->swap_directory);

	/* check if specified directory does already exist */
	if(g_file_test(cache->priv->swap_directory, G_FILE_TEST_IS_DIR))
	{
		/* remove old files from swap directory */
		ret = _cache_clear_swap_folder(cache);
	}
	else
	{
		/* try to create directorytry to create directory */
		if(g_file_test(cache->priv->swap_directory, G_FILE_TEST_EXISTS))
		{
			g_warning("Specified swap location is not a directory: \"%s\"", cache->priv->swap_directory);
		}
		else
		{
			g_debug("Trying to create swap folder: \"%s\"", cache->priv->swap_directory);
			if(g_mkdir_with_parents(cache->priv->swap_directory, 500))
			{
				g_warning("Couldn't create swap folder: \"%s\"", cache->priv->swap_directory);
			}
			else
			{
				ret = TRUE;
			}
		}
	}

	if(ret)
	{
		cache->priv->swap_initialized = TRUE;
	}
	else
	{
		g_warning("Disabling swapping, initialization failed");
		cache->priv->enable_swap = FALSE;
		g_free(cache->priv->swap_directory);
		cache->priv->swap_directory = NULL;
	}

	return ret;
}

static gboolean
_cache_save(Cache *cache, const gchar * restrict key, const gchar * restrict data, gint size, gint lifetime)
{
	_CacheItem *item;
	gchar *orig_key;
	gint64 usec;
	gboolean ret = FALSE;

	g_mutex_lock(cache->priv->mutex);

	/* check if item size doesn't exceed cache limit */
	if(size <= cache->priv->cache_limit)
	{
		/* get timestamp */
		usec = g_get_monotonic_time();

		/* check if hashtable does already contain an item with the given key */
		if(g_hash_table_lookup_extended(cache->priv->table, (gconstpointer)key, (gpointer)&orig_key, (gpointer)&item))
		{
			g_debug("Replacing cache item: \"%s\", current size: %d, limit: %d", key, cache->priv->size, cache->priv->cache_limit);

			/* don't reallocate memory if the current memory block can hold the new data  */
			if(size <= item->size)
			{
				cache->priv->size -= item->size - size;
				item->mtime = usec;
				item->size = size;
				memcpy(item->data, data, size);
			}
			else
			{
				/* steal item from cache (to prevent _cache_shrink from removing it) */
				g_hash_table_steal(cache->priv->table, (gconstpointer)key);
				cache->priv->size -= item->size;

				/* update data */
				item->mtime = usec;
				item->size = size;
				item->data = (gchar *)g_realloc(item->data, size);
				memcpy(item->data, data, size);

				/* resize cache (if necessary) */
				if(cache->priv->cache_limit < (size + cache->priv->size))
				{
					_cache_shrink(cache, size);
				}

				/* rewrite item to cache */
				g_hash_table_insert(cache->priv->table, (gpointer)orig_key, (gpointer)item);
				cache->priv->size += size;
				ret = TRUE;
			}
	
			g_debug("Replaced cached item: \"%s\", current size: %d, limit: %d", key, cache->priv->size, cache->priv->cache_limit);
		}
		else
		{
			/* test if an item with the given key can be found in swap-table */
			if((item = g_hash_table_lookup(cache->priv->swap_table, (gconstpointer)key)))
			{
				/* save data directly on disk */
				g_debug("Updating swapped item: \"%s\"", key);
				if(_cache_write_data_to_disk(cache->priv->swap_directory, key, data, size))
				{
					item->size = size;
					item->lifetime = lifetime;
					item->mtime = usec;
				}
			}
			else
			{
				g_debug("Adding \"%s\" to cache, current size: %d, limit: %d", key, cache->priv->size, cache->priv->cache_limit);

				/* check if cache has to be resized */
				if(cache->priv->cache_limit < (size + cache->priv->size))
				{
					_cache_shrink(cache, size);
				}

				/* create new item */
				item = (_CacheItem *)g_malloc(sizeof(_CacheItem));
				item->key = NULL;
				item->data = g_memdup(data, size);
				item->size = size;
				item->lifetime = lifetime;
				item->mtime = usec;

				/* append element to cache */
				g_hash_table_insert(cache->priv->table, (gpointer)g_strdup(key), (gpointer)item);
				cache->priv->size += size;
				ret = TRUE;

				g_debug("Added \"%s\" to cache, current size: %d, limit: %d", key, cache->priv->size, cache->priv->cache_limit);
			}
		}

	}
	else
	{
		g_warning("Couldn't write element to cache: size exceeds cache limit");
	}

	g_mutex_unlock(cache->priv->mutex);

	return ret;
}

static gint
_cache_load(Cache *cache, const gchar * restrict key, gchar ** restrict data)
{
	gchar *orig_key;
	_CacheItem *item = NULL;
	gboolean from_disk = FALSE;
	gboolean dup = TRUE;
	gint ret = -1;

	g_mutex_lock(cache->priv->mutex);

	*data = NULL;

	g_debug("Searching cache item \"%s\"", key);

	/* try to load item from memory */
	if(g_hash_table_lookup_extended(cache->priv->table, (gconstpointer)key, (gpointer)&orig_key, (gpointer)&item))
	{
		g_debug("Found cache item \"%s\" in memory", key);

		/* check if item has been expired */
		if(_cache_item_expired(item))
		{
			/* remove item from table */
			g_hash_table_remove(cache->priv->table, (gconstpointer)key);
			cache->priv->size -= item->size;
			item = NULL;
		}
	}
	else if(cache->priv->enable_swap && cache->priv->swap_initialized)
	{
		/* try to load item from disk */
		g_debug("Couldn't find \"%s\" in memory, searching for related swap-file", key);
		if(g_hash_table_lookup_extended(cache->priv->swap_table, (gconstpointer)key, (gpointer)&orig_key, (gpointer)&item))
		{
			g_debug("Found \"%s\" in swap-table", key);
			from_disk = TRUE;

			/* check if item has been expired */
			if(_cache_item_expired(item))
			{
				/* remove item from table */
				g_hash_table_remove(cache->priv->swap_table, (gconstpointer)key);
				item = NULL;
			}
			else
			{
				if(!_cache_load_item_from_disk(cache, item))
				{
					g_warning("Couldn't restore item from disk");
					item = NULL;
				}
			}
		}
	}

	/* copy data */
	if(item)
	{
		if(from_disk)
		{
			/* try to write swapped item back into memory */
			if(item->size <= (cache->priv->cache_limit - cache->priv->size))
			{
				g_debug("Space available: %d, item size: %d", cache->priv->cache_limit, item->size);
				g_debug("Writing \"%s\" back into memory", key);
				g_hash_table_steal(cache->priv->swap_table, (gconstpointer)key);
				g_hash_table_insert(cache->priv->table, (gpointer)orig_key, (gpointer)item);
				cache->priv->size += item->size;
				g_debug("Reinserted \"%s\" into cache, current size: %d, limit: %d", key, cache->priv->size, cache->priv->cache_limit);
			}
			else
			{
				dup = FALSE;
			}
		}

		/* copy data */
		*data = dup ? g_memdup(item->data, item->size) : item->data;
		++item->read_count;
		ret = item->size;

		/* free memory */
		if(from_disk)
		{
			if(dup)
			{
				g_free(item->data);
			}
			item->data = NULL;
		}
	}

	g_mutex_unlock(cache->priv->mutex);

	return ret;
}

static void
_cache_remove(Cache *cache, const gchar *key)
{
	_CacheItem *item;

	g_mutex_lock(cache->priv->mutex);

	g_debug("Removing item from cache: \"%s\"", key);
	if((item = g_hash_table_lookup(cache->priv->table, (gconstpointer)key)))
	{
		cache->priv->size -= item->size;
		g_hash_table_remove(cache->priv->table, (gconstpointer)key);
		g_debug("Item removed: \"%s\", current size: %d, limit: %d", key, cache->priv->size, cache->priv->cache_limit);
	}
	else
	{
		if(g_hash_table_remove(cache->priv->swap_table, (gconstpointer)key))
		{
			g_debug("Item removed from swap-table: \"%s\"", key);
		}
		else
		{
			g_warning("Couldn't find item in cache: \"%s\"", key);
		}
	}

	g_mutex_unlock(cache->priv->mutex);
}

static void
_cache_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	Cache *cache = CACHE(object);

	g_mutex_lock(cache->priv->mutex);

	switch(prop_id)
	{
		case PROP_CACHE_LIMIT:
			g_value_set_int(value, cache->priv->cache_limit);
			break;

		case PROP_ENABLE_SWAP:
			g_value_set_boolean(value, cache->priv->enable_swap);
			break;

		case PROP_SWAP_DIRECTORY:
			g_value_set_string(value, cache->priv->swap_directory);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}

	g_mutex_unlock(cache->priv->mutex);
}

static void
_cache_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	Cache *cache = CACHE(object);
	gint v_int;

	g_mutex_lock(cache->priv->mutex);

	switch(prop_id)
	{
		case PROP_CACHE_LIMIT:
			if((v_int = g_value_get_int(value)) < MINIMUM_CACHE_LIMIT)
			{
				g_warning("cache-limit too small (%d) => setting minimum value (%d)", v_int, MINIMUM_CACHE_LIMIT);
				cache->priv->cache_limit = DEFAULT_CACHE_LIMIT;
			}
			else
			{
				cache->priv->cache_limit = v_int;
			}
			break;

		case PROP_ENABLE_SWAP:
			cache->priv->enable_swap = g_value_get_boolean(value);
			break;

		case PROP_SWAP_DIRECTORY:
			if(cache->priv->swap_directory)
			{
				g_free(cache->priv->swap_directory);
			}
			cache->priv->swap_directory = g_value_dup_string(value);
			break;


		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}

	g_mutex_unlock(cache->priv->mutex);
}

/*
 *	public:
 */
gboolean
cache_initialize_swap_folder(Cache *cache)
{
	return CACHE_GET_CLASS(cache)->initialize_swap_folder(cache);
}

gboolean
cache_clear_swap_folder(Cache *cache)
{
	return CACHE_GET_CLASS(cache)->clear_swap_folder(cache);
}

gboolean
cache_save(Cache *cache, const gchar * restrict key, const gchar * restrict data, gint size, gint lifetime)
{
	return CACHE_GET_CLASS(cache)->save(cache, key, data, size, lifetime);
}

gint
cache_load(Cache *cache, const gchar * restrict key, gchar ** restrict data)
{
	return CACHE_GET_CLASS(cache)->load(cache, key, data);
}

void
cache_remove(Cache *cache, const gchar *key)
{
	CACHE_GET_CLASS(cache)->remove(cache, key);
}

Cache *
cache_new(gint cache_limit, gboolean enable_swap, const gchar *swap_directory)
{
	Cache *cache;

	g_assert((enable_swap == FALSE && swap_directory == NULL) || (enable_swap == TRUE && swap_directory != NULL));

	cache = (Cache *)g_object_new(CACHE_TYPE, "cache-limit", cache_limit, "enable-swap", enable_swap, "swap_directory", swap_directory, NULL);

	return cache;
}

/*
 *	initialization/finalization:
 */
static void
_cache_finalize(GObject *object)
{
	Cache *cache = CACHE(object);

	if(cache->priv->enable_swap && cache->priv->swap_initialized)
	{
		cache_clear_swap_folder(cache);
	}

	if(cache->priv->mutex)
	{
		g_mutex_free(cache->priv->mutex);
	}

	if(cache->priv->table)
	{
		g_debug("Destroying cache table");
		g_hash_table_destroy(cache->priv->table);
	}

	if(cache->priv->swap_table)
	{
		g_debug("Destroying swap table");
		g_hash_table_destroy(cache->priv->swap_table);
	}

	if(cache->priv->swap_directory)
	{
		g_free(cache->priv->swap_directory);
	}

	if(G_OBJECT_CLASS(cache_parent_class)->finalize)
	{
		(*G_OBJECT_CLASS(cache_parent_class)->finalize)(object);
	}
}

static void
cache_class_init(CacheClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	g_type_class_add_private(klass, sizeof(CacheClass));

	gobject_class->finalize = _cache_finalize;
	gobject_class->get_property = _cache_get_property;
	gobject_class->set_property = _cache_set_property;

	klass->initialize_swap_folder = _cache_initialize_cache_folder;
	klass->clear_swap_folder = _cache_clear_swap_folder;
	klass->save = _cache_save;
	klass->load = _cache_load;
	klass->remove= _cache_remove;

	g_object_class_install_property(gobject_class, PROP_CACHE_LIMIT,
	                                g_param_spec_int("cache-limit", NULL, NULL, 0, G_MAXINT, DEFAULT_CACHE_LIMIT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property(gobject_class, PROP_ENABLE_SWAP,
	                                g_param_spec_boolean("enable-swap", NULL, NULL, FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property(gobject_class, PROP_SWAP_DIRECTORY,
	                                g_param_spec_string("swap-directory", NULL, NULL, FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
cache_init(Cache *cache)
{
	/* register private data */
	cache->priv = G_TYPE_INSTANCE_GET_PRIVATE(cache, CACHE_TYPE, CachePrivate);

	/* initialize mutex */
	cache->priv->mutex = g_mutex_new();

	/* create hashtables */
	cache->priv->table = g_hash_table_new_full(g_str_hash, g_str_equal, _cache_free_key, _cache_remove_item_from_hashtable);
	cache->priv->swap_table = g_hash_table_new_full(g_str_hash, g_str_equal, _cache_free_key, _cache_remove_item_from_hashtable);
}

/**
 * @}
 */


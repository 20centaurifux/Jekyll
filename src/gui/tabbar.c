/***************************************************************************
    begin........: June 2010
    copyright....: Sebastian Fedrau
    email........: lord-kefir@arcor.de
 ***************************************************************************/

/***************************************************************************
    this program is free software; you can redistribute it and/or modify
    it under the terms of the gnu general public license as published by
    the free software foundation.

    this program is distributed in the hope that it will be useful,
    but without any warranty; without even the implied warranty of
    merchantability or fitness for a particular purpose. see the gnu
    general public license for more details.
 ***************************************************************************/
/*!
 * \file tabbar.c
 * \brief tab functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 26. December 2011
 */

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "tabbar.h"
#include "statustab.h"
#include "mainwindow.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/*! Signal sent to destroy the tabbar. */
#define TABBAR_KILL_SIGNAL -9

static void _tabbar_destroy_page(GtkNotebook *notebook, gint index); /* prototype required by _tabbar_close_button_clicked */

/*
 *	background worker:
 */

/**
 * \struct _Tabbar
 * \brief Widgets and data of the tabbar.
 */
typedef struct
{
	/*! Pointer to the mainwindow. */
	GtkWidget *mainwindow;
	/*! A GtkNotebook. */
	GtkWidget *notebook;
	/*! Background worker. */
	GThread *thread;
	/*! A message queue. */
	GAsyncQueue *queue;
	/*! Number of objects to destroy. */
	gint object_count;
	/*! A mutex protecting object_count. */
	GMutex *object_mutex;
	/**
	 * \struct _sync
	 * \brief Holds "busy" status and related mutex.
	 *
	 * \var sync
	 * \brief "busy" status and related mutex.
	 */
	struct _sync
	{
		/*! TRUE if mainwindow is synchronzing. */
		gboolean busy;
		/*! Mutex protecting busy. */
		GMutex *mutex;
	} sync;
} _Tabbar;

/*
 *	close tabs:
 */

/*
 * This background worker receives notebook pages through an asynchronous message
 * queue and destroys them. The idea behind it is that we can hide pages immediately
 * if the user closes them and destroy the widget later in the background.
 */
static void
_tabbar_increment_widget_counter(GtkWidget *notebook)
{
	_Tabbar *meta;

	meta = g_object_get_data(G_OBJECT(notebook), "meta");
	g_mutex_lock(meta->object_mutex);
	++meta->object_count;
	g_debug("%s: widget counter: %d", __func__, meta->object_count);
	g_mutex_unlock(meta->object_mutex);
}

static void
_tabbar_decrement_widget_counter(GtkWidget *notebook)
{
	_Tabbar *meta;

	meta = g_object_get_data(G_OBJECT(notebook), "meta");
	g_mutex_lock(meta->object_mutex);
	--meta->object_count;
	g_debug("%s: widget counter: %d", __func__, meta->object_count);
	g_mutex_unlock(meta->object_mutex);
}

static gpointer
_tabbar_destroy_page_widget_worker(_Tabbar *tabbar)
{
	gpointer *page;
	Tab *meta;

	/* pop the item from queue */
	while((page = g_async_queue_pop(tabbar->queue)))
	{
		/* check if we've received a "kill" signal */
		if(GPOINTER_TO_INT(page) == TABBAR_KILL_SIGNAL)
		{
			return NULL;
		}

		/* call destroy handler (if assigned) */
		g_debug("%s: calling assigned destroy handler", __func__);
		if((meta = g_object_get_data(G_OBJECT(page), "meta")))
		{
			if(meta->funcs->destroy)
			{
				meta->funcs->destroy(GTK_WIDGET(page));
			}
			else
			{
				g_warning("Tab doesn't have a destroy handler");
			}
		}
		else
		{
			g_warning("Couldn't get meta information from tab");
		}

		/* destroy widget & update counter*/
		gdk_threads_enter();
		g_object_ref_sink(page);
		gtk_widget_destroy(GTK_WIDGET(page));
		g_object_unref(page);
		gdk_threads_leave();
		_tabbar_decrement_widget_counter(tabbar->notebook);
	}

	g_error("%s: received NULL", __func__);

	return NULL;
}

/*
 * This close handler is connected to the "click" event of the close button in the
 * label of the notebook page.
 */
static void
_tabbar_close_button_clicked(GtkWidget *button, GtkWidget *page)
{
	GtkWidget *notebook;

	notebook = gtk_widget_get_parent(page);
	g_assert(GTK_IS_NOTEBOOK(notebook));

	_tabbar_destroy_page(GTK_NOTEBOOK(notebook), gtk_notebook_page_num(GTK_NOTEBOOK(notebook), page));
}

/*
 *	scrolling:
 */
static gboolean
_tabbar_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	GtkWidget *page;
	gint index;
	Tab *tab;
	gboolean result = FALSE;

	g_assert(GTK_IS_NOTEBOOK(widget));

	if(event->keyval == GDK_Down || event->keyval == GDK_Up)
	{
		if((index = gtk_notebook_get_current_page(GTK_NOTEBOOK(widget))) != -1)
		{
			if((page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), index)))
			{
				if((tab = g_object_get_data(G_OBJECT(page), "meta")) && tab->funcs->scroll)
				{
					tab->funcs->scroll(page, (event->keyval == GDK_Down) ? TRUE : FALSE);
				}
			}
		}

		result = TRUE;
	}

	return result;
}

/*
 *	helpers:
 */
static Tab *
_tabbar_get_current_tab(GtkWidget *widget)
{
	GtkWidget *page;
	gint index;

	g_assert(GTK_IS_NOTEBOOK(widget));

	if((index = gtk_notebook_get_current_page(GTK_NOTEBOOK(widget))) != -1)
	{
		if((page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), index)))
		{
			return g_object_get_data(G_OBJECT(page), "meta");
		}
	}

	return NULL;
}

static void
_tabbar_page_changed(GtkNotebook *notebook, gint index)
{
	Tab *tab = NULL;
	GtkWidget *page;
	gchar *id;
	gchar *title = NULL;
	gchar **pieces;

	g_assert(GTK_IS_NOTEBOOK(notebook));

	if(index == -1)
	{
		tab = _tabbar_get_current_tab(GTK_WIDGET(notebook));
	}
	else if((page = gtk_notebook_get_nth_page(notebook, index)))
	{
		tab = g_object_get_data(G_OBJECT(page), "meta");
	}

	if(tab)
	{
		id = tab_get_id(tab);

		switch(tab->type_id)
		{
			case TAB_TYPE_ID_PUBLIC_TIMELINE:
				title = g_strdup_printf("%s (%s)", _("Messages"), id);
				break;

			case TAB_TYPE_ID_REPLIES:
				title = g_strdup_printf("%s (%s)", _("Replies"), id);
				break;

			case TAB_TYPE_ID_DIRECT_MESSAGES:
				title = g_strdup_printf("%s (%s)", _("Direct Messages"), id);
				break;

			case TAB_TYPE_ID_SEARCH:
				if((pieces = g_strsplit(id, ":", 2)))
				{
					title = g_strdup_printf("%s: %s", _("Search"), pieces[1] + 1);
					g_strfreev(pieces);
				}
				break;

			case TAB_TYPE_ID_USER_TIMELINE:
			case TAB_TYPE_ID_LIST:
				title = g_strdup(id);
				break;

			default:
				g_warning("Invalid tab type.");
		}

		g_free(id);
	}

	mainwindow_set_title(tabbar_get_mainwindow(GTK_WIDGET(notebook)), title);
	g_free(title);
}

static GtkWidget *
_tabbar_create_tab_label(GtkImage *image, const gchar *title, GCallback button_clicked, gpointer user_data)
{
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *button;

	box = gtk_hbox_new(FALSE, 5);

	#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 20
	GtkWidget *spinner;

	spinner = gtk_spinner_new();
	gtk_widget_set_size_request(spinner, 16, 16);
	gtk_spinner_stop(GTK_SPINNER(spinner));
	gtk_box_pack_start(GTK_BOX(box), spinner, TRUE, TRUE, 0);
	#endif

	if(image)
	{
		gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(image), TRUE, TRUE, 0);
	}

	label = gtk_label_new(title);
	gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

	button = gtk_button_new();
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(button), "clicked", button_clicked, user_data);

	gtk_widget_show_all(box);

	#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 20
	gtk_widget_hide(spinner);
	#endif

	return box;
}

static gint
_tabbar_append_page(GtkNotebook *notebook, GtkImage *image, const gchar *title, GtkWidget *child)
{
	GtkWidget *label;
	gint index;

	label = _tabbar_create_tab_label(image, title, G_CALLBACK(_tabbar_close_button_clicked), child);
	index = gtk_notebook_append_page(notebook, child, label);
	gtk_notebook_set_tab_reorderable(notebook, child, TRUE);
	gtk_widget_show_all(child);

	return index;
}

static void
_tabbar_destroy_page(GtkNotebook *notebook, gint index)
{
	_Tabbar *meta;
	GtkWidget *child;

	g_debug("Destroying tab(%d)", index);

	/* create object reference & remove page from notebook */
	child = g_object_ref(gtk_notebook_get_nth_page(notebook, index));
	gtk_notebook_remove_page(notebook, index);

	/* push page object into remove queue */
	meta = (_Tabbar *)g_object_get_data(G_OBJECT(notebook), "meta");
	g_async_queue_push(meta->queue, child);

	/* update window title */
	_tabbar_page_changed(notebook, -1);
}

static gint
_tabbar_find_page(GtkNotebook *notebook, TabTypeId type_id, const gchar *id)
{
	gint count;
	GtkWidget *page;
	Tab *tab;
	gchar *tab_id;

	g_debug("Searching for tab");

	count = gtk_notebook_get_n_pages(notebook);
	for(gint i = 0; i < count; ++i)
	{
		g_debug("Trying to get meta information from tab");
		page = gtk_notebook_get_nth_page(notebook, i);

		if((tab = g_object_get_data(G_OBJECT(page), "meta")))
		{
			tab_id = tab_get_id(tab);

			g_debug("Found meta information: type_id=%d, id=\"%s\"", tab->type_id, tab_id);
			if(tab->type_id == type_id && !g_ascii_strcasecmp(tab_id, id))
			{
				return i;
			}

			g_free(tab_id);
		}
		else
		{
			g_warning("Couldn't get meta information from tab");
		}
	}

	return -1;
}

static void
_tabbar_refresh_page(GtkNotebook *notebook, gint index)
{
	GtkWidget *page;
	Tab *meta;

	g_debug("Refreshing tab(%d)", index);

	if((page = gtk_notebook_get_nth_page(notebook, index)))
	{
		if((meta = g_object_get_data(G_OBJECT(page), "meta")))
		{
			if(meta->funcs->refresh)
			{
				meta->funcs->refresh(page);
			}
			else
			{
				g_debug("Tab doesn't have a refresh handler");
			}
		}
		else
		{
			g_warning("Couldn't get meta information from tab");
		}
	}
}

static void
_tabbar_open_status_page(GtkNotebook *notebook, TabTypeId type_id, const gchar *id)
{
	gint index;
	GtkWidget *image = NULL;
	GtkWidget *child;

	if((index = _tabbar_find_page(notebook, type_id, id)) != -1)
	{
		gtk_notebook_set_current_page(notebook, index);
	}
	else
	{
		switch(type_id)
		{
			case TAB_TYPE_ID_PUBLIC_TIMELINE:
				image = gtk_image_new_from_file(pathbuilder_load_path("icon_message_16"));
				break;

			case TAB_TYPE_ID_DIRECT_MESSAGES:
				image = gtk_image_new_from_file(pathbuilder_load_path("icon_direct_message_16"));
				break;

			case TAB_TYPE_ID_REPLIES:
				image = gtk_image_new_from_file(pathbuilder_load_path("icon_replies_16"));
				break;

			case TAB_TYPE_ID_USER_TIMELINE:
				image = gtk_image_new_from_file(pathbuilder_load_path("icon_usertimeline_16"));
				break;
	
			case TAB_TYPE_ID_LIST:
				image = gtk_image_new_from_file(pathbuilder_load_path("icon_lists_16"));
				break;

			case TAB_TYPE_ID_SEARCH:
				image = gtk_image_new_from_file(pathbuilder_load_path("icon_search_16"));
				break;

			default:
				g_warning("%s: unexpected type_id(%d)", __func__, type_id);
				return;
		}

		/* create new page */
		child = status_tab_create(GTK_WIDGET(notebook), type_id, id);
		index = _tabbar_append_page(notebook, GTK_IMAGE(image), id, child);
		_tabbar_refresh_page(notebook, index);
		gtk_notebook_set_current_page(notebook, index);

		/* update widgetcounter */
		_tabbar_increment_widget_counter(GTK_WIDGET(notebook));

		/* set page busy */
		if(tabbar_is_busy(GTK_WIDGET(notebook)))
		{
			tabbar_set_page_busy(GTK_WIDGET(notebook), child, TRUE);
		}
	}
}

/*
 *	handle page switches:
 */
static void
_tabbar_switch_page(GtkNotebook *notebook, gpointer page, guint page_num, gpointer user_data)
{
	_tabbar_page_changed(notebook, page_num);
}

/*
 *	public:
 */
gchar *
tab_get_id(Tab *tab)
{
	gchar *id;

	g_mutex_lock(tab->id.mutex);
	id = g_strdup(tab->id.id);
	g_mutex_unlock(tab->id.mutex);

	return id;
}

void
tab_set_id(Tab *tab, const gchar *id)
{
	g_mutex_lock(tab->id.mutex);
	g_free(tab->id.id);
	tab->id.id = g_strdup(id);
	g_mutex_unlock(tab->id.mutex);
}

GtkWidget *
tabbar_create(GtkWidget *parent)
{
	GtkWidget *widget;
	_Tabbar *meta;
	GError *err = NULL;

	/* create widget */
	widget = gtk_notebook_new();
	g_object_set(G_OBJECT(widget), "tab-border", 0, "scrollable", TRUE, NULL);

	/* create meta information */
	meta = (_Tabbar *)g_malloc(sizeof(_Tabbar));
	meta->notebook = widget;
	meta->mainwindow = parent;
	meta->queue = g_async_queue_new();
	meta->thread = g_thread_create((GThreadFunc)_tabbar_destroy_page_widget_worker, meta, TRUE, &err);
	meta->object_count = 0;
	meta->object_mutex = g_mutex_new();
	meta->sync.busy = FALSE;
	meta->sync.mutex = g_mutex_new();
	g_object_set_data(G_OBJECT(widget), "meta", meta);

	if(!meta->thread)
	{
		g_error("%s", err->message);
	}

	g_signal_connect(G_OBJECT(meta->notebook), "key-press-event", G_CALLBACK(_tabbar_key_press), NULL);
	g_signal_connect(G_OBJECT(meta->notebook), "switch-page", G_CALLBACK(_tabbar_switch_page), NULL);

	return widget;
}

void
tabbar_destroy(GtkWidget *widget)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(widget);
	_Tabbar *meta;
	gint count;

	/* close tabs */
	g_debug("Closing tabs...");
	gdk_threads_enter();
	while(gtk_notebook_get_n_pages(notebook))
	{
		_tabbar_destroy_page(notebook, 0);
	}
	gdk_threads_leave();

	/* get meta information */
	meta = (_Tabbar *)g_object_get_data(G_OBJECT(widget), "meta");

	/* wait for page widgets to destroy */
	g_debug("%s: waiting for widgets to destroy...", __func__);
	for( ;; )
	{
		g_mutex_lock(meta->object_mutex);
		count = meta->object_count;
		g_mutex_unlock(meta->object_mutex);
		g_debug("%s: page widgets in queue: %d", __func__, count);

		if(!count)
		{
			break;
		}

		g_usleep(500000);
	}

	/* send "kill" signal & join background worker */
	g_debug("%s: sending \"kill\" signal", __func__);
	g_async_queue_push(meta->queue, GINT_TO_POINTER(TABBAR_KILL_SIGNAL));

	g_debug("%s: joining background worker...", __func__);
	g_thread_join(meta->thread);

	/* free memory */
	g_async_queue_unref(meta->queue);
	g_mutex_free(meta->object_mutex);
	g_mutex_free(meta->sync.mutex);
	g_free(meta);
}

void
tabbar_refresh(GtkWidget *widget)
{
	gint count;

	count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(widget));
	for(gint i = 0; i < count; ++i)
	{
		_tabbar_refresh_page(GTK_NOTEBOOK(widget), i);
	}
}

gchar *
tabbar_get_current_id(GtkWidget *widget)
{
	Tab *tab;

	g_assert(GTK_IS_NOTEBOOK(widget));

	if((tab = _tabbar_get_current_tab(widget)))
	{
		return g_strdup(tab_get_id(tab));
	}

	return NULL;
}

gint
tabbar_get_current_type(GtkWidget *widget)
{
	Tab *tab;

	g_assert(GTK_IS_NOTEBOOK(widget));

	if((tab = _tabbar_get_current_tab(widget)))
	{
		return tab->type_id;
	}

	return -1;
}

void
tabbar_close_current_tab(GtkWidget *widget)
{
	gint index;

	g_assert(GTK_IS_NOTEBOOK(widget));

	if((index = gtk_notebook_get_current_page(GTK_NOTEBOOK(widget))) != -1)
	{
		_tabbar_destroy_page(GTK_NOTEBOOK(widget), index);
	}
}

void
tabbar_close_tab(GtkWidget *widget, TabTypeId type_id, const gchar *id)
{
	gint index;

	if((index = _tabbar_find_page(GTK_NOTEBOOK(widget), type_id, id)) != -1)
	{
		_tabbar_destroy_page(GTK_NOTEBOOK(widget), index);
	}
}

void
tabbar_open_public_timeline(GtkWidget *widget, const gchar *id)
{
	_tabbar_open_status_page(GTK_NOTEBOOK(widget), TAB_TYPE_ID_PUBLIC_TIMELINE, id);
}

void
tabbar_open_direct_messages(GtkWidget *widget, const gchar *id)
{
	_tabbar_open_status_page(GTK_NOTEBOOK(widget), TAB_TYPE_ID_DIRECT_MESSAGES, id);
}

void
tabbar_open_replies(GtkWidget *widget, const gchar *id)
{
	_tabbar_open_status_page(GTK_NOTEBOOK(widget), TAB_TYPE_ID_REPLIES, id);
}

void
tabbar_open_user_timeline(GtkWidget *widget, const gchar *user)
{
	_tabbar_open_status_page(GTK_NOTEBOOK(widget), TAB_TYPE_ID_USER_TIMELINE, user);
}

void
tabbar_open_list(GtkWidget *widget, const gchar *user, const gchar *list)
{
	gchar *id;

	id = g_strdup_printf("%s@%s", user, list);
	_tabbar_open_status_page(GTK_NOTEBOOK(widget), TAB_TYPE_ID_LIST, id);
	g_free(id);
}

void
tabbar_update_list(GtkWidget *widget, const gchar *user, const gchar *old_listname, const gchar *new_listname)
{
	gint index;
	GtkWidget *page;
	gchar id[128];
	GtkWidget *label;
	GList *children;
	GList *iter;
	Tab *tab;
	gboolean found = FALSE;

	/* build old id */
	sprintf(id, "%s@%s", user, old_listname);
	g_debug("Searching for list to update: \"%s\"", id);

	/* try to find list */
	if((index = _tabbar_find_page(GTK_NOTEBOOK(widget), TAB_TYPE_ID_LIST, id)) == -1)
	{
		return;
	}

	page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), index);

	/* build new id */
	sprintf(id, "%s@%s", user, new_listname);

	/* get label related to specified page */
	if((label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(widget), page)))
	{
		g_assert(GTK_IS_CONTAINER(label));

		/* search for label widget */
		iter = children = gtk_container_get_children(GTK_CONTAINER(label));

		while(iter && !found)
		{
			if(GTK_IS_LABEL(iter->data))
			{
				found = TRUE;

				/* update page header */
				g_debug("Updating tab header");
				gtk_label_set_text(GTK_LABEL(iter->data), id);

				/* update id */
				g_debug("Updating tab id");
				tab = (Tab *)g_object_get_data(G_OBJECT(page), "meta");
				tab_set_id(tab, id);
			}

			iter = iter->next;
		}

		/* cleanup */
		g_list_free(children);
	}
}

void
tabbar_open_search_query(GtkWidget *widget, const gchar *user, const gchar *query)
{
	gchar *id;

	id = g_strdup_printf("%s: %s", user, query);
	_tabbar_open_status_page(GTK_NOTEBOOK(widget), TAB_TYPE_ID_SEARCH, id);
	g_free(id);
}

void
tabbar_set_busy(GtkWidget *widget, gboolean busy)
{
	_Tabbar *meta;
	gint count;
	GtkWidget *page;

	g_assert(GTK_IS_NOTEBOOK(widget));

	/* update busy status */
	meta = g_object_get_data(G_OBJECT(widget), "meta");
	g_mutex_lock(meta->sync.mutex);
	meta->sync.busy = busy;
	g_mutex_unlock(meta->sync.mutex);

	/* update pages */
	count = gtk_notebook_get_n_pages(GTK_NOTEBOOK(widget));

	for(gint i = 0; i < count; ++i)
	{
		if((page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), i)))
		{
			tabbar_set_page_busy(widget, page, busy);
		}
	}
}

gboolean
tabbar_is_busy(GtkWidget *widget)
{
	_Tabbar *meta;
	gboolean busy;

	g_assert(GTK_IS_NOTEBOOK(widget));

	meta = g_object_get_data(G_OBJECT(widget), "meta");

	g_mutex_lock(meta->sync.mutex);
	busy = meta->sync.busy;
	g_mutex_unlock(meta->sync.mutex);

	return busy;
}

void
tabbar_set_page_busy(GtkWidget *widget, GtkWidget *page, gboolean busy)
{
	Tab *meta;

	/* call busy handler */
	if((meta = g_object_get_data(G_OBJECT(page), "meta")))
	{
		if(meta->funcs->set_busy)
		{
			meta->funcs->set_busy(page, busy);
		}
	}

	/* update page header */
	#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 20
	GtkWidget *label;
	GList *children;
	GList *iter;
	gboolean found = FALSE;
	Tab *tab;

	tab = g_object_get_data(G_OBJECT(page), "meta");

	if(!busy && tabbar_is_busy(widget) && tab->type_id != TAB_TYPE_ID_USER_TIMELINE)
	{
		return;
	}

	/* get label related to specified page */
	if((label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(widget), page)))
	{
		g_assert(GTK_IS_CONTAINER(label));

		/* search for spinner widget */
		iter = children = gtk_container_get_children(GTK_CONTAINER(label));

		while(iter && !found)
		{
			/* check if we've found the spinner widget & update status/visibility */
			if(GTK_IS_SPINNER(iter->data))
			{
				found = TRUE;

				if(busy)
				{
					gtk_widget_show(GTK_WIDGET(iter->data));
					gtk_spinner_start(GTK_SPINNER(iter->data));
				}
				else
				{
					gtk_widget_hide(GTK_WIDGET(iter->data));
					gtk_spinner_stop(GTK_SPINNER(iter->data));
				}
			}

			iter = iter->next;
		}

		/* cleanup */
		g_list_free(children);
	}
	#endif
}

GtkWidget *
tabbar_get_mainwindow(GtkWidget *widget)
{
	_Tabbar *meta;

	meta = g_object_get_data(G_OBJECT(widget), "meta");

	g_assert(meta != NULL);
	g_assert(GTK_IS_WINDOW(meta->mainwindow));

	return meta->mainwindow;
}

/**
 * @}
 */


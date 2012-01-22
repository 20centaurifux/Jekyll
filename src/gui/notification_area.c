/***************************************************************************
    begin........: March 2011
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
 * \file notification_area.c
 * \brief Displays notifications.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 22. January 2011
 */

#include <time.h>

#include "notification_area.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _NotificationAreaData
 * \brief Data of the notification area.
 */
typedef struct
{
	/*! A treeview. */
	GtkWidget *tree;
	/*! Pixbuf to display a debug message. */
	GdkPixbuf *pixbuf_debug;
	/*! Pixbuf to display an information message. */
	GdkPixbuf *pixbuf_info;
	/*! Pixbuf to display a warning message. */
	GdkPixbuf *pixbuf_warning;
} _NotificationAreaData;

enum
{
	NOTIFICATION_AREA_TREEVIEW_PIXBUF,
	NOTIFICATION_AREA_TREEVIEW_DATE,
	NOTIFICATION_AREA_TREEVIEW_MESSAGE
};

/*
 *	public:
 */
void
notification_area_notify(GtkWidget *area, NotificationLevel level, const gchar *message)
{
	_NotificationAreaData *meta;
	GtkTreeModel *model;
	GtkTreeIter iter;
	time_t now;
	struct tm *time_info;
	gchar timestamp[24];
	GdkPixbuf *pixbuf = NULL;

	g_assert(GTK_IS_BOX(area));

	meta = (_NotificationAreaData *)g_object_get_data(G_OBJECT(area), "meta");
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(meta->tree));

	time(&now);
	time_info = localtime(&now);
	strftime(timestamp, 24, "%d.%m.%Y %H:%M:%S", time_info);

	if(level == NOTIFICATION_LEVEL_WARNING)
	{
		pixbuf = meta->pixbuf_warning;
	}
	else if(level == NOTIFICATION_LEVEL_DEBUG)
	{
		pixbuf = meta->pixbuf_debug;
	}
	else
	{
		pixbuf = meta->pixbuf_info;
	}

	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model),
	                   &iter,
	                   NOTIFICATION_AREA_TREEVIEW_PIXBUF,
	                   pixbuf,	
	                   NOTIFICATION_AREA_TREEVIEW_DATE,
	                   timestamp,
	                   NOTIFICATION_AREA_TREEVIEW_MESSAGE,
	                   message,
	                   -1);
}

GtkWidget *
notification_area_create(void)
{
	_NotificationAreaData *meta;
	GtkWidget *tree;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *image;
	GtkWidget *box;
	GtkWidget *scrolled;

	meta = (_NotificationAreaData *)g_malloc(sizeof(_NotificationAreaData));

	/* create treeview & model */
	tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

	store = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));

	/* insert column to display icon & date */
	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", NOTIFICATION_AREA_TREEVIEW_PIXBUF);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", NOTIFICATION_AREA_TREEVIEW_DATE);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* insert column to display message */
	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", NOTIFICATION_AREA_TREEVIEW_MESSAGE);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* load images */
	image = gtk_image_new();

	meta->pixbuf_debug = gtk_widget_render_icon(image, GTK_STOCK_INFO, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	meta->pixbuf_info = gtk_widget_render_icon(image, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	meta->pixbuf_warning = gtk_widget_render_icon(image, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

	g_object_ref_sink(G_OBJECT(image));
	gtk_widget_destroy(image);
	g_object_unref(G_OBJECT(image));

	/* create box */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled), tree);

	box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);

	meta->tree = tree;
	g_object_set_data(G_OBJECT(box), "meta", meta);

	return box;
}

void
notification_area_destroy(GtkWidget *area)
{
	_NotificationAreaData *meta;

	g_assert(GTK_IS_BOX(area));

	meta = g_object_get_data(G_OBJECT(area), "meta");

	g_object_unref(G_OBJECT(meta->pixbuf_debug));
	g_object_unref(G_OBJECT(meta->pixbuf_info));
	g_object_unref(G_OBJECT(meta->pixbuf_warning));

	g_free(meta);
}

/**
 * @}
 */


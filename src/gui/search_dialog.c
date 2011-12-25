/***************************************************************************
    begin........: December 2011
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
 * \file search_dialog.c
 * \brief A dialog for entering search queries.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 25. December 2011
 */

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h> 

#include "search_dialog.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _SearchWindowPrivate
 * \brief Private data of the "search" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! Entry holding search query. */
	GtkWidget *entry_query;
} _SearchWindowPrivate;

/*
 *	helpers:
 */
static const gchar *
_search_dialog_get_text(GtkWidget *dialog)
{
	return gtk_entry_get_text(GTK_ENTRY(((_SearchWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private"))->entry_query));
}

/*
 *	events:
 */
static void
_search_dialog_destroy(GtkWidget *dialog, gpointer user_data)
{
	g_assert(GTK_IS_DIALOG(dialog));

	g_free(g_object_get_data(G_OBJECT(dialog), "private"));
}

static gboolean
_search_dialog_key_press(GtkWidget *dialog, GdkEventKey *event, gpointer user_data)
{
	if(event->keyval == GDK_Return)
	{
		gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	}

	return FALSE;
}

/*
 *	public:
 */
GtkWidget *
search_dialog_create(GtkWidget *parent)
{
	GtkWidget *dialog;
	_SearchWindowPrivate *private;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *align;

	g_debug("Opening \"search\" dialog");

	/* create dialog */
	private = (_SearchWindowPrivate *)g_malloc(sizeof(_SearchWindowPrivate));
	private->parent = parent;

	dialog = gtk_dialog_new_with_buttons(_("Search"), GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* main boxes */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, FALSE, 0);

	/* insert image */
	image = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

	/* create table */
	frame = gtk_frame_new(_("Search"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 5);

	/* search query */
	align = gtk_alignment_new(0, 0.5, 0, 0);
	label = gtk_label_new(_("Please enter your search query:"));
	gtk_container_add(GTK_CONTAINER(align), label);

	private->entry_query = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(private->entry_query), 16);

	gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), private->entry_query, 1, 2, 0, 1);

	/* insert buttons */
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_FIND), GTK_RESPONSE_OK);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_CANCEL), GTK_RESPONSE_NO);

	/* connect signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_search_dialog_destroy), NULL);
	g_signal_connect(dialog, "key-press-event", G_CALLBACK(_search_dialog_key_press), NULL);

	/* show widgets */
	gtk_widget_show_all(dialog);

	return dialog;
}

gchar *
search_dialog_get_query(GtkWidget *dialog)
{
	gchar *text = NULL;

	if((text = g_strdup(_search_dialog_get_text(dialog))))
	{
		if(!strlen((text = g_strstrip(text))))
		{
			g_free(text);
			text = NULL;
		}
	}

	return text;
}

/**
 * @}
 */


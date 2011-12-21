/***************************************************************************
   begin........: April 2011
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
 * \file add_list_dialog.c
 * \brief Remove lists.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 29. April 2011
 */

#include <glib/gi18n.h>

#include "add_list_dialog.h"
#include "gtkdeletabledialog.h"
#include "mainwindow.h"
#include "gtk_helpers.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _AddListWindowPrivate
 * \brief Private data of the "add list" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! List owner. */
	gchar owner[64];
	/*! Entry holding list name. */
	GtkWidget *entry_name;
	/*! Indicates if the list is protected. */
	GtkWidget *checkbox_protected;
} _AddListWindowPrivate;

/*
 *	apply worker:
 */
static gpointer
_add_list_dialog_apply_worker(GtkWidget *dialog)
{
	_AddListWindowPrivate *private;
	TwitterClient *client;
	GError *err = NULL;
	GtkWidget *message_dialog;
	GtkWidget *parent;
	gboolean success = FALSE;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_AddListWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	parent = private->parent;

	/* add list */
	if((client = mainwindow_create_twittter_client(parent, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
	{
		if((twitter_client_add_list(client,
		    private->owner,
		    gtk_entry_get_text(GTK_ENTRY(private->entry_name)),
		    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->checkbox_protected)),
		    &err)))
		{
			success = TRUE;
		}

		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}

		g_object_unref(client);
	}
	else
	{
		g_warning("Couldn't create TwitterClient instance.");
	}

	if(success)
	{
		/* update GUI */
		mainwindow_sync_gui(parent);

		/* close dialog */
		gdk_threads_enter();
		gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), GTK_RESPONSE_APPLY);
		gdk_threads_leave();
	}
	else
	{
		/* close dialog */
		gdk_threads_enter();
		gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), GTK_RESPONSE_APPLY);
		gdk_threads_leave();

		/* display failure message */
		message_dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
							GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							GTK_BUTTONS_OK,
							_("Couldn't create list, please try again later."));
		g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, message_dialog);
	}

	return NULL;
}

/*
 *	events:
 */
static void
_add_list_dialog_destroy(GtkWidget *dialog, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	g_free(g_object_get_data(G_OBJECT(dialog), "private"));
}

static gboolean
_add_list_dialog_delete(GtkDeletableDialog *dialog, GdkEvent event, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	if(gtk_widget_get_sensitive(GTK_WIDGET(dialog)))
	{
		gtk_deletable_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
	}

	return TRUE;
}

static void
_add_list_dialog_apply_clicked(GtkWidget *button, GtkWidget *dialog)
{
	GThread *thread;
	GError *err = NULL;

	/* set window busy */
	gtk_helpers_set_widget_busy(dialog, TRUE);

	/* create worker */
	thread = g_thread_create((GThreadFunc)_add_list_dialog_apply_worker, dialog, FALSE, &err);

	if(!thread)
	{
		if(err)
		{
			g_error("%s", err->message);
			g_error_free(err);
		}
		else
		{
			g_error("Couldn't create worker.");
		}
	}
}

/*
 *	public:
 */
GtkWidget *
add_list_dialog_create(GtkWidget *parent, const gchar *owner)
{
	GtkWidget *dialog;
	_AddListWindowPrivate *private;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *image;
	gchar *filename;
	GtkWidget *label;
	GtkWidget *area;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *align;
	GtkWidget *button_apply;

	g_debug("Opening \"add list\" dialog");

	/* create dialog */
	private = (_AddListWindowPrivate *)g_malloc(sizeof(_AddListWindowPrivate));
	private->parent = parent;
	g_strlcpy(private->owner, owner, 64);

	dialog = gtk_deletable_dialog_new_with_buttons(_("Create new list"), GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* main boxes */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_add(GTK_CONTAINER(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog))), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* insert image */
	filename = pathbuilder_build_image_path("list.png");
	image = gtk_image_new_from_file(filename);
	g_free(filename);

	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

	/* create table */
	frame = gtk_frame_new(_("List details"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 5);

	/* list name */
	align = gtk_alignment_new(0, 0.5, 0, 0);
	label = gtk_label_new(_("Name"));
	gtk_container_add(GTK_CONTAINER(align), label);

	private->entry_name = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(private->entry_name), 32);

	gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), private->entry_name, 1, 2, 0, 1);

	/* protected */
	align = gtk_alignment_new(0, 0, 0, 0);
	label = gtk_label_new(_("Private"));
	gtk_container_add(GTK_CONTAINER(align), label);

	private->checkbox_protected = gtk_check_button_new();

	gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), private->checkbox_protected, 1, 2, 2, 3);

	/* insert buttons */
	area = gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));

	button_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_box_pack_start(GTK_BOX(area), button_apply, FALSE, FALSE, 0);

	gtk_deletable_dialog_add_action_widget(GTK_DELETABLE_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_CANCEL), GTK_RESPONSE_NO);

	/* connect signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_add_list_dialog_destroy), NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(_add_list_dialog_delete), NULL);
	g_signal_connect(button_apply, "clicked", G_CALLBACK(_add_list_dialog_apply_clicked), dialog);

	/* show widgets */
	gtk_widget_show_all(dialog);

	return dialog;
}

/**
 * @}
 */

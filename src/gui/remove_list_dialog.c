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
 * \file remove_list_dialog.c
 * \brief Remove lists.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 27. February 2012
 */

#include <glib/gi18n.h>

#include "remove_list_dialog.h"
#include "gtkdeletabledialog.h"
#include "mainwindow.h"
#include "gtk_helpers.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _RemoveListWindowPrivate
 * \brief Private data of the "remove list" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! List owner. */
	gchar owner[64];
	/*! Listname. */
	gchar listname[64];
} _RemoveListWindowPrivate;

/*
 *	apply worker:
 */
static gpointer
_remove_list_dialog_apply_worker(GtkWidget *dialog)
{
	_RemoveListWindowPrivate *windata;
	TwitterClient *client;
	GError *err = NULL;
	GtkWidget *message_dialog;
	GtkWidget *parent;
	gboolean success = FALSE;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	windata = (_RemoveListWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "windata");
	parent = windata->parent;

	if(mainwindow_get_current_sync_status(parent) != MAINWINDOW_SYNC_STATUS_SYNC_LISTS)
	{
		/* remove list */
		if((client = mainwindow_create_twittter_client(parent, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
		{
			success = twitter_client_remove_list(client, windata->owner, windata->listname, &err);

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
	}

	if(success)
	{
		/* update GUI */
		mainwindow_notify_list_removed(parent, windata->owner, windata->listname);

		/* close dialog */
		gdk_threads_enter();
		gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), GTK_RESPONSE_YES);
		gdk_threads_leave();
	}
	else
	{
		/* close dialog */
		gdk_threads_enter();
		gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), GTK_RESPONSE_YES);
		gdk_threads_leave();

		/* display failure message */
		message_dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
							GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							GTK_BUTTONS_OK,
							_("Couldn't remove list, please try again later."));
		g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, message_dialog);
	}

	return NULL;
}

/*
 *	events:
 */
static void
_remove_list_dialog_destroy(GtkWidget *dialog, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	g_free(g_object_get_data(G_OBJECT(dialog), "windata"));
}

static gboolean
_remove_list_dialog_delete(GtkDeletableDialog *dialog, GdkEvent event, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	if(gtk_widget_get_sensitive(GTK_WIDGET(dialog)))
	{
		gtk_deletable_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
	}

	return TRUE;
}

static void
_remove_list_dialog_apply_clicked(GtkWidget *button, GtkWidget *dialog)
{
	GThread *thread;
	GError *err = NULL;

	/* set window busy */
	gtk_helpers_set_widget_busy(dialog, TRUE);

	/* create worker */
	thread = g_thread_create((GThreadFunc)_remove_list_dialog_apply_worker, dialog, FALSE, &err);

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
remove_list_dialog_create(GtkWidget *parent, const gchar *owner, const gchar *listname)
{
	GtkWidget *dialog;
	_RemoveListWindowPrivate *windata;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *alignment;
	GtkWidget *image;
	GtkWidget *label;
	gchar *escaped;
	gchar *text;
	GtkWidget *area;
	GtkWidget *button_apply;

	g_debug("Opening \"remove list\" dialog");

	/* create dialog */
	windata = (_RemoveListWindowPrivate *)g_malloc(sizeof(_RemoveListWindowPrivate));
	windata->parent = parent;
	g_strlcpy(windata->owner, owner, 64);
	g_strlcpy(windata->listname, listname, 64);

	dialog = gtk_deletable_dialog_new_with_buttons(_("Remove list"), GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "windata", windata);

	/* main boxes */
	hbox = gtk_hbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
	gtk_container_add(GTK_CONTAINER(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog))), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* insert image */
	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

	/* insert label */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);

	escaped = g_markup_escape_text(listname, -1);
	text = g_strdup_printf(_("Do you really want to remove the list (<b>%s</b>)?"), escaped);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), text);
	gtk_container_add(GTK_CONTAINER(alignment), label);

	/* insert buttons */
	area = gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));

	button_apply = gtk_button_new_from_stock(GTK_STOCK_YES);
	gtk_box_pack_start(GTK_BOX(area), button_apply, FALSE, FALSE, 0);

	gtk_deletable_dialog_add_action_widget(GTK_DELETABLE_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_NO), GTK_RESPONSE_NO);

	/* connect signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_remove_list_dialog_destroy), NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(_remove_list_dialog_delete), NULL);
	g_signal_connect(button_apply, "clicked", G_CALLBACK(_remove_list_dialog_apply_clicked), dialog);

	/* show widgets */
	gtk_widget_show_all(dialog);

	/* cleanup */
	g_free(escaped);
	g_free(text);

	return dialog;
}

/**
 * @}
 */


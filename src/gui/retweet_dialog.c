/***************************************************************************
    begin........: July 2011
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
 * \file retweet_dialog.c
 * \brief Retweet a status.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2011
 */

#include <glib/gi18n.h>

#include "retweet_dialog.h"
#include "gtkdeletabledialog.h"
#include "gtk_helpers.h"
#include "mainwindow.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _RetweetWindowPrivate
 * \brief Private data of the "retweet" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! Authenticated user. */
	gchar user[64];
	/*! Status to retweet. */
	gchar guid[32];
} _RetweetWindowPrivate;

/*
 *	apply worker:
 */
static gpointer
_retweet_dialog_apply_worker(GtkWidget *dialog)
{
	_RetweetWindowPrivate *private = (_RetweetWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	TwitterClient *client;
	GError *err = NULL;
	gint response = GTK_RESPONSE_YES;

	client = mainwindow_create_twittter_client(private->parent, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME);

	if(!twitter_client_retweet(client, private->user, private->guid, &err))
	{
		response = GTK_RESPONSE_CANCEL;
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_object_unref(client);

	gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), response);

	return NULL;
}

/*
 *	events:
 */
static void
_retweet_dialog_destroy(GtkWidget *dialog, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	g_free(g_object_get_data(G_OBJECT(dialog), "private"));
}

static gboolean
_retweet_dialog_delete(GtkDeletableDialog *dialog, GdkEvent event, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	if(gtk_widget_get_sensitive(GTK_WIDGET(dialog)))
	{
		gtk_deletable_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
	}

	return TRUE;
}

static void
_retweet_dialog_apply_clicked(GtkWidget *button, GtkWidget *dialog)
{
	GThread *thread;
	GError *err = NULL;

	/* set window busy */
	gtk_helpers_set_widget_busy(dialog, TRUE);

	/* create worker */
	thread = g_thread_create((GThreadFunc)_retweet_dialog_apply_worker, dialog, FALSE, &err);

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
retweet_dialog_create(GtkWidget *parent, const gchar *user, const gchar *guid)
{
	GtkWidget *dialog;
	_RetweetWindowPrivate *private;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *alignment;
	gchar *path;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *area;
	GtkWidget *button_apply;

	g_debug("Opening \"retweet\" dialog");

	/* create dialog */
	private = (_RetweetWindowPrivate *)g_malloc(sizeof(_RetweetWindowPrivate));
	private->parent = parent;
	g_strlcpy(private->user, user, 64);
	g_strlcpy(private->guid, guid, 32);

	dialog = gtk_deletable_dialog_new_with_buttons("Retweet", GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* main boxes */
	hbox = gtk_hbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
	gtk_container_add(GTK_CONTAINER(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog))), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* insert image */
	path = pathbuilder_build_image_path("retweet.png");
	image = gtk_image_new_from_file(path);
	g_free(path);
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

	/* insert label */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("Do you really want to retweet the status to all of your followers?"));
	gtk_container_add(GTK_CONTAINER(alignment), label);

	/* insert buttons */
	area = gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));

	button_apply = gtk_button_new_from_stock(GTK_STOCK_YES);
	gtk_box_pack_start(GTK_BOX(area), button_apply, FALSE, FALSE, 0);

	gtk_deletable_dialog_add_action_widget(GTK_DELETABLE_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_NO), GTK_RESPONSE_NO);

	/* connect signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_retweet_dialog_destroy), NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(_retweet_dialog_delete), NULL);
	g_signal_connect(button_apply, "clicked", G_CALLBACK(_retweet_dialog_apply_clicked), dialog);

	/* show widgets */
	gtk_widget_show_all(dialog);

	return dialog;
}

/**
 * @}
 */


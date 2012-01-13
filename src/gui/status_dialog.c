/***************************************************************************
    begin........: January 2012
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
 * \file status_dialog.h
 * \brief A dialog displaying tweets.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 13. January 2012
 */

#include <glib/gi18n.h>
#include "status_dialog.h"
#include "gtktwitterstatus.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _StatusDialogPrivate
 * \brief Private data of the "status" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! Box containing tweets. */
	GtkWidget *vbox;
} _StatusDialogPrivate;

/*
 *	events:
 */
static void
_status_dialog_destroy(GtkDialog *dialog, gpointer user_data)
{
	g_assert(GTK_IS_DIALOG(dialog));

	g_free(g_object_get_data(G_OBJECT(dialog), "private"));
}

/*
 *	public:
 */
GtkWidget *
status_dialog_create(GtkWidget *parent, const gchar *title)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *scrolled;
	_StatusDialogPrivate *private = (_StatusDialogPrivate *)g_malloc(sizeof(_StatusDialogPrivate));

	g_assert(GTK_IS_WINDOW(parent));

	g_debug("Opening \"status\" dialog");

	private->parent = parent;

	dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* mainbox */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);

	/* create status list */
	frame = gtk_frame_new(_("Tweets"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 2);

	private->vbox = vbox;

	/* set size & show widgets */
	gtk_widget_set_size_request(dialog, 500, 300);
	gtk_widget_show_all(dialog);

	/* signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_status_dialog_destroy), NULL);

	return dialog;
}

void
status_dialog_add_status(GtkWidget *widget, TwitterUser *user, TwitterStatus *status)
{
	_StatusDialogPrivate *private = (_StatusDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");
	GtkWidget *tweet;

	tweet = gtk_twitter_status_new();
	g_object_set(G_OBJECT(tweet),
	             "guid", status->id,
	             "username", user->screen_name,
	             "realname", user->name,
	             "timestamp", status->timestamp,
	             "status", status->text,
	             "selectable", TRUE,
	             "show-reply-button", FALSE,
	             "show-edit-lists-button", FALSE,
	             "show-edit-friendship-button", FALSE,
	             "show-retweet-button", FALSE,
	             "show-delete-button", FALSE,
	             "show-history-button", FALSE,
	             NULL);
	gtk_box_pack_start(GTK_BOX(private->vbox), tweet, FALSE, FALSE, 2);
	gtk_widget_show_all(tweet);
}

/**
 * @}
 */


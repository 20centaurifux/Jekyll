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
 * \file replies_dialog.c
 * \brief A dialog displaying tweets.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 22. January 2012
 */

#include <glib/gi18n.h>
#include "gtkdeletabledialog.h"
#include "replies_dialog.h"
#include "mainwindow.h"
#include "gtk_helpers.h"
#include "pixbuf_helpers.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _RepliesDialogPrivate
 * \brief Private data of the "status" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! Account name. */
	gchar *username;
	/*! First status to append. */
	gchar first_status[32];
	/*! Box containing tweets. */
	GtkWidget *vbox;
	/*! A cancellable. */
	GCancellable *cancellable;
	/*! Background worker thread. */
	GThread *thread;
	/*! Background worker status. */
	gboolean finished;
	/*! Mutex protecting finished. */
	GMutex *mutex;
	/*! Unique pixbuf group. */
	gchar pixbuf_group[32];
	/*! Function to call when an user has been activated. */
	RepliesDialogEventHandler user_callback;
	/*! Data passed to user callback. */
	gpointer user_data;
	/*! Function to call when an url has been activated. */
	RepliesDialogEventHandler url_callback;
	/*! Data passed to url callback. */
	gpointer url_data;
} _RepliesDialogPrivate;

/*
 *	events:
 */
static void
_replies_dialog_url_activated(GtkWidget *status, const gchar *url, GtkWidget *widget)
{
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");

	if(private->url_callback)
	{
		private->url_callback(GTK_TWITTER_STATUS(status), url, private->url_data);
	}
}

static void
_replies_dialog_username_activated(GtkWidget *status, const gchar *user, GtkWidget *widget)
{
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");

	if(private->user_callback)
	{
		private->user_callback(GTK_TWITTER_STATUS(status), user, private->user_data);
	}
}

/*
 *	helpers:
 */
static void
_replies_dialog_add_status(GtkWidget *widget, TwitterStatus status, TwitterUser user)
{
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");
	GtkWidget *tweet;

	gdk_threads_enter();
	tweet = gtk_twitter_status_new();
	g_object_set(G_OBJECT(tweet),
	             "guid", status.id,
	             "username", user.screen_name,
	             "realname", user.name,
	             "timestamp", status.timestamp,
	             "status", status.text,
	             "selectable", TRUE,
	             "show-reply-button", FALSE,
	             "show-edit-lists-button", FALSE,
	             "show-edit-friendship-button", FALSE,
	             "show-retweet-button", FALSE,
	             "show-delete-button", FALSE,
	             "show-replies-button", FALSE,
	             NULL);
	gtk_box_pack_start(GTK_BOX(private->vbox), tweet, FALSE, FALSE, 2);
	gtk_widget_show_all(tweet);
	mainwindow_load_pixbuf(private->parent, private->pixbuf_group, user.image, (PixbufLoaderCallback)pixbuf_helpers_set_gtktwitterstatus_callback, tweet, NULL);


	g_signal_connect(G_OBJECT(tweet), "url-activated", (GCallback)_replies_dialog_url_activated, widget);
	g_signal_connect(G_OBJECT(tweet), "username-activated", (GCallback)_replies_dialog_username_activated, widget);

	gdk_threads_leave();
}

static gboolean
_replies_dialog_finished(GtkWidget *widget)
{
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");
	gboolean finished;

	g_mutex_lock(private->mutex);
	finished = private->finished;
	g_mutex_unlock(private->mutex);

	return finished;
}

/*
 *	worker:
 */
static gpointer
_replies_dialog_worker(GtkWidget *widget)
{
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");
	TwitterClient *client;
	TwitterStatus status;
	TwitterUser user;
	gboolean abort = FALSE;
	gchar guid[32];
	GError *err = NULL;

	g_debug("Starting worker: %s", __func__);

	if((client = mainwindow_create_twittter_client(private->parent, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
	{
		g_strlcpy(guid, private->first_status, 32);

		while(!abort && !g_cancellable_is_cancelled(private->cancellable))
		{
			if(twitter_client_get_status(client, private->username, guid, &status, &user, &err))
			{
				_replies_dialog_add_status(widget, status, user);

				if(status.prev_status[0])
				{
					g_strlcpy(guid, status.prev_status, 32);
				}
				else
				{
					abort = TRUE;
				}
			}
			else
			{
				abort = TRUE;
			}
		}

		g_object_unref(client);
	}
	else
	{
		g_warning("Couldn't create TwitterClient instance.");
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_mutex_lock(private->mutex);
	private->finished = TRUE;
	g_mutex_unlock(private->mutex);

	g_debug("Leaving worker: %s", __func__);

	return NULL;
}

static gboolean
_replies_dialog_close_worker(GtkWidget *dialog)
{
	while(!_replies_dialog_finished(dialog))
	{
		g_usleep(150000);
	}

	gdk_threads_enter();
	gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), GTK_RESPONSE_OK);
	gdk_threads_leave();

	return FALSE;
}

static void
_replies_dialog_close(GtkWidget *dialog)
{
	_RepliesDialogPrivate *private = g_object_get_data(G_OBJECT(dialog), "private");

	mainwindow_remove_pixbuf_group(private->parent, private->pixbuf_group);
	gtk_helpers_set_widget_busy(dialog, TRUE);
	g_cancellable_cancel(private->cancellable);
	g_idle_add((GSourceFunc)_replies_dialog_close_worker, dialog);

}

/*
 *	events:
 */
static void
_replies_dialog_destroy(GtkDialog *dialog, gpointer user_data)
{
	_RepliesDialogPrivate *private = g_object_get_data(G_OBJECT(dialog), "private");

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	g_free(private->username);
	g_mutex_free(private->mutex);
	g_object_unref(private->cancellable);
	g_free(private);
}

static gboolean
_replies_dialog_delete(GtkDeletableDialog *dialog, GdkEvent event, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	if(gtk_widget_get_sensitive(GTK_WIDGET(dialog)))
	{
		_replies_dialog_close(GTK_WIDGET(dialog));
	}

	return TRUE;
}

static void
_replies_dialog_button_ok_clicked(GtkWidget *button, GtkWidget *dialog)
{
	_replies_dialog_close(dialog);
}

/*
 *	public:
 */
GtkWidget *
replies_dialog_create(GtkWidget *parent, const gchar * restrict account, const gchar * restrict first_status)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *scrolled;
	GtkWidget *area;
	GtkWidget *button_ok;
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_malloc(sizeof(_RepliesDialogPrivate));
	static guint id = 0;

	g_assert(GTK_IS_WINDOW(parent));

	g_debug("Opening \"status\" dialog");

	private->parent = parent;
	private->cancellable = g_cancellable_new();
	private->finished = FALSE;
	private->mutex = g_mutex_new();
	private->username = g_strdup(account);
	g_strlcpy(private->first_status, first_status, 32);
	sprintf(private->pixbuf_group, "replies-%d", id);

	dialog = gtk_deletable_dialog_new_with_buttons(_("Replies"), GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* mainbox */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog))), hbox);

	/* create status list */
	frame = gtk_frame_new(_("Replies"));
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

	/* ok button */
	area = gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));

	button_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_start(GTK_BOX(area), button_ok, FALSE, FALSE, 0);

	/* set size & show widgets */
	gtk_widget_set_size_request(dialog, 500, 300);
	gtk_widget_show_all(dialog);

	/* signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_replies_dialog_destroy), NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(_replies_dialog_delete), NULL);
	g_signal_connect(button_ok, "clicked", G_CALLBACK(_replies_dialog_button_ok_clicked), dialog);

	/* update id */
	if(id != G_MAXUINT)
	{
		++id;
	}
	else
	{
		id = 0;
	}

	return dialog;
}

void
replies_dialog_run(GtkWidget *widget)
{
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");
	GError *err = NULL;

	g_assert(GTK_IS_DELETABLE_DIALOG(widget));

	private->thread = g_thread_create((GThreadFunc)_replies_dialog_worker, widget, TRUE, &err);

	if(err)
	{
		g_warning("%s\n", err->message);
		g_error_free(err);
	}

	g_assert(private->thread != NULL);

	gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(widget));
}

void
replies_dialog_set_user_handler(GtkWidget *widget, RepliesDialogEventHandler callback, gpointer user_data)
{
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");

	g_assert(GTK_IS_DELETABLE_DIALOG(widget));

	private->user_callback = callback;
	private->user_data = user_data;
}

void
replies_dialog_set_url_handler(GtkWidget *widget, RepliesDialogEventHandler callback, gpointer user_data)
{
	_RepliesDialogPrivate *private = (_RepliesDialogPrivate *)g_object_get_data(G_OBJECT(widget), "private");

	g_assert(GTK_IS_DELETABLE_DIALOG(widget));

	private->url_callback = callback;
	private->url_data = user_data;
}

/**
 * @}
 */


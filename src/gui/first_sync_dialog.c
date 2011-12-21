/***************************************************************************
    begin........: August 2010
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
 * \file first_sync_dialog.c
 * \brief Dialog shown when doing the initial synchronization.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. September 2011
 */

#include <glib/gi18n.h>
#include <string.h>

#include "first_sync_dialog.h"
#include "../pathbuilder.h"
#include "../application.h"
#include "../twittersync.h"
#include "../twitterdb.h"
#include "../twitterxmlparser.h"

/**
 * \struct _FirstSyncPrivate
 * \brief Private data of the first synchronization window.
 */
typedef struct
{
	/*! A progressbar. */
	GtkWidget *progressbar;
	/*! Label displaying status information. */
	GtkWidget *label_status;
	/*! User to synchronize. */
	const gchar *username;
	/*! OAuth access key. */
	const gchar *access_key;
	/*! OAuth access secret. */
	const gchar *access_secret;
	/*! Status information. */
	gboolean success;
} _FirstSyncPrivate;

/*
 *	events:
 */
static gboolean
_first_sync_dialog_delete(GtkWidget *widget, GdkEvent event, gpointer data)
{
	return TRUE;
}

static void
_first_sync_dialog_destroy(GtkWidget *widget, gpointer data)
{
	g_free(g_object_get_data(G_OBJECT(widget), "private"));
}

/*
 *	helpers:
 */
static void
_first_sync_dialog_update_status(GtkWidget *dialog, const gchar *text)
{
	_FirstSyncPrivate *private = (_FirstSyncPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	gchar *escaped;
	gchar *label;

	escaped = g_markup_escape_text(text, -1);
	label = g_strdup_printf("<span size=\"small\" style=\"italic\">%s</span>", escaped);
	gtk_label_set_markup(GTK_LABEL(private->label_status), label);
	g_free(label);
	g_free(escaped);
}

static void
_first_sync_copy_user(TwitterUser user, gpointer destination)
{
	memcpy(destination, &user, sizeof(TwitterUser));
}

/*
 *	worker:
 */
static gpointer
_first_sync_worker(GtkWidget *dialog)
{
	_FirstSyncPrivate *private = (_FirstSyncPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	TwitterWebClient *client = NULL;
	TwitterDbHandle *handle;
	gint count;
	TwitterUser user;
	gchar *buffer = NULL;
	gint length;
	GTimeVal now;
	GError *err = NULL;

	private->success = FALSE;

	/* connect to database & synchronize account */
	if((handle = twitterdb_get_handle(&err)))
	{
		/* create web client */
		client = twitter_web_client_new();
		twitter_web_client_set_username(client, private->username);
		twitter_web_client_set_oauth_authorization(client, OAUTH_CONSUMER_KEY, OAUTH_CONSUMER_SECRET, private->access_key, private->access_secret);
		twitter_web_client_set_format(client, "xml");
		g_object_set(G_OBJECT(client), "status-count", TWITTER_MAX_STATUS_COUNT, NULL);

		/* fetch user details */
		gdk_threads_enter();
		_first_sync_dialog_update_status(dialog, _("Fetching user details..."));
		gdk_threads_leave();

		if(twitter_web_client_get_user_details(client, private->username, &buffer, &length))
		{
			memset(&user, 0, sizeof(TwitterUser));
			twitter_xml_parse_user_details(buffer, length, _first_sync_copy_user, &user);

			if(user.id[0])
			{
				if(twitterdb_save_user(handle, user.id, user.screen_name, user.name, user.image, user.location, user.url, user.description, &err))
				{
					private->success = TRUE;
				}
				else
				{
					g_warning("Couldn't write user \"%s\" (%s) to database", user.name, user.id);
				}
			}
		}
		else
		{
			g_warning("Couldn't receive user details from Twitter service");
		}

		/* synchronize timelines */
		if(private->success)
		{
			gdk_threads_enter();
			_first_sync_dialog_update_status(dialog, _("Synchronizing timelines..."));
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(private->progressbar), 0.1), 
			gdk_threads_leave();

			private->success = twittersync_update_timelines(handle, client, &count, NULL, &err);
		}

		/* synchronize lists */
		if(private->success)
		{
			gdk_threads_enter();
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(private->progressbar), 0.4), 
			_first_sync_dialog_update_status(dialog, _("Synchronizing lists..."));
			gdk_threads_leave();

			private->success = twittersync_update_lists(handle, client, &count, TRUE, NULL, &err);
		}

		/* synchronize direct messages */
		/*
		if(private->success)
		{
			gdk_threads_enter();
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(private->progressbar), 0.6), 
			_first_sync_dialog_update_status(dialog, "Synchronizing direct messages...");
			gdk_threads_leave();

			private->success = twittersync_update_direct_messages(handle, client, &count, &err);
		}
		*/

		/* synchronize followers */
		if(private->success)
		{
			gdk_threads_enter();
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(private->progressbar), 0.7), 
			_first_sync_dialog_update_status(dialog, _("Synchronizing followers..."));
			gdk_threads_leave();

			private->success = twittersync_update_friends(handle, client, NULL, &err);

			gdk_threads_enter();
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(private->progressbar), 0.9), 
			gdk_threads_leave();

			private->success = twittersync_update_followers(handle, client, NULL, &err);
		}

		/* update last synchronization timestamps */
		if(private->success)
		{
			g_get_current_time(&now);
			twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_TIMELINES, user.id, now.tv_sec);
			twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_LISTS, user.id, now.tv_sec);
			twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_LIST_MEMBERS, user.id, now.tv_sec);
			//twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_DIRECT_MESSAGES, user.id, now.tv_sec);
			twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_FRIENDS, user.id, now.tv_sec);
			twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_FOLLOWERS, user.id, now.tv_sec);
		}

		/* close window  */
		gtk_main_quit();
	}

	/* cleanup */
	if(client)
	{
		g_object_unref(client);
	}

	g_free(buffer);
	twitterdb_close_handle(handle);

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	return NULL;
}

/*
 *	public:
 */
gboolean
first_sync_dialog_run(GtkWidget *parent, const gchar *username, const gchar *access_key, gchar *access_secret)
{
	GtkWidget *window;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	gchar *path;
	GtkWidget *image;
	gchar *text;
	GtkWidget *label_title;
	GtkWidget *progressbar;
	GtkWidget *align;
	GtkWidget *label_status;
	#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 20
	GtkWidget *spinner;
	#endif
	GThread *thread;
	GError *err = NULL;
	_FirstSyncPrivate *private;

	/* create window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	#ifdef G_OS_WIN32
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
	#endif

	gtk_window_set_title(GTK_WINDOW(window), _("First synchronization"));
	gtk_window_set_deletable(GTK_WINDOW(window), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	if(parent)
	{
		gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));
		gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	}

	/* create main boxes & frame */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 2);

	frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 2);

	/* create boxes & image */
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	path = pathbuilder_build_image_path("network.png");
	image = gtk_image_new_from_file(path);
	g_free(path);
	gtk_box_pack_start(GTK_BOX(hbox), image, TRUE, TRUE, 2);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 8);

	/* create title */
	label_title = gtk_label_new(NULL);
	text = g_strdup_printf(_("Please stay patient, while <b>%s</b> synchronizes your account for the very first time."), APPLICATION_NAME);
	gtk_label_set_markup(GTK_LABEL(label_title), text);
	g_free(text);
	gtk_label_set_line_wrap(GTK_LABEL(label_title), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label_title, FALSE, FALSE, 2);

	/* create progressbar */
	progressbar = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), progressbar, FALSE, FALSE, 2);

	/* create spinner & status label */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);

	#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 20
	spinner = gtk_spinner_new();
	gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 2);
	gtk_spinner_start(GTK_SPINNER(spinner));
	#endif

	align = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, FALSE, 2);

	label_status = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(align), label_status);

	/* show widgets */
	gtk_widget_show_all(window);
	gtk_window_present(GTK_WINDOW(window));

	/* create & assign private data */
	private = (_FirstSyncPrivate *)g_malloc(sizeof(_FirstSyncPrivate));
	private->progressbar = progressbar;
	private->label_status = label_status;
	private->username = username;
	private->access_key = access_key;
	private->access_secret = access_secret;
	private->success = TRUE;
	g_object_set_data(G_OBJECT(window), "private", private);

	/* signals */
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(_first_sync_dialog_destroy), NULL);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(_first_sync_dialog_delete), NULL);

	/* initialize status information */
	_first_sync_dialog_update_status(window, "Starting synchronization...");

	/* start worker */
	thread = g_thread_create((GThreadFunc)_first_sync_worker, window, FALSE, &err);

	if(err)
	{
		g_warning("%s\n", err->message);
		g_error_free(err);
	}

	g_assert(thread != NULL);

	/* run window */
	gtk_main();

	/* destroy window */
	gtk_widget_destroy(window);

	return private->success;
}


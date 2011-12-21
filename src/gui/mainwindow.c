/***************************************************************************
    begin........: May 2010
    copyright....: Sebastian Fedrau
    email........: lord-kefir@arcor.de
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 ***************************************************************************/
/*!
 * \file mainwindow.c
 * \brief The mainwindow.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2011
 */

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <glib/gi18n.h>

#include "mainwindow.h"
#include "systray.h"
#include "statusbar.h"
#include "tabbar.h"
#include "about_dialog.h"
#include "preferences_dialog.h"
#include "accounts_dialog.h"
#include "list_preferences_dialog.h"
#include "remove_list_dialog.h"
#include "add_list_dialog.h"
#include "edit_members_dialog.h"
#include "composer_dialog.h"
#include "gtk_helpers.h"
#include "../listener.h"
#include "../application.h"
#include "../pathbuilder.h"
#include "../twittersync.h"
#include "../twitterdb.h"
#include "../twitterclient_factory.h"
#include "../net/twitterwebclient.h"
#include "../twitterxmlparser.h"

/**
 * @addtogroup Gui
 * @{
 */

/*! Get mainwindow meta data. */
#define MAINWINDOW_GET_DATA(w) (_MainWindowPrivate *)g_object_get_data(G_OBJECT(w), "private")

/**
 * \typedef MainMenuAction
 * \brief A collection of menu actions.
 */
typedef enum
{
	/*! Synchronize data and GUI. */
	MAINMENU_ACTION_REFRESH,
	/*! Quit program. */
	MAINMENU_ACTION_QUIT,
	/*! Write new tweet. */
	MAINMENU_ACTION_COMPOSE_TWEET,
	/*! Open "preferences" dialog. */
	MAINMENU_ACTION_PREFERENCES,
	/*! Open the "accounts" dialog. */
	MAINMENU_ACTION_PREFERENCES_ACCOUNTS,
	/*! Open the "about" dialog. */
	MAINMENU_ACTION_ABOUT
} MainMenuAction;;

/**
 * \struct _MainWindowMenuData
 * \brief Holds menu data.
 */
typedef struct
{
	/*! Pointer to the mainwindow. */
	GtkWidget *window;
	/*! The performed action. */
	MainMenuAction action;
} _MainWindowMenuData;

/*!
 * \typedef _MainWindowSyncEvent
 * \brief Synchronization events.
 */
typedef enum
{
	/*! Emitted to start the synchronization. */
	MAINWINDOW_SYNC_EVENT_START_SYNC = 1,
	/*! Emitted to continue the synchronization. */
	MAINWINDOW_SYNC_EVENT_CONTINUE = 2,
	/*! Emitted to abort the synchronization and stop the synchronization thread. */
	MAINWINDOW_SYNC_EVENT_ABORT = 3,
	/*! Emitted to synchronize lists. */
	MAINWINDOW_SYNC_EVENT_UPDATE_LISTS = 4,
	/*! Emitted to synchronize timelines. */
	MAINWINDOW_SYNC_EVENT_UPDATE_TIMELINES = 5,
	/*! Emitted to stop the synchronization. */
	MAINWINDOW_SYNC_EVENT_UPDATE_GUI = 6,
} _MainWindowSyncEvent;

/*!
 * \typedef _MainWindowSyncStep;
 * \brief Synchronization co-procedure steps.
 */
typedef enum
{
	/*! Displays an update notification. */
	MAINWINDOW_SYNC_STEP_NOTICY_TIMELINES,
	/*! Updates the main timelines (home, replies, mentions). */
	MAINWINDOW_SYNC_STEP_TIMELINES,
	/*! Displays an update notification. */
	MAINWINDOW_SYNC_STEP_NOTICY_LISTS,
	/*! Updates list members & list timelines. */
	MAINWINDOW_SYNC_STEP_LISTS,
	/*! Displays an update notification. */
	MAINWINDOW_SYNC_STEP_NOTICY_DIRECT_MESSAGES,
	/*! Updates direct messages. */
	MAINWINDOW_SYNC_STEP_DIRECT_MESSAGES,
	/*! Refreshs the GUI. */
	MAINWINDOW_SYNC_STEP_GUI,
	/*! Idle. */
	MAINWINDOW_SYNC_STEP_GUI_IDLE,
	/*! Displays an update notification. */
	MAINWINDOW_SYNC_STEP_NOTICY_FOLLOWERS,
	/*! Updates followers. */
	MAINWINDOW_SYNC_STEP_FOLLOWERS,
	/*! Last synchronization id. */
	MAINWINDOW_SYNC_STEP_LAST
} _MainWindowSyncStep;

/**
 * \struct _MainWindowNotification
 * \brief Holds notification data.
 */
typedef struct
{
	/*! Pointer to the mainwindow. */
	GtkWidget *window;
	/*! The notification level. */
	NotificationLevel level;
	/*! The message. */
	gchar *message;
} _MainWindowNotification;

/**
 * \struct _MainwindowEditFollowersWorker
 * \brief Data required for the "members dialog" worker.
 */
typedef struct
{
	/*! Mainwindow. */
	GtkWidget *parent;
	/*! "Edit members" dialog. */
	GtkWidget *dialog;
	/*! Name of the user account. */
	gchar account[64];
	/*! Friends flag. */
	gboolean edit_friends;
}
_MainwindowEditFollowersWorker;

/*! Callback function to process account data. */
typedef void (* _MainWindowProcessAccountFunc)(GtkWidget *widget, const gchar *username, const gchar *access_key, const gchar *access_secret);

/**
 * \struct _MainWindowPrivate
 * \brief Private data of the mainwindow.
 */
typedef struct
{
	/**
	 * \struct _config
	 * \brief Holds _Config object and related mutex.
	 *
	 * \var config
	 * \brief Configuration object and related mutex.
	 */
	struct _config
	{
		/*! Pointer to the configuration. */
		Config *config;
		/*! Mutex to protect the configuration. */
		GMutex *mutex;
	} config;
	/*! Pointer to the cache. */
	Cache *cache;
	/*! Indicates if the window has been shown already. */
	gboolean visibility_set;
	/*! The main box.*/
	GtkWidget *vbox;
	/*! Box containing the accounts tree. */
	GtkWidget *left;
	/*! Box containing tabs & notifications. */
	GtkWidget *right;
	/*! The mainmenu. */
	GtkWidget *menu;
	/*! The systray widget. */
	GtkStatusIcon *systray;
	/*! A paned widget. */
	GtkWidget *paned_tree;
	/*! "accountbrowser" widget. */
	GtkWidget *accountbrowser;
	/*! A notebook containing tabs. */
	GtkWidget *tabbar;
	/*! A statusbar. */
	GtkWidget *statusbar;
	/*! Box holding the notification area. */
	GtkWidget *notification_box;
	/*! A notification area. */
	GtkWidget *notification_area;
	/*! A paned widget.*/
	GtkWidget *paned_notifications;
	/*! Log handler id. */
	guint log_handler;
	/*! Specifies if debugging has been activated. */
	gboolean enable_debug;
	/*! Specifies if synchronization has been deactivated. */
	gboolean no_sync;
	/**
	 * \struct _notification_level
	 * \brief Holds notification level and related mutex.
	 *
	 * \var notification_level
	 * \brief Notification level and related mutex.
	 */
	struct _notification_level
	{
		/*! The notification level. */
		gint level;
		/*! Mutex to protect the notification level. */
		GMutex *mutex;
	} notification_level;
	/**
	 * \struct _sync
	 * \brief Holds synchronization thread and related message queue.
	 *
	 * \var sync
	 * \brief Synchronization thread and related message queue.
	 */
	struct _sync
	{
		/*! A background worker. */
		GThread *thread;
		/*! Queue to send messages to background worker. */
		GAsyncQueue *queue;

	} sync;
	/*! A background image loader. */
	PixbufLoader *pixbuf_loader;
	/*! A cancellable. */
	GCancellable *cancellable;
	/**
	 * \struct _last_gui_sync
	 * \brief Holds time of the last GUI synchronization.
	 *
	 * \var last_gui_sync
	 * \brief Time of the last GUI synchronization.
	 */
	struct _last_gui_sync
	{
		/*! Time of the last synchronization. */
		GTimeVal time;
		/*! A mutex. */
		GMutex *mutex;
	} last_gui_sync;
	/**
	 * \struct _sync_state
	 * \brief Holds state of the synchronization "co-procedure".
	 *
	 * \var sync_state
	 * \brief State of the synchronization "co-procedure".
	 */
	struct _sync_state
	{
		/*! Current synchronization status. */
		gint state;
		/*! Mutex protecting sync_state. */
		GMutex *mutex;
	} sync_state;
	/**
	 * \struct _accountbrowser_menues
	 * \brief Holds accountbrowser menues.
	 *
	 * \var accountbrowser_menues
	 * \brief Accountbrowser menues.
	 */
	struct _accountbrowser_menues
	{
		/*! The selected account. */
		gchar account[64];
		/*! The selected text. */
		gchar text[64];
		/*! "lists" popup menu. */
		GtkWidget *menu_lists;
		/*! "list" popup menu. */
		GtkWidget *menu_list[2];
	} accountbrowser_menues;
} _MainWindowPrivate;

/*
 *	log handler:
 */
static gboolean
_mainwindow_notification_worker(_MainWindowNotification *notification)
{
	gdk_threads_enter();
	mainwindow_notify(notification->window, notification->level, notification->message);
	gdk_threads_leave();

	g_free(notification->message);
	g_slice_free1(sizeof(_MainWindowNotification), notification);

	return FALSE;
}

static void
_mainwindow_log_handler(const gchar *domain, GLogLevelFlags level, const gchar *message, GtkWidget *mainwindow)
{
	_MainWindowPrivate *private;
	NotificationLevel notification_level = NOTIFICATION_LEVEL_INFO;
	gint verbosity;
	gboolean show_notification = TRUE;
	_MainWindowNotification *notification;

	private = MAINWINDOW_GET_DATA(mainwindow);

	if(level != G_LOG_LEVEL_DEBUG || private->enable_debug)
	{
		g_log_default_handler(domain, level, message, NULL);
	}

	g_mutex_lock(private->notification_level.mutex);
	verbosity = private->notification_level.level;
	g_mutex_unlock(private->notification_level.mutex);

	if(level == G_LOG_LEVEL_DEBUG && verbosity == 2)
	{
		notification_level = NOTIFICATION_LEVEL_DEBUG;
	}
	else if(level == G_LOG_LEVEL_INFO && verbosity > 0)
	{
		notification_level = NOTIFICATION_LEVEL_INFO;
	}
	else if(level == G_LOG_LEVEL_WARNING)
	{
		notification_level = NOTIFICATION_LEVEL_WARNING;
	}
	else
	{
		show_notification = FALSE;
	}

	if(show_notification)
	{
		notification = (_MainWindowNotification *)g_slice_alloc(sizeof(_MainWindowNotification));
		notification->level = notification_level;
		notification->message = g_strdup(message);
		notification->window = mainwindow;
		g_idle_add((GSourceFunc)_mainwindow_notification_worker, notification);
	}
}

static void
_mainwindow_set_log_handler(GtkWidget *mainwindow)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(mainwindow);

	private->log_handler = g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO | G_LOG_LEVEL_WARNING, (GLogFunc)_mainwindow_log_handler, mainwindow);
}

/*
 *	listener event handler:
 */
static gboolean
_mainwindow_show_worker(GtkWidget *mainwindow)
{
	gdk_threads_enter();
	mainwindow_show(mainwindow);
	gdk_threads_leave();

	return FALSE;
}

static void
_mainwindow_handle_listener_request(gint code, const gchar *text, GtkWidget *mainwindow)
{
	g_assert(GTK_IS_WINDOW(mainwindow));

	if(code == LISTENER_MESSAGE_ACTIVATE_INSTANCE)
	{
		g_debug("%s: showing mainwindow", __func__);
		g_idle_add((GSourceFunc)_mainwindow_show_worker, mainwindow);
	}
}

/*
 *	populate data:
 */
static void
_mainwindow_populate_lists_from_account(GtkWidget *accountbrowser, const gchar *username)
{
	TwitterDbHandle *handle;
	gchar guid[32];
	TwitterUser user;
	GList *lists;
	GError *err = NULL;

	if((handle = twitterdb_get_handle(&err)))
	{
		if(twitterdb_map_username(handle, username, guid, 32, &err))
		{
			if((lists = twitterdb_get_lists(handle, guid, &user, &err)))
			{
				accountbrowser_set_lists(accountbrowser, user.screen_name, lists);
				twitterdb_free_get_lists_result(lists);
			}
		}

		twitterdb_close_handle(handle);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}
}

static void
_mainwindow_append_account_to_list(Section *section, GList **accounts)
{
	Value *value;

	g_assert(section != NULL);

	if(!g_ascii_strcasecmp(section_get_name(section), "Account"))
	{
		if((value = section_find_first_value(section, "username")))
		{
			if(VALUE_IS_STRING(value))
			{
				*accounts = g_list_append(*accounts, g_strdup(value_get_string(value)));
			}
		}
	}
}

static void
_mainwindow_populate(GtkWidget *widget)
{
	_MainWindowPrivate *private;
	Section *section;
	GList *accounts = NULL;
	GList *old_accounts;
	GList *iter;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	/* get accounts from configuration */
	section = config_get_root(private->config.config);
	if((section = section_find_first_child(section, "Twitter")))
	{
		if((section = section_find_first_child(section, "Accounts")))
		{
			section_foreach_child(section, (ForEachSectionFunc)_mainwindow_append_account_to_list, &accounts);
		}
	}

	if(accounts)
	{
		/* remove deleted accounts */
		if((iter = old_accounts = accountbrowser_get_accounts(private->accountbrowser)))
		{
			while(iter)
			{
				if(!g_list_find_custom(accounts, iter->data, (GCompareFunc)g_strcmp0))
				{
					accountbrowser_remove_account(private->accountbrowser, (gchar *)iter->data);
				}
				iter = iter->next;
			}

			g_list_foreach(old_accounts, (GFunc)g_free, NULL);
			g_list_free(old_accounts);
		}

		/* append accounts to accountbrowser */
		iter = accounts;

		while(iter)
		{
			accountbrowser_append_account(private->accountbrowser, (gchar *)iter->data);
			_mainwindow_populate_lists_from_account(private->accountbrowser, (gchar *)iter->data);
			iter = iter->next;
		}

		g_list_foreach(accounts, (GFunc)g_free, NULL);
		g_list_free(accounts);
	}
}

/*
 *	systray icon:
 */
static void
_mainwindow_show_systray_icon(GtkWidget *widget, gboolean show)
{
	_MainWindowPrivate *private;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	if(show && !private->systray)
	{
		private->systray = systray_get_instance(widget);
	}

	if(private->systray)
	{
		systray_set_visible(private->systray, show);
	}
}

/*
 *	synchronization:
 */
static void
_mainwindow_send_sync_message(GtkWidget *widget, _MainWindowSyncEvent event)
{
	GAsyncQueue *queue;

	g_assert(GTK_IS_WINDOW(widget));

	queue = (MAINWINDOW_GET_DATA(widget))->sync.queue;
	g_async_queue_push(queue, GINT_TO_POINTER(event));
}

static gint
_mainwindow_sync_get_accounts(GtkWidget *widget, gchar ***usernames, gchar ***access_keys, gchar ***access_secrets)
{
	Config *config;
	Section *section;
	Section *child;
	Value *value;
	gint count;
	gint result = 0;

	/* get root section from configuration */
	config = mainwindow_lock_config(widget);
	section = config_get_root(config);

	/* find "Accounts" section */
	if((section = section_find_first_child(section, "Twitter")) && (section = section_find_first_child(section, "Accounts")))
	{
		count = section_n_children(section);

		for(gint i = 0; i < count; ++i)
		{
			child = section_nth_child(section, i);

			if(!g_ascii_strcasecmp(section_get_name(child), "Account"))
			{
				++result;
			
				*usernames = (gchar **)g_realloc(*usernames, sizeof(gchar **) * result);

				if(access_keys)
				{
					*access_keys = (gchar **)g_realloc(*access_keys, sizeof(gchar **) * result);
				}

				if(access_secrets)
				{
					*access_secrets = (gchar **)g_realloc(*access_secrets, sizeof(gchar **) * result);
				}

				if((value = section_find_first_value(child, "username")) && VALUE_IS_STRING(value))
				{
					(*usernames)[result - 1] = g_strdup(value_get_string(value));
				}
				else
				{
					(*usernames)[result - 1] = NULL;
				}

				if(access_keys)
				{
					if((value = section_find_first_value(child, "oauth_access_key")) && VALUE_IS_STRING(value))
					{
						(*access_keys)[result - 1] = g_strdup(value_get_string(value));
					}
					else
					{
						(*access_keys)[result - 1] = NULL;
					}
				}

				if(access_secrets)
				{
					if((value = section_find_first_value(child, "oauth_access_secret")) && VALUE_IS_STRING(value))
					{
						(*access_secrets)[result - 1] = g_strdup(value_get_string(value));
					}
					else
					{
						(*access_secrets)[result - 1] = NULL;
					}
				}
			}
		}
	}

	/* unlock configuration */
	mainwindow_unlock_config(widget);

	return result;
}

static void
_mainwindow_sync_foreach_account(GtkWidget *widget, _MainWindowProcessAccountFunc func)
{
	gchar **usernames = NULL;
	gchar **access_keys = NULL;
	gchar **access_secrets = NULL;
	gint count;

	g_assert(func != NULL);

	count = _mainwindow_sync_get_accounts(widget, &usernames, &access_keys, &access_secrets);

	for(gint i = 0; i < count; ++i)
	{
		func(widget, usernames[i], access_keys[i], access_secrets[i]);
		g_free(usernames[i]);
		g_free(access_keys[i]);
		g_free(access_secrets[i]);
	}

	g_free(usernames);
	g_free(access_keys);
	g_free(access_secrets);
}

static TwitterWebClient *
_mainwindow_sync_create_twitter_client(const gchar *username, const gchar *access_key, const gchar *access_secret)
{
	TwitterWebClient *client;

	client = twitter_web_client_new();
	twitter_web_client_set_username(client, username);
	twitter_web_client_set_oauth_authorization(client, OAUTH_CONSUMER_KEY, OAUTH_CONSUMER_SECRET, access_key, access_secret);
	twitter_web_client_set_format(client, "xml");
	g_object_set(G_OBJECT(client), "status-count", MAINWINDOW_DEFAULT_STATUS_COUNT, NULL);

	return client;
}

static gint
_mainwindow_sync_get_time_diff(GTimeVal last_sync)
{
	GTimeVal now;
	gint diff = 0;

	if(last_sync.tv_sec)
	{
		g_get_current_time(&now);
		diff = now.tv_sec - last_sync.tv_sec;
	}

	return diff;
}

static gint
_mainwindow_sync_get_diff(gint seconds)
{
	GTimeVal now;
	gint diff = 0;

	if(seconds)
	{
		g_get_current_time(&now);
		diff = now.tv_sec - seconds;
	}

	return diff;
}

static void
_mainwindow_sync_copy_user(TwitterUser user, gpointer destination)
{
	memcpy(destination, &user, sizeof(TwitterUser));
}

static gboolean
_mainwindow_sync_map_username(TwitterDbHandle *handle, const gchar *username, const gchar *access_key, const gchar *access_secret, gchar guid[], gint size, GError **err)
{
	TwitterWebClient *client;
	gchar *buffer = NULL;
	gint length;
	TwitterUser user;
	gboolean result = FALSE;

	g_debug("Mapping username:  \"%s\"", username);

	/* find user in database */
	if(!(result = twitterdb_map_username(handle, username, guid, size, err)))
	{
		/* get user from Twitter service */
		if(!*err && (client = _mainwindow_sync_create_twitter_client(username, access_key, access_secret)))
		{
			if(twitter_web_client_get_user_details(client, username, &buffer, &length))
			{
				/* parse user information */
				twitter_xml_parse_user_details(buffer, length, _mainwindow_sync_copy_user, &user);

				if(user.id[0])
				{
					/* write user details to database  */
					g_debug("Got user details from Twitter service: \"%s\" (%s)", user.name, user.id);
					if(twitterdb_save_user(handle, user.id, user.screen_name, user.name, user.image, user.location, user.url, user.description, err))
					{
						/* copy guid */
						g_strlcpy(guid, user.id, size);
						result = TRUE;
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

			g_object_unref(client);
		}
	}

	if(result)
	{
		g_debug("Username \"%s"" mapped successfully => \"%s\"", username, guid);
	}
	else
	{
		g_warning("Couldn't map username: \"%s\"", username);
	}

	/* cleanup */
	g_free(buffer);

	return result;
}

static void
_mainwindow_sync_timelines(GtkWidget *widget, const gchar *username, const gchar *access_key, const gchar *access_secret)
{
	_MainWindowPrivate *private;
	TwitterDbHandle *handle;
	gchar user_guid[32];
	TwitterWebClient *client;
	GError *err = NULL;
	gint count;
	gint last_sync;
	GTimeVal now;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	if((handle = twitterdb_get_handle(&err)))
	{
		if(_mainwindow_sync_map_username(handle, username, access_key, access_secret, user_guid, 32, &err))
		{
			/* get last synchronization timestamp */
			last_sync = twitterdb_get_last_sync(handle, TWITTERDB_SYNC_SOURCE_TIMELINES, user_guid, 0);

			/* test difference */
			if((_mainwindow_sync_get_diff(last_sync) > MAINWINDOW_TIMELINE_SYNC_MIN) || !last_sync)
			{
				/* synchronize timelines */
				g_debug("Synchronizing timelines (username=\"%s\")", username);

				client = _mainwindow_sync_create_twitter_client(username, access_key, access_secret);

				/*
				 * If this is the first timeline synchronization we fetch as many tweets as possible.
				 * This will reduce the amount of API calls when updating friendship information because
				 * we don't have to get the user details for each received follower id.
				 */
				if(!last_sync)
				{
					g_object_set(G_OBJECT(client), "status-count", TWITTER_MAX_STATUS_COUNT, NULL);
				}

				/* update last synchronization timestamp */
				if(twittersync_update_timelines(handle, client, &count, private->cancellable, &err))
				{
					g_get_current_time(&now);
					twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_TIMELINES, user_guid, now.tv_sec);
				}
				else
				{
					g_warning("Couldn't synchronize timelines of user \"%s\"", username);
				}

				g_object_unref(client);
			}
		}

		twitterdb_close_handle(handle);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}
}

static void
_mainwindow_sync_lists(GtkWidget *widget, const gchar *username, const gchar *access_key, const gchar *access_secret)
{
	_MainWindowPrivate *private;
	TwitterDbHandle *handle;
	TwitterWebClient *client;
	GError *err = NULL;
	gchar user_guid[32];
	gint count;
	gint last_sync;
	gboolean sync_members = FALSE;
	GTimeVal now;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	if((handle = twitterdb_get_handle(&err)))
	{
		if(_mainwindow_sync_map_username(handle, username, access_key, access_secret, user_guid, 32, &err))
		{
			/* get last synchronization timestamp */
			last_sync = twitterdb_get_last_sync(handle, TWITTERDB_SYNC_SOURCE_LISTS, user_guid, 0);

			/* test difference */
			if((_mainwindow_sync_get_diff(last_sync) > MAINWINDOW_LIST_SYNC_MIN) || !last_sync)
			{
				last_sync = twitterdb_get_last_sync(handle, TWITTERDB_SYNC_SOURCE_LIST_MEMBERS, user_guid, 0);

				if((_mainwindow_sync_get_diff(last_sync) > MAINWINDOW_LIST_MEMBERS_SYNC_MIN) || !last_sync)
				{
					sync_members = TRUE;
				}

				/* synchronize lists */
				g_debug("Synchronizing lists (username=\"%s\")", username);

				client = _mainwindow_sync_create_twitter_client(username, access_key, access_secret);

				if(twittersync_update_lists(handle, client, &count, sync_members, private->cancellable, &err))
				{
					g_get_current_time(&now);
					twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_LISTS, user_guid, now.tv_sec);

					if(sync_members)
					{
						twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_LIST_MEMBERS, user_guid, now.tv_sec);
					}
				}
				else
				{
					g_warning("Couldn't synchronize lists of user \"%s\"", username);
				}

				g_object_unref(client);
			}
		}

		twitterdb_close_handle(handle);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
		err = NULL;
	}
}

static void
_mainwindow_sync_followers(GtkWidget *widget, const gchar *username, const gchar *access_key, const gchar *access_secret)
{
	_MainWindowPrivate *private;
	TwitterDbHandle *handle;
	TwitterWebClient *client;
	GError *err = NULL;
	gchar user_guid[32];
	gint last_sync;
	GTimeVal now;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	if((handle = twitterdb_get_handle(&err)))
	{
		if(_mainwindow_sync_map_username(handle, username, access_key, access_secret, user_guid, 32, &err))
		{
			client = _mainwindow_sync_create_twitter_client(username, access_key, access_secret);

			/*
			 *	friends:
			 */

			/* get last synchronization timestamp */
			last_sync = twitterdb_get_last_sync(handle, TWITTERDB_SYNC_SOURCE_FRIENDS, user_guid, 0);

			/* test difference */
			if((_mainwindow_sync_get_diff(last_sync) > MAINWINDOW_FRIENDS_SYNC_MIN) || !last_sync)
			{
				/* synchronize friends */
				g_debug("Synchronizing friends (username=\"%s\")", username);

				if(twittersync_update_friends(handle, client, private->cancellable, &err))
				{
					g_get_current_time(&now);
					twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_FRIENDS, user_guid, now.tv_sec);
				}
				else
				{
					g_warning("Couldn't synchronize friends of user \"%s\"", username);
				}
			}

			/*
			 *	followers:
			 */

			/* get last synchronization timestamp */
			last_sync = twitterdb_get_last_sync(handle, TWITTERDB_SYNC_SOURCE_FOLLOWERS, user_guid, 0);

			/* test difference */
			if((_mainwindow_sync_get_diff(last_sync) > MAINWINDOW_FOLLOWERS_SYNC_MIN) || !last_sync)
			{
				/* synchronize friends */
				g_debug("Synchronizing followers (username=\"%s\")", username);

				if(twittersync_update_followers(handle, client, private->cancellable, &err))
				{
					g_get_current_time(&now);
					twitterdb_set_last_sync(handle, TWITTERDB_SYNC_SOURCE_FOLLOWERS, user_guid, now.tv_sec);
				}
				else
				{
					g_warning("Couldn't synchronize followers of user \"%s\"", username);
				}

			}

			g_object_unref(client);
		}

		twitterdb_close_handle(handle);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
		err = NULL;
	}
}

static void
_mainwindow_sync_gui(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);
	GTimeVal last_sync;
	
	g_mutex_lock(private->last_gui_sync.mutex);
	last_sync = private->last_gui_sync.time;
	g_mutex_unlock(private->last_gui_sync.mutex);

	if((_mainwindow_sync_get_time_diff(last_sync) > MAINWINDOW_GUI_SYNC_MIN) || !last_sync.tv_sec)
	{
		tabbar_refresh(private->tabbar);
		_mainwindow_populate(widget);

		g_mutex_lock(private->last_gui_sync.mutex);
		g_get_current_time(&private->last_gui_sync.time);
		g_mutex_unlock(private->last_gui_sync.mutex);
	}
}

static gboolean
_mainwindow_sync_coprocedure(GtkWidget *widget, gint step)
{
	_MainWindowPrivate *private;
	gint state;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	if(private->no_sync)
	{
		return FALSE;
	}

	g_mutex_lock(private->sync_state.mutex);

	if(step != -1)
	{
		private->sync_state.state = step;
	}

	state = private->sync_state.state;

	g_mutex_unlock(private->sync_state.mutex);

	if(!state || !(state % 2) || state == MAINWINDOW_SYNC_STEP_LAST)
	{
		gdk_threads_enter();
		statusbar_show(private->statusbar, TRUE);
	}

	switch(state)
	{
		case MAINWINDOW_SYNC_STEP_NOTICY_TIMELINES:
			g_debug("Synchronizing timelines");
			statusbar_set_text(private->statusbar, _("Updating messages..."));
			tabbar_set_busy(private->tabbar, TRUE);
			break;

		case MAINWINDOW_SYNC_STEP_TIMELINES:
			_mainwindow_sync_foreach_account(widget, _mainwindow_sync_timelines);
			break;

		case MAINWINDOW_SYNC_STEP_NOTICY_LISTS:
			g_debug("Synchronizing lists");
			tabbar_set_busy(private->tabbar, TRUE);
			statusbar_set_text(private->statusbar, _("Updating lists..."));
			break;

		case MAINWINDOW_SYNC_STEP_LISTS:
			_mainwindow_sync_foreach_account(widget, _mainwindow_sync_lists);
			break;

		case MAINWINDOW_SYNC_STEP_NOTICY_DIRECT_MESSAGES:
			g_debug("Synchronizing direct messages");
			tabbar_set_busy(private->tabbar, TRUE);
			statusbar_set_text(private->statusbar, _("Updating direct messages..."));
			break;

		case MAINWINDOW_SYNC_STEP_DIRECT_MESSAGES:
			break;

		case MAINWINDOW_SYNC_STEP_GUI:
			g_debug("Synchronizing GUI");
			statusbar_show(private->statusbar, FALSE);
			tabbar_set_busy(private->tabbar, FALSE);
			_mainwindow_sync_gui(widget);
			break;

		case MAINWINDOW_SYNC_STEP_NOTICY_FOLLOWERS:
			g_debug("Synchronizing friends");
			statusbar_set_text(private->statusbar, _("Updating followers..."));
			break;

		case MAINWINDOW_SYNC_STEP_FOLLOWERS:
			_mainwindow_sync_foreach_account(widget, _mainwindow_sync_followers);
			break;

		case MAINWINDOW_SYNC_STEP_LAST:
			statusbar_show(private->statusbar, FALSE);
			break;

		default:
			break;
	}

	/* unlock GUI */
	if(!state || !(state % 2) || state == MAINWINDOW_SYNC_STEP_LAST)
	{
		gdk_threads_leave();
	}

	if(state == MAINWINDOW_SYNC_STEP_LAST)
	{
		/* reset status */
		g_mutex_lock(private->sync_state.mutex);
		private->sync_state.state = 0;
		g_mutex_unlock(private->sync_state.mutex);
	
		return FALSE;
	}

	/* send continue message */
	g_mutex_lock(private->sync_state.mutex);
	++private->sync_state.state;
	g_mutex_unlock(private->sync_state.mutex);
	_mainwindow_send_sync_message(widget, MAINWINDOW_SYNC_EVENT_CONTINUE);

	return TRUE;
}

static gboolean
_mainwindow_synchronization_force_step(GtkWidget *widget, _MainWindowSyncStep step, TwitterDbSyncSource sync_source)
{
	TwitterDbHandle *handle;
	GError *err = NULL;

	/* reset last synchronization timestamp */
	if((handle = twitterdb_get_handle(&err)))
	{
		if(!twitterdb_remove_last_sync_source(handle, sync_source, &err))
		{
			g_warning("Couldn't reset last synchronization timestamps for Twitter lists.");
		}

		twitterdb_close_handle(handle);
	}

	/* start synchronization */
	return _mainwindow_sync_coprocedure(widget, step);
}

static gpointer 
_mainwindow_synchronization_thread(GtkWidget *widget)
{
	_MainWindowPrivate *private;
	GTimeVal tv;
	gpointer data;
	gboolean running = TRUE;
	gboolean synchronizing = FALSE;
	GError *err = NULL;

	g_assert(GTK_IS_WINDOW(widget));

	g_debug("Synchronization thread is running");
	private = MAINWINDOW_GET_DATA(widget);

	g_get_current_time(&tv);

	while(running)
	{
		/* update final time */
		g_time_val_add(&tv, 1000000);

		/* get message from queue */
		if((data = g_async_queue_timed_pop(private->sync.queue, &tv)))
		{
			switch(GPOINTER_TO_INT(data))
			{
				case MAINWINDOW_SYNC_EVENT_ABORT:
					running = FALSE;
					break;

				case MAINWINDOW_SYNC_EVENT_START_SYNC:
					if(!synchronizing)
					{
						synchronizing = _mainwindow_sync_coprocedure(widget, -1);
					}
					break;

				case MAINWINDOW_SYNC_EVENT_CONTINUE:
					if(synchronizing)
					{
						synchronizing = _mainwindow_sync_coprocedure(widget, -1);
					}
					break;

				case MAINWINDOW_SYNC_EVENT_UPDATE_TIMELINES:
					synchronizing = _mainwindow_synchronization_force_step(widget, MAINWINDOW_SYNC_STEP_NOTICY_TIMELINES, TWITTERDB_SYNC_SOURCE_TIMELINES);
					break;

				case MAINWINDOW_SYNC_EVENT_UPDATE_LISTS:
					synchronizing = _mainwindow_synchronization_force_step(widget, MAINWINDOW_SYNC_STEP_NOTICY_LISTS, TWITTERDB_SYNC_SOURCE_LISTS);
					break;

				case MAINWINDOW_SYNC_EVENT_UPDATE_GUI:
					/* reset last gui synchronization timestamp */
					g_mutex_lock(private->last_gui_sync.mutex);
					memset(&private->last_gui_sync.time, 0, sizeof(GTimeVal));
					g_mutex_unlock(private->last_gui_sync.mutex);

					/* start gui synchronization */
					synchronizing = _mainwindow_sync_coprocedure(widget, MAINWINDOW_SYNC_STEP_GUI);
					break;
			}

			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
		}
	}

	g_debug("Finishing synchronization thread");

	return NULL;
}

static gboolean
_mainwindow_sync_starter(GtkWidget *widget)
{
	_mainwindow_send_sync_message(widget, MAINWINDOW_SYNC_EVENT_START_SYNC);

	return TRUE;
}

static gboolean
_mainwindow_sync_starter_first(GtkWidget *widget)
{
	_mainwindow_send_sync_message(widget, MAINWINDOW_SYNC_EVENT_START_SYNC);

	return FALSE;
}

/*
 *	global keyboard bindings:
 */
static void
_mainwindow_refresh_closure(GtkWidget *widget)
{
	_mainwindow_send_sync_message(widget, MAINWINDOW_SYNC_EVENT_START_SYNC);
}

static void
_mainwindow_close_tab_closure(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	tabbar_close_current_tab(private->tabbar);
}

/*
 *	notification area:
 */
static void
_mainwindow_notification_area_hide(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	gtk_widget_set_visible(private->notification_box, FALSE);
}

static void
_mainwindow_notification_area_show(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	gtk_widget_set_visible(private->notification_box, TRUE);
}

static void
_mainwindow_notification_area_close_clicked(GtkWidget *button, GtkWidget *mainwindow)
{
	Config *config;
	Section *root;
	Section *section;
	Value *value;

	/* update configuration */
	config = mainwindow_lock_config(mainwindow);
	root = config_get_root(config);

	if(!(section = section_find_first_child(root, "View")))
	{
		section = section_append_child(root, "View");
	}

	if(!(value = section_find_first_value(section, "show-notification-area")))
	{
		value = section_append_value(section, "show-notification-area", VALUE_DATATYPE_BOOLEAN);
	}

	value_set_bool(value, FALSE);

	mainwindow_unlock_config(mainwindow);

	/* hide notification area */
	_mainwindow_notification_area_hide(mainwindow);
}

/*
 *	paned positions:
 */
static void
_mainwindow_save_paned_positions(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);
	Section *section;
	Value *value;

	if((section = section_find_first_child(config_get_root(private->config.config), "Window")))
	{
		if(!(value = section_find_first_value(section, "paned-position-treeview")))
		{
			value = section_append_value(section, "paned-position-treeview", VALUE_DATATYPE_INT32);
		}

		value_set_int32(value, gtk_paned_get_position(GTK_PANED(private->paned_tree)));

		if(!(value = section_find_first_value(section, "paned-position-notifications")))
		{
			value = section_append_value(section, "paned-position-notifications", VALUE_DATATYPE_INT32);
		}

		value_set_int32(value, gtk_paned_get_position(GTK_PANED(private->paned_notifications)));
	}
}

static void
_mainwindow_restore_paned_positions(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);
	GtkAllocation allocation;
	Section *section;
	Value *value;

	/* set default positions */
	gtk_paned_set_position(GTK_PANED(private->paned_tree), 200);
	gtk_widget_get_allocation(private->vbox, &allocation);
	gtk_paned_set_position(GTK_PANED(private->paned_notifications), allocation.height - 150);

	/* get positions from configuration data */
	if((section = section_find_first_child(config_get_root(private->config.config), "Window")))
	{
		if((value = section_find_first_value(section, "paned-position-treeview")) && VALUE_IS_INT32(value))
		{
			gtk_paned_set_position(GTK_PANED(private->paned_tree), value_get_int32(value));
		}

		if((value = section_find_first_value(section, "paned-position-notifications")) && VALUE_IS_INT32(value))
		{
			gtk_paned_set_position(GTK_PANED(private->paned_notifications), value_get_int32(value));
		}
	}
}

static gboolean
_mainwindow_restore_paned_positions_worker(GtkWidget *widget)
{
	gdk_threads_enter();
	_mainwindow_restore_paned_positions(widget);
	gdk_threads_leave();

	return FALSE;
}

/*
 *	window settings:
 */
static void
_mainwindow_save_state(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);
	Section *section;
	Value *value;
	GtkAllocation allocation;
	gint x;
	gint y;

	if(!gtk_widget_get_visible(widget))
	{
		return;
	}

	gtk_widget_get_allocation(widget, &allocation);
	g_debug("Saving window geometry: %dx%d", allocation.width, allocation.height);

	if((section = section_find_first_child(config_get_root(private->config.config), "Window")))
	{
		/* save state & size */
		if(!(value = section_find_first_value(section, "maximized")))
		{
			value = section_append_value(section, "maximized", VALUE_DATATYPE_BOOLEAN);
		}

		if(gdk_window_get_state(widget->window) & GDK_WINDOW_STATE_MAXIMIZED)
		{
			value_set_bool(value, TRUE);

			if((value = section_find_first_value(section, "width")))
			{
				section_remove_value(section, value);
			}

			if((value = section_find_first_value(section, "height")))
			{
				section_remove_value(section, value);
			}
		}
		else
		{
			value_set_bool(value, FALSE);
	
			if(!(value = section_find_first_value(section, "width")))
			{
				value = section_append_value(section, "width", VALUE_DATATYPE_INT32);
			}

			value_set_int32(value, allocation.width);

			if(!(value = section_find_first_value(section, "height")))
			{
				value = section_append_value(section, "height", VALUE_DATATYPE_INT32);
			}

			value_set_int32(value, allocation.height);
		}

		/* save position */
		gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
		g_debug("Saving window position: %dx%d", x, y);

		if(!(value = section_find_first_value(section, "x")))
		{
			value = section_append_value(section, "x", VALUE_DATATYPE_INT32);
		}

		value_set_int32(value, x);

		if(!(value = section_find_first_value(section, "y")))
		{
			value = section_append_value(section, "y", VALUE_DATATYPE_INT32);
		}

		value_set_int32(value, y);
	}
}

static void
_mainwindow_restore_state(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);
	Section *section;
	Value *value;
	gint x = 0;
	gint y = 0;
	gboolean has_x = FALSE;
	gboolean has_y = FALSE;

	if((section = section_find_first_child(config_get_root(private->config.config), "Window")))
	{
		if((value = section_find_first_value(section, "restore-window")) && VALUE_IS_BOOLEAN(value) && value_get_bool(value))
		{
			g_debug("Restoring window state");
	
			if((value = section_find_first_value(section, "maximized")) && VALUE_IS_BOOLEAN(value) && value_get_bool(value))
			{
				g_debug("Maximizing window");
				gtk_window_maximize(GTK_WINDOW(widget));
			}
			else
			{
				/* restore size */
				if((value = section_find_first_value(section, "width")) && VALUE_IS_INT32(value))
				{
					x = value_get_int32(value);
				}

				if((value = section_find_first_value(section, "height")) && VALUE_IS_INT32(value))
				{
					y = value_get_int32(value);
				}

				if(x > 0 && y > 0)
				{
					g_debug("Restoring window geometry: %dx%d", x, y);
					gtk_window_resize(GTK_WINDOW(widget), x, y);
				}

				/* restore position */
				x = 0;
				y = 0;

				if((value = section_find_first_value(section, "x")) && VALUE_IS_INT32(value))
				{
					x = value_get_int32(value);
					has_x = TRUE;
				}

				if((value = section_find_first_value(section, "y")) && VALUE_IS_INT32(value))
				{
					y = value_get_int32(value);
					has_y = TRUE;
				}

				if(has_x && has_y)
				{
					g_debug("Restoring window position: %dx%d", x, y);
					gtk_window_move(GTK_WINDOW(widget), x, y);
				}
				else
				{
					gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER);
				}
			}
		}
		else
		{
			gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER);
			gtk_window_maximize(GTK_WINDOW(widget));
		}
	}
}

/*
 *	preferences:
 */
static void
_mainwindow_apply_preferences(GtkWidget *widget, gboolean save_preferences)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);
	Config *config;
	Section *root;
	Section *section;
	Value *value;
	gboolean enable_systray = FALSE;
	gboolean show_notifications = FALSE;
	GError *err = NULL;

	g_debug("Applying preferences");

	/* lock configuration & receive root section */
	config = mainwindow_lock_config(widget);
	root = config_get_root(config);

	/* get window preferences */
	if((section = section_find_first_child(root, "Window")))
	{
		if((value = section_find_first_value(section, "enable-systray")) && VALUE_IS_BOOLEAN(value))
		{
			enable_systray = value_get_bool(value);
		}
	}

	/* show/hide systray icon */
	_mainwindow_show_systray_icon(widget, enable_systray);

	/* set notification level & update visibility */
	g_mutex_lock(private->notification_level.mutex);

	private->notification_level.level = 2;

	if((section = section_find_first_child(root, "View")))
	{
		if((value = section_find_first_value(section, "show-notification-area")) && VALUE_IS_BOOLEAN(value))
		{
			show_notifications = value_get_bool(value);
		}

		if((value = section_find_first_value(section, "notification-area-level")) && VALUE_IS_INT32(value))
		{
			private->notification_level.level = value_get_int32(value);
		}
	}

	if(private->notification_level.level < 0 || private->notification_level.level > 2)
	{
		private->notification_level.level = 2;
	}

	g_mutex_unlock(private->notification_level.mutex);

	if(show_notifications)
	{
		_mainwindow_notification_area_show(widget);
	}
	else
	{
		_mainwindow_notification_area_hide(widget);
	}

	/* save configuration */
	if(save_preferences)
	{
		if(!config_save(config, &err))
		{
			g_warning("Couldn't save configuration");
		}

		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	/* unlock configuration */
	mainwindow_unlock_config(widget);
}

/*
 *	compose tweets:
 */
static gboolean
_mainwindow_compose_tweet_callback(const gchar *username, const gchar *text, GtkWidget *mainwindow)
{
	TwitterClient *client;
	GtkWidget *dialog;
	GError *err = NULL;
	gboolean result = FALSE;

	g_assert(GTK_IS_WINDOW(mainwindow));

	if((client = mainwindow_create_twittter_client(mainwindow, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
	{
		if((result = twitter_client_post(client, username, text, &err)))
		{
			mainwindow_sync_gui(mainwindow);
		}
		else
		{
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}

			dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow),
		                                        GTK_DIALOG_MODAL,
		                                        GTK_MESSAGE_WARNING,
		                                        GTK_BUTTONS_OK,
		                                        _("Couldn't publish your tweet, please try again later."));
			g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, dialog);
		}

		g_object_unref(client);
	}

	return result;
}

static void
_mainwindow_compose_tweet(GtkWidget *widget)
{
	_MainWindowPrivate *private;
	GtkWidget *dialog;
	gint count;
	gchar **usernames = NULL;
	static gchar selected_user[64] = { 0 };
	gchar *id;
	gchar *username = NULL;
	gchar **pieces;
	gint i;

	/* get user accounts */
	if((count = _mainwindow_sync_get_accounts(widget, &usernames, NULL, NULL)))
	{
		/* try to get username from current tab id */
		private = MAINWINDOW_GET_DATA(widget);

		if((id = tabbar_get_current_id(private->tabbar)))
		{
			if(g_strstr_len(id, -1, "@"))
			{
				pieces = g_strsplit(id, "@", 2);
				username = g_strdup(pieces[0]);
				g_strfreev(pieces);
				g_free(id);
			}
			else
			{
				username = id;
			}

			/* check if found username is an account */
			for(i = 0; i < count; ++i)
			{
				if(!g_strcmp0(username, usernames[i]))
				{
					g_strlcpy(selected_user, username, 64);
					break;
				}
			}

			g_free(username);
		}

		/* copy first found username if username is empty */
		if(!selected_user[0])
		{
			g_strlcpy(selected_user, usernames[0], 64);
		}

		/* create & run composer dialog */
		dialog = composer_dialog_create(widget, usernames, count, selected_user, _("Compose new tweet"));
		composer_dialog_set_apply_callback(dialog, (ComposerApplyCallback)_mainwindow_compose_tweet_callback, widget);
		gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));

		/* cleanup */
		if(GTK_IS_WIDGET(dialog))
		{
			/* try to copy selected username */
			if((username = composer_dialog_get_user(dialog)))
			{
				g_strlcpy(selected_user, username, 64);
				g_free(username);
			}

			gtk_widget_destroy(dialog);
		}

		/* cleanup */
		for(gint i = 0; i < count; ++i)
		{
			g_free(usernames[i]);
		}
	}
}

/*
 *	mainmenu:
 */
static void
_mainwindow_menu_item_activated(GtkWidget *widget, gpointer user_data)
{
	_MainWindowMenuData *menu = (_MainWindowMenuData *)user_data;
	GtkWidget *dialog = NULL;

	switch(menu->action)
	{
		case MAINMENU_ACTION_REFRESH:
			/* send synchronization event message */
			_mainwindow_send_sync_message(menu->window, MAINWINDOW_SYNC_EVENT_START_SYNC);
			break;

		case MAINMENU_ACTION_QUIT:
			/* close program */ 
			mainwindow_quit(menu->window);
			break;

		case MAINMENU_ACTION_ABOUT:
			/* create & run "about" dialog */
			dialog = about_dialog_create(menu->window);
			gtk_dialog_run(GTK_DIALOG(dialog));
			break;

		case MAINMENU_ACTION_COMPOSE_TWEET:
			_mainwindow_compose_tweet(menu->window);
			break;

		case MAINMENU_ACTION_PREFERENCES:
			/* create & run "preferences "dialog */
			dialog = preferences_dialog_create(menu->window);
			if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_APPLY)
			{
				/* apply preferences */
				_mainwindow_apply_preferences(menu->window, TRUE);
			}
			break;

		case MAINMENU_ACTION_PREFERENCES_ACCOUNTS:
			/* create & run" accounts" dialog */
			dialog = accounts_dialog_create(menu->window);
			gtk_dialog_run(GTK_DIALOG(dialog));

			/* update accounts */
			_mainwindow_send_sync_message(menu->window, MAINWINDOW_SYNC_EVENT_UPDATE_GUI);
			break;

		default:
			g_warning("Invalid menu id");
	}

	/* destroy dialog */
	if(dialog)
	{
		gtk_widget_destroy(dialog);
	}
}

static _MainWindowMenuData 
_mainwindow_create_menu_data(GtkWidget *widget, MainMenuAction action)
{
	_MainWindowMenuData data;

	data.window = widget;
	data.action = action;

	return data;
}

static GtkWidget *
_mainwindow_create_menu(GtkWidget *widget)
{
	GtkWidget *menu;
	GtkWidget *submenu;
	GtkWidget *item;
	static _MainWindowMenuData menu_data[5];

	g_debug("Creating mainmenu");

	menu = gtk_menu_bar_new();

	for(gint i = 0; i <= 5; ++i)
	{
		menu_data[i] = _mainwindow_create_menu_data(widget, i);
	}

	/*
	 *	menu "Jekyll":
	 */
	item = gtk_menu_item_new_with_mnemonic("_Jekyll");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_menu_item_activated), (gpointer)&(menu_data[MAINMENU_ACTION_REFRESH]));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), gtk_separator_menu_item_new());

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_menu_item_activated), (gpointer)&(menu_data[MAINMENU_ACTION_QUIT]));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	/*
	 *	menu "Twitter":
	 */
	item = gtk_menu_item_new_with_mnemonic("_Twitter");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	item = gtk_image_menu_item_new_with_mnemonic(_("_Compose tweet"));
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_menu_item_activated), (gpointer)&(menu_data[MAINMENU_ACTION_COMPOSE_TWEET]));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	/*
	 *	menu "Preferences":
	 */
	item = gtk_menu_item_new_with_mnemonic(_("_Preferences"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_menu_item_activated), (gpointer)&(menu_data[MAINMENU_ACTION_PREFERENCES]));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	item = gtk_image_menu_item_new_with_mnemonic(_("_Twitter Accounts"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_menu_item_activated), (gpointer)&(menu_data[MAINMENU_ACTION_PREFERENCES_ACCOUNTS]));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	/*
	 *	menu "?":
	 */
	item = gtk_menu_item_new_with_mnemonic("_?");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_menu_item_activated), (gpointer)&(menu_data[MAINMENU_ACTION_ABOUT]));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

	return menu;
}

/*
 *	edit followers:
 */
static gpointer
_mainwindow_edit_followers_apply_worker(_MainwindowEditFollowersWorker *arg)
{
	GList *users;
	GList *iter;
	gchar *username;
	TwitterClient *client;
	GError *err = NULL;
	GtkWidget *message_dialog;
	gboolean success = FALSE;

	g_debug("Starting worker: %s", __func__);

	/* try to update followers */
	if((client = mainwindow_create_twittter_client(arg->parent, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
	{
		gdk_threads_enter();
		users = gtk_user_list_dialog_get_users(GTK_USER_LIST_DIALOG(arg->dialog), arg->edit_friends ? FALSE : TRUE);
		gdk_threads_leave();

		iter = users;
		success = TRUE;

		while(iter && success)
		{
			username = (gchar *)iter->data;

			if(arg->edit_friends)
			{
				g_debug("Removing friendship (account=\"%s\", user=\"%s\")", arg->account, username);
				success = twitter_client_remove_friend(client, arg->account, username, &err);
			}
			else
			{
				g_debug("Blocking (account=\"%s\", user=\"%s\")", arg->account, username);
				success = twitter_client_block_follower(client, arg->account, username, &err);
			}

			if(!success)
			{
				if(err)
				{
					g_warning("%s", err->message);
					g_error_free(err);
					err = NULL;
				}
			}

			iter = iter->next;
		}

		g_list_foreach(users, (GFunc)g_free, NULL);
		g_list_free(users);

		/* close dialog */
		gdk_threads_enter();
		gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(arg->dialog), GTK_RESPONSE_APPLY);
		gdk_threads_leave();

		/* free memory */
		g_object_unref(client);
	}

	if(!success)
	{
		/* show failure message */
		message_dialog = gtk_message_dialog_new(GTK_WINDOW(arg->parent),
		                                        GTK_DIALOG_MODAL,
		                                        GTK_MESSAGE_WARNING,
		                                        GTK_BUTTONS_OK,
		                                        arg->edit_friends ?
		                                        _("Couldn't update friends, please try again later.") :
		                                        _("Couldn't update followers, please try again later."));
		g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, message_dialog);
	}


	g_debug("Leaving worker: %s", __func__);

	return NULL;
}

static void
_mainwindow_edit_followers_apply(GtkWidget *button, _MainwindowEditFollowersWorker *arg)
{
	GThread *thread;
	GError *err = NULL;

	/* disable window */
	gtk_helpers_set_widget_busy(arg->dialog, TRUE);

	/* start worker */
	thread = g_thread_create((GThreadFunc)_mainwindow_edit_followers_apply_worker, arg, FALSE, &err);

	if(err)
	{
		g_warning("%s\n", err->message);
		g_error_free(err);
	}

	g_assert(thread != NULL);
}

static void
_mainwindow_add_user_to_edit_members_dialog(TwitterUser user, _MainwindowEditFollowersWorker *arg)
{
	gdk_threads_enter();
	edit_members_dialog_add_user(arg->dialog, user, arg->edit_friends);
	gdk_threads_leave();
}

static gboolean
_mainwindow_edit_followers_worker(_MainwindowEditFollowersWorker *arg)
{
	GtkWidget *dialog;
	TwitterDbHandle *handle;
	GtkWidget *button;
	gchar user_guid[32];
	GError *err = NULL;

	gdk_threads_enter();

	/* create dialog */
	dialog = edit_members_dialog_create(arg->parent, arg->edit_friends ? _("Editing friends") : _("Editing followers"));
	g_object_set(G_OBJECT(dialog),
	             "checkbox-column-visible",
	             TRUE,
	             "checkbox-column-activatable",
	             TRUE,
	             "checkbox-column-title",
	             arg->edit_friends ? _("Follow user") : _("Block user"),
	             "username-column-title",
	             _("Username"),
	             "has-separator",
	             FALSE,
	             NULL);
	arg->dialog = dialog;

	/* add buttons */
	button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_box_pack_start(GTK_BOX(gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog))), button, FALSE, FALSE, 0);
	gtk_widget_set_visible(button, TRUE);
	g_signal_connect(button, "clicked", (GCallback)_mainwindow_edit_followers_apply, arg);
	gtk_deletable_dialog_add_button(GTK_DELETABLE_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gdk_threads_leave();

	/* populate list */
	if((handle = twitterdb_get_handle(&err)))
	{
		g_debug("Mapping username: \"%s\"", arg->account);
		if(twitterdb_map_username(handle, arg->account, user_guid, 64, &err))
		{
			g_debug("Username mapped successfully: \"%s\" => \"%s\"", arg->account, user_guid);
			twitterdb_foreach_follower(handle, user_guid, arg->edit_friends, (TwitterProcessUserFunc)_mainwindow_add_user_to_edit_members_dialog, arg, &err);
		}

		twitterdb_close_handle(handle);
	}

	/* display error message */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	/* run dialog */
	gdk_threads_enter();

	if(gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog)) == GTK_RESPONSE_APPLY)
	{
		// TODO
	}

	edit_members_dialog_destroy(dialog);

	gdk_threads_leave();

	/* free memory */
	g_slice_free1(sizeof(_MainwindowEditFollowersWorker), arg);

	return FALSE;
}

static void
_mainwindow_edit_followers(GtkWidget *widget, const gchar *account, gboolean edit_friends)
{
	_MainwindowEditFollowersWorker *arg;


	arg = (_MainwindowEditFollowersWorker *)g_slice_alloc(sizeof(_MainwindowEditFollowersWorker));
	arg->parent = widget;
	g_strlcpy(arg->account, account, 64);
	arg->edit_friends = edit_friends;

	g_idle_add((GSourceFunc)_mainwindow_edit_followers_worker, arg);
}

/*
 *	accountbrowser popup menues:
 */
static void
_mainwindow_accountbrowser_menu_add_list_activated(GtkWidget *item, GtkWidget *mainwindow)
{
	GtkWidget *dialog;
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(mainwindow);

	dialog = add_list_dialog_create(mainwindow, private->accountbrowser_menues.account);

	gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));

	if(GTK_IS_DELETABLE_DIALOG(dialog))
	{
		gtk_widget_destroy(dialog);
	}
}

static void
_mainwindow_accountbrowser_menu_open_list_activated(GtkWidget *item, GtkWidget *mainwindow)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(mainwindow);

	tabbar_open_list(private->tabbar, private->accountbrowser_menues.account, private->accountbrowser_menues.text);
}

static void
_mainwindow_accountbrowser_menu_edit_list_activated(GtkWidget *item, GtkWidget *mainwindow)
{
	GtkWidget *dialog;
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(mainwindow);
	MainWindowSyncStatus status;

	status = mainwindow_get_current_sync_status(mainwindow);

	if(status != MAINWINDOW_SYNC_STATUS_SYNC_LISTS)
	{
		dialog = list_preferences_dialog_create(mainwindow, _("List preferences"), private->accountbrowser_menues.account, private->accountbrowser_menues.text);
		gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

static void
_mainwindow_accountbrowser_menu_remove_list_activated(GtkWidget *item, GtkWidget *mainwindow)
{
	GtkWidget *dialog;
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(mainwindow);
	MainWindowSyncStatus status;

	status = mainwindow_get_current_sync_status(mainwindow);

	if(status != MAINWINDOW_SYNC_STATUS_SYNC_LISTS)
	{
		dialog = remove_list_dialog_create(mainwindow, private->accountbrowser_menues.account, private->accountbrowser_menues.text);
		gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));

		if(GTK_IS_DELETABLE_DIALOG(dialog))
		{
			gtk_widget_destroy(dialog);
		}
	}
}

static GtkWidget *
_mainwindow_create_popup_menu_lists(GtkWidget *mainwindow)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_accountbrowser_menu_add_list_activated), (gpointer)mainwindow);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	gtk_widget_show_all(menu);

	return menu;
}

static GtkWidget *
_mainwindow_create_popup_menu_list_full(GtkWidget *mainwindow)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_accountbrowser_menu_open_list_activated), (gpointer)mainwindow);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_EDIT, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_accountbrowser_menu_edit_list_activated), (gpointer)mainwindow);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_accountbrowser_menu_remove_list_activated), (gpointer)mainwindow);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	gtk_widget_show_all(menu);

	return menu;
}

static GtkWidget *
_mainwindow_create_popup_menu_list_readonly(GtkWidget *mainwindow)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_mainwindow_accountbrowser_menu_open_list_activated), (gpointer)mainwindow);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	gtk_widget_show_all(menu);

	return menu;
}

/*
 *	pixbuf loader:
 */
static PixbufLoader *
_mainwindow_pixbuf_loader_create(void)
{
	gchar *path;
	PixbufLoader *pixbuf_loader;

	path = g_build_filename(pathbuilder_get_user_application_directory(), G_DIR_SEPARATOR_S, ".images", NULL);
	pixbuf_loader = pixbuf_loader_new(path);
	g_free(path);

	return pixbuf_loader;
}

/*
 *	window:
 */
static gboolean
_mainwindows_destroy_worker(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	/* hide window */
	gdk_threads_enter();
	gtk_widget_set_visible(widget, FALSE);
	gdk_flush();
	gdk_threads_leave();

	/* free configuration mutex */
	g_debug("Destroying configuration mutex");
	g_mutex_free(private->config.mutex);

	/* finish synchronization thread */
	_mainwindow_send_sync_message(widget, MAINWINDOW_SYNC_EVENT_ABORT);
	g_debug("Joining sync thread...");
	g_thread_join(private->sync.thread);
	g_debug("Thread has been finished");

	gdk_threads_enter();

	/* destroy children */
	if(private->systray)
	{
		g_debug("Destroying systray");
		systray_destroy(private->systray);
	}

	g_debug("Destroying accountbrowser");
	accountbrowser_destroy(private->accountbrowser);

	g_debug("Destroying statusbar");
	statusbar_destroy(private->statusbar);

	g_debug("Destroying notification area");
	notification_area_destroy(private->notification_area);

	gdk_threads_leave(); /* tabbar_destroy() joins a thread which locks the GDK mutex
	                      * and so we have to unlock it to avoid a deadlock. */

	g_debug("Destroying tabbar");
	tabbar_destroy(private->tabbar);

	g_debug("Destroying synchronization data");
	g_async_queue_unref(private->sync.queue);

	/* quit pixbuf loader */
	g_debug("Stopping pixbuf loader...");
	pixbuf_loader_stop(private->pixbuf_loader);
	g_object_unref(private->pixbuf_loader);

	/* cancellable */
	g_debug("Destroying cancellable");
	g_object_unref(private->cancellable);

	/* free memory */
	g_debug("Freeing memory");
	g_mutex_free(private->sync_state.mutex);
	g_mutex_free(private->notification_level.mutex);
	g_mutex_free(private->last_gui_sync.mutex);
	g_free(private);

	/* destroy window widget */
	gdk_threads_enter();
	gtk_widget_destroy(widget);
	gdk_threads_leave();

	return FALSE;
}

static void
_mainwindow_destroy(GtkWidget *widget, gpointer user_data)
{
	/* quit mainloop */
	g_debug("Stopping mainloop");
	gtk_main_quit();
	g_debug("Mainloop stopped");
}

static void
_mainwindow_hide(GtkWidget *widget, gboolean lock_config)
{
	_MainWindowPrivate *private;
	Config *config;
	Section *section;
	Value *value;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	if(lock_config)
	{
		g_mutex_lock(private->config.mutex);
	}

	config = private->config.config;

	/* store window position */
	_mainwindow_save_state(widget);

	/* check if systray support is enabled */
	if((section = section_find_first_child(config_get_root(config), "Window")))
	{
		if((value = section_find_first_value(section, "enable-systray")) && VALUE_IS_BOOLEAN(value) && value_get_bool(value))
		{
			/* hide window */
			gtk_widget_set_visible(widget, FALSE);
		}
		else
		{
			/* minimize window */
			gtk_window_iconify(GTK_WINDOW(widget));
		}
	}

	if(lock_config)
	{
		g_mutex_unlock(private->config.mutex);
	}
}

static gboolean
_mainwindow_delete(GtkWidget *widget, GdkEvent event, gpointer data)
{
	Config *config;
	Section *root;
	Section *section;
	Value *value;
	gboolean systray = FALSE;
	static gboolean shutdown = FALSE;

	if(shutdown)
	{
		return FALSE;
	}

	/* lock configuration & receive root section */
	config = mainwindow_lock_config(widget);
	root = config_get_root(config);

	/* check if systray support is enabled */
	if((section = section_find_first_child(root, "Window")))
	{
		if((value = section_find_first_value(section, "enable-systray")) && VALUE_IS_BOOLEAN(value))
		{
			if((systray = value_get_bool(value)))
			{
				/* hide window */
				_mainwindow_hide(widget, FALSE);
			}
		}
	}

	/* unlock configuration */
	mainwindow_unlock_config(widget);

	/* close program if systray support isn't enabled */
	if(!systray)
	{
		shutdown = TRUE;
		mainwindow_quit(widget);
	}

	return TRUE;
}

static void
_mainwindow_initialize_window_visibility(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);
	Section *section;
	Value *value;
	gboolean show_window = TRUE;

	/* hide statusbar */
	statusbar_show(private->statusbar, FALSE);

	/* check if user wants the window to be opened minimized */
	if((section = section_find_first_child(config_get_root(private->config.config), "Window")))
	{
		if((value = section_find_first_value(section, "start-minimized")) && VALUE_IS_BOOLEAN(value))
		{
			if(value_get_bool(value))
			{
				if((value = section_find_first_value(section, "enable-systray")) && VALUE_IS_BOOLEAN(value) && !value_get_bool(value))
				{
					/* show window in taskbar if systray support is disabled */
					gtk_widget_set_visible(widget, TRUE);
					gtk_window_iconify(GTK_WINDOW(widget));
					show_window = FALSE;
				}
				else
				{
					show_window = FALSE;
				}
			}
		}
	}

	if(show_window)
	{
		mainwindow_show(widget);
	}
}

static void
_mainwindow_assign_startup_options(GtkWidget *mainwindow)
{
	_MainWindowPrivate *private;
	Section *root;
	Section *section;
	Value *value;

	private = MAINWINDOW_GET_DATA(mainwindow);
	root = config_get_root(private->config.config);

	if((section = section_find_first_child(root, "Startup")))
	{
		if((value = section_find_first_value(section, "enable-debug")) && VALUE_IS_BOOLEAN(value))
		{
			private->enable_debug = value_get_bool(value);
		}

		if((value = section_find_first_value(section, "no-sync")) && VALUE_IS_BOOLEAN(value))
		{
			private->no_sync = value_get_bool(value);
		}
	}
}

static GtkWidget *
_mainwindow_create(Config *config, Cache *cache)
{
	GtkWidget *window;
	_MainWindowPrivate *private;
	GtkWidget *vbox;
	GtkWidget *left;
	GtkWidget *right;
	GtkUIManager *uimanager;
	GtkAccelGroup *accelgroup;
	GClosure *closure;
	guint key;
	GdkModifierType mods;
	GtkWidget *align;
	GtkWidget *button_close;

	/* create window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), APPLICATION_NAME);
	gtk_widget_set_size_request(window, MAINWINDOW_MINIMUM_WIDTH, MAINWINDOW_MINIMUM_HEIGHT);

	/* window data  */
	private = (_MainWindowPrivate *)g_malloc(sizeof(_MainWindowPrivate));
	memset(private, 0, sizeof(_MainWindowPrivate));

	private->config.config = config;
	private->config.mutex = g_mutex_new();
	private->cache = cache;
	private->notification_level.mutex = g_mutex_new();
	private->sync_state.mutex = g_mutex_new();
	private->last_gui_sync.mutex = g_mutex_new();
	g_object_set_data(G_OBJECT(window), "private", private);

	/* assign startup options */
	_mainwindow_assign_startup_options(window);

	/* synchronization data & thread */
	g_debug("Initializing window synchronization data");
	private->sync.queue = g_async_queue_new();
	g_debug("Starting synchronization thread");
	private->sync.thread = g_thread_create((GThreadFunc)_mainwindow_synchronization_thread, window, TRUE, NULL);

	g_assert(private->sync.thread != NULL);

	/* pixmap loader */
	g_debug("Creating pixbuf loader");
	private->pixbuf_loader = _mainwindow_pixbuf_loader_create();

	/* cancellable */
	private->cancellable = g_cancellable_new();

	/* mainbox */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* mainmenu */
	private->menu = _mainwindow_create_menu(window);
	gtk_box_pack_start(GTK_BOX(vbox), private->menu, FALSE, TRUE, 0);

	/* systray icon */
	private->systray = NULL;

	/* paned and mainboxes */
	left = gtk_vbox_new(FALSE, 0);
	right = gtk_hbox_new(FALSE, 0);

	private->paned_tree = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), private->paned_tree, TRUE, TRUE, 1);
	gtk_paned_pack1(GTK_PANED(private->paned_tree), left, FALSE, FALSE);
	gtk_paned_pack2(GTK_PANED(private->paned_tree), right, TRUE, TRUE);

	/* global keyboard accelerators */
	uimanager = gtk_ui_manager_new();
	accelgroup = gtk_ui_manager_get_accel_group(uimanager);

	closure = g_cclosure_new_swap(G_CALLBACK(_mainwindow_refresh_closure), (gpointer)window, NULL);
	gtk_accelerator_parse("F5", &key, 0);
	gtk_accel_group_connect(accelgroup, key, 0, 0, closure);

	closure = g_cclosure_new_swap(G_CALLBACK(_mainwindow_close_tab_closure), (gpointer)window, NULL);
	gtk_accelerator_parse("<Control>w", &key, &mods);
	gtk_accel_group_connect(accelgroup, key, mods, 0, closure);

	gtk_window_add_accel_group(GTK_WINDOW(window), accelgroup);

	g_object_unref(G_OBJECT(uimanager));

	/* accountbrowser */
	private->accountbrowser = accountbrowser_create(window);
	gtk_box_pack_start(GTK_BOX(left), private->accountbrowser, TRUE, TRUE, 0);

	/* statusbar */
	private->statusbar = statusbar_create();
	gtk_box_pack_start(GTK_BOX(left), private->statusbar, FALSE, TRUE, 0);

	/* paned */
	private->paned_notifications = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(right), private->paned_notifications, TRUE, TRUE, 1);

	/* tabbar */
	private->tabbar = tabbar_create(window);
	gtk_paned_pack1(GTK_PANED(private->paned_notifications), private->tabbar, TRUE, TRUE);

	/* notification area */
	private->notification_box = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(private->paned_notifications), private->notification_box, TRUE, TRUE);

	align = gtk_alignment_new(1, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(private->notification_box), align, FALSE, TRUE, 0);

	button_close = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button_close), gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	g_object_set(G_OBJECT(button_close), "relief", GTK_RELIEF_NONE, NULL);
	gtk_container_add(GTK_CONTAINER(align), button_close);

	private->notification_area = notification_area_create();
	gtk_box_pack_start(GTK_BOX(private->notification_box), private->notification_area, TRUE, TRUE, 0);

	/* create popup menues */
	g_debug("Creating popup menue");
	private->accountbrowser_menues.menu_lists = _mainwindow_create_popup_menu_lists(window);
	private->accountbrowser_menues.menu_list[0] = _mainwindow_create_popup_menu_list_full(window);
	private->accountbrowser_menues.menu_list[1] = _mainwindow_create_popup_menu_list_readonly(window);

	/* populate data */
	_mainwindow_populate(window);

	/* start background worker to start synchronization regulary */
	g_timeout_add_seconds(MAINWINDOW_UPDATE_INTERVAL, (GSourceFunc)_mainwindow_sync_starter, window);
	g_idle_add((GSourceFunc)_mainwindow_sync_starter_first, window);

	/* show widgets */
	private->vbox = vbox;
	private->left = left;
	private->right = right;
	gtk_widget_show_all(private->vbox);
	_mainwindow_initialize_window_visibility(window);

	/* apply preferences */
	_mainwindow_apply_preferences(window, FALSE);

	/* start pixmap loader */
	g_debug("Starting pixbuf loader");
	pixbuf_loader_start(private->pixbuf_loader);

	/* signals */
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(_mainwindow_destroy), NULL);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(_mainwindow_delete), NULL);
	g_signal_connect(G_OBJECT(button_close), "clicked", G_CALLBACK(_mainwindow_notification_area_close_clicked), window);

	return window;
}

/*
 *	public:
 */
void
mainwindow_start(Config *config, Cache *cache)
{
	GtkWidget *window;
	gchar info[256];
	gint callback_id;

	/* create mainwindow */
	window = _mainwindow_create(config, cache);

	/* add listener handler */
	callback_id = listener_add_callback((ListenerCallback)_mainwindow_handle_listener_request, window, NULL);
	g_object_set_data(G_OBJECT(window), "listener-callback-id", GINT_TO_POINTER(callback_id));

	/* assign startup option */
	_mainwindow_assign_startup_options(window);

	/* set log handler */
	g_debug("Registering log handler");
	_mainwindow_set_log_handler(window);

	/* display notification */
	sprintf(info, "%s %d.%d.%d has been started successfully", APPLICATION_NAME, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH_LEVEL);
	g_log(NULL, G_LOG_LEVEL_INFO, "%s", info);

	/* start mainloop */
	gtk_main();
}

void
mainwindow_quit(GtkWidget *widget)
{
	_MainWindowPrivate *private;
	gint callback_id;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	g_debug("Removing log handler");
	g_log(NULL, G_LOG_LEVEL_INFO, "%s", "Shutting down application");
	g_log_remove_handler(NULL, private->log_handler);

	/* save window state & paned position */
	g_debug("Saving window state & paned position");
	_mainwindow_save_state(widget);
	_mainwindow_save_paned_positions(widget);

	/* remove listener callback function */
	callback_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "listener-callback-id"));
	listener_remove_callback(callback_id);

	/* cancel operations */
	g_cancellable_cancel(private->cancellable);

	/* destroy window */
	g_idle_add((GSourceFunc)_mainwindows_destroy_worker, widget); /* calling the destroy functions
	                                                                 outside of a worker will cause
	                                                                 a deadlock! */
}

void
mainwindow_show(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	if(!gtk_widget_get_visible(widget))
	{
		_mainwindow_restore_state(widget);
	}

	gtk_widget_set_visible(widget, TRUE);
	gtk_window_deiconify(GTK_WINDOW(widget));
	gtk_window_present(GTK_WINDOW(widget));

	if(!private->visibility_set)
	{
		private->visibility_set = TRUE;
		g_idle_add((GSourceFunc)_mainwindow_restore_paned_positions_worker, widget);
	}
}

void
mainwindow_hide(GtkWidget *widget)
{
	_mainwindow_hide(widget, TRUE);
}

void
mainwindow_account_node_activated(GtkWidget *widget, const gchar *account, AccountBrowserTreeViewNodeType type, const gchar *text)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	switch(type)
	{
		case ACCOUNTBROWSER_TREEVIEW_NODE_MESSAGES:
			g_debug("Opening public timeline (\"%s\")", account);
			tabbar_open_public_timeline(private->tabbar, account);
			break;

		case ACCOUNTBROWSER_TREEVIEW_NODE_DIRECT_MESSAGES:
			g_debug("Opening private messages (\"%s\")", account);
			tabbar_open_direct_messages(private->tabbar, account);
			break;

		case ACCOUNTBROWSER_TREEVIEW_NODE_REPLIES:
			g_debug("Opening replies: (\"%s\")", account);
			tabbar_open_replies(private->tabbar, account);
			break;

		case ACCOUNTBROWSER_TREEVIEW_NODE_USER_TIMELINE:
			g_debug("Opening user timeline: (\"%s\")", account);
			tabbar_open_user_timeline(private->tabbar, account);
			break;

		case ACCOUNTBROWSER_TREEVIEW_NODE_PROTECTED_LIST:
		case ACCOUNTBROWSER_TREEVIEW_NODE_LIST:
			g_debug("Opening list: (\"%s\"@\"%s\")", account, text);
			tabbar_open_list(private->tabbar, account, text);
			break;

		case ACCOUNTBROWSER_TREEVIEW_NODE_FRIENDS:
			_mainwindow_edit_followers(widget, account, TRUE);
			break;

		case ACCOUNTBROWSER_TREEVIEW_NODE_FOLLOWERS:
			_mainwindow_edit_followers(widget, account, FALSE);
			break;

		default:
			break;
	}
}

void
mainwindow_account_node_right_clicked(GtkWidget *widget, guint event_time, const gchar *account, AccountBrowserTreeViewNodeType type, const gchar *text)
{
	_MainWindowPrivate *private;
	MainWindowSyncStatus status;
	gint index = 0;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);

	g_strlcpy(private->accountbrowser_menues.account, account, 64);
	g_strlcpy(private->accountbrowser_menues.text, text, 64);

	switch(type)
	{
		case ACCOUNTBROWSER_TREEVIEW_NODE_LISTS:
			g_debug("Handling lists node");
			gtk_menu_popup(GTK_MENU(private->accountbrowser_menues.menu_lists), NULL, NULL, NULL, NULL, 3, event_time);
			break;

		case ACCOUNTBROWSER_TREEVIEW_NODE_LIST:
		case ACCOUNTBROWSER_TREEVIEW_NODE_PROTECTED_LIST:
			g_debug("Handling list node");
			status = mainwindow_get_current_sync_status(widget);
			if(status == MAINWINDOW_SYNC_STATUS_SYNC_LISTS)
			{
				++index;
			}
			gtk_menu_popup(GTK_MENU(private->accountbrowser_menues.menu_list[index]), NULL, NULL, NULL, NULL, 3, event_time);
			break;

		default:
			break;
	}
}

Config *
mainwindow_lock_config(GtkWidget *widget)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	g_debug("Locking configuration");
	g_mutex_lock(private->config.mutex);

	return private->config.config;
}

void
mainwindow_unlock_config(GtkWidget *widget)
{
	g_debug("Unlocking configuration");
	g_mutex_unlock((MAINWINDOW_GET_DATA(widget))->config.mutex);
}

void
mainwindow_load_pixbuf(GtkWidget *widget, const gchar *group, const gchar *url, PixbufLoaderCallback callback, gpointer user_data, GFreeFunc free_func)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	pixbuf_loader_add_callback(private->pixbuf_loader, url, callback, group, user_data, free_func);
}

void
mainwindow_remove_pixbuf_group(GtkWidget *widget, const gchar *group)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	pixbuf_loader_remove_group(private->pixbuf_loader, group);
}

TwitterClient *
mainwindow_create_twittter_client(GtkWidget *widget, gint lifetime)
{
	Config *config;
	TwitterClient *client;
	_MainWindowPrivate *private;

	g_assert(GTK_IS_WIDGET(widget));

	g_debug("Creating TwitterClient instance");
	private = MAINWINDOW_GET_DATA(widget);

	config = mainwindow_lock_config(widget);
	client = twitterclient_new_from_config(config, private->cache, lifetime);
	mainwindow_unlock_config(widget);

	return client;
}

MainWindowSyncStatus
mainwindow_get_current_sync_status(GtkWidget *widget)
{
	_MainWindowPrivate *private;
	_MainWindowSyncStep step;
	MainWindowSyncStatus result;

	g_assert(GTK_IS_WINDOW(widget));

	private = MAINWINDOW_GET_DATA(widget);
	g_mutex_lock(private->sync_state.mutex);
	step = private->sync_state.state;
	g_mutex_unlock(private->sync_state.mutex);

	switch(step)
	{
		case MAINWINDOW_SYNC_STEP_NOTICY_TIMELINES:
		case MAINWINDOW_SYNC_STEP_TIMELINES:
			result = MAINWINDOW_SYNC_STATUS_SYNC_MESSAGES;
			break;

		case MAINWINDOW_SYNC_STEP_NOTICY_LISTS:
		case MAINWINDOW_SYNC_STEP_LISTS:
			result = MAINWINDOW_SYNC_STATUS_SYNC_LISTS;
			break;

		case MAINWINDOW_SYNC_STEP_NOTICY_DIRECT_MESSAGES:
		case MAINWINDOW_SYNC_STEP_DIRECT_MESSAGES:
			result = MAINWINDOW_SYNC_STATUS_SYNC_DIRECT_MESSAGES;
			break;

		case MAINWINDOW_SYNC_STEP_NOTICY_FOLLOWERS:
		case MAINWINDOW_SYNC_STEP_FOLLOWERS:
			result = MAINWINDOW_SYNC_STATUS_SYNC_FOLLOWERS;
			break;

		case MAINWINDOW_SYNC_STEP_GUI:
			result = MAINWINDOW_SYNC_STATUS_SYNC_GUI;
			break;

		default:
			result = MAINWINDOW_SYNC_STATUS_IDLE;
	}

	return result;
}

void
mainwindow_sync_timelines(GtkWidget *widget)
{
	_mainwindow_send_sync_message(widget, MAINWINDOW_SYNC_EVENT_UPDATE_TIMELINES);
}

void
mainwindow_sync_lists(GtkWidget *widget)
{
	_mainwindow_send_sync_message(widget, MAINWINDOW_SYNC_EVENT_UPDATE_LISTS);
}

void
mainwindow_sync_gui(GtkWidget *widget)
{
	_mainwindow_send_sync_message(widget, MAINWINDOW_SYNC_EVENT_UPDATE_GUI);
}

void
mainwindow_update_list(GtkWidget *widget, const gchar *owner, const gchar *old_name, const gchar *new_name, gboolean protected)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	g_debug("Updating list: \"%s\"@\"%s\" => \"%s\"", owner, old_name, new_name);

	/* update accountbrowser */
	g_debug("Updating accountbrowser");
	accountbrowser_update_list(private->accountbrowser, owner, old_name, new_name, protected);

	/* update tabbar & open tab */
	g_debug("Updating tabbar");
	if(g_strcmp0(old_name, new_name))
	{
		tabbar_update_list(private->tabbar, owner, old_name, new_name);
	}
}

void
mainwindow_remove_list(GtkWidget *widget, const gchar *owner, const gchar *listname)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);
	gchar id[192];

	g_debug("Removing list: \"%s\"@\"%s\"", owner, listname);

	/* build id */
	sprintf(id, "%s@%s", owner, listname);

	/* update accountbrowser */
	g_debug("Removing list from accountbrowser");
	accountbrowser_remove_list(private->accountbrowser, owner, listname);

	/* close open tab */
	g_debug("Removing list from tabbar");
	tabbar_close_tab(private->tabbar, TAB_TYPE_ID_LIST, id);
}

void
mainwindow_notify(GtkWidget *widget, NotificationLevel level, const gchar *message)
{
	_MainWindowPrivate *private = MAINWINDOW_GET_DATA(widget);

	notification_area_notify(private->notification_area, level, message);
}

/**
 * @}
 */


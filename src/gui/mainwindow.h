/***************************************************************************
    begin........: May 2010
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
 * \file mainwindow.h
 * \brief The mainwindow.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 29. February 2012
 */

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include "accountbrowser.h"
#include "notification_area.h"
#include "../configuration.h"
#include "../cache.h"
#include "../pixbufloader.h"
#include "../twitterclient.h"

/**
 * @addtogroup Gui
 * @{
 */

/*! The minimum width of the mainwindow. */
#define MAINWINDOW_MINIMUM_WIDTH         600
/*! The minimum height of the mainwindow. */
#define MAINWINDOW_MINIMUM_HEIGHT        400
/*! The update interval.  */
#define MAINWINDOW_UPDATE_INTERVAL       180
/*! Minimum update interval for timelines. */
#define MAINWINDOW_TIMELINE_SYNC_MIN     180
/*! Minimum update interval for lists. */
#define MAINWINDOW_LIST_SYNC_MIN         180
/*! Minimum update interval for list members. */
#define MAINWINDOW_LIST_MEMBERS_SYNC_MIN 600
/*! Minimum update interval for friends. */
#define MAINWINDOW_FRIENDS_SYNC_MIN      1800
/*! Minimum update interval for followers. */
#define MAINWINDOW_FOLLOWERS_SYNC_MIN    1800
/*! Minimum update interval for the GUI. */
#define MAINWINDOW_GUI_SYNC_MIN          5
/*! Default status count. */
#define MAINWINDOW_DEFAULT_STATUS_COUNT  40

/*!
 * \typedef MainWindowSyncStatus
 * \brief Synchronization statuses.
 */
typedef enum
{
	/*! Mainwindow is idle. */
	MAINWINDOW_SYNC_STATUS_IDLE,
	/*! Mainwindow is synchronizing messages. */
	MAINWINDOW_SYNC_STATUS_SYNC_MESSAGES,
	/*! Mainwindow is synchronizing lists. */
	MAINWINDOW_SYNC_STATUS_SYNC_LISTS,
	/*! Mainwindow is synchronizing direct messages. */
	MAINWINDOW_SYNC_STATUS_SYNC_DIRECT_MESSAGES,
	/*! Mainwindow is synchronizing followers. */
	MAINWINDOW_SYNC_STATUS_SYNC_FOLLOWERS,
	/*! Mainwindow is synchronizing gui. */
	MAINWINDOW_SYNC_STATUS_SYNC_GUI
} MainWindowSyncStatus;
/**
 * \param config a Configuration instance
 * \param cache a Cache instance
 *
 * Creates and starts the mainwindow.
 */
void mainwindow_start(Config *config, Cache *cache);

/**
 * \param widget the mainwindow widget
 *
 * Closes the window.
 */
void mainwindow_quit(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 *
 * Shows the window.
 */
void mainwindow_show(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 *
 * Hides the mainwindow.
 */
void mainwindow_hide(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 * \param account name of the account related to the activated node
 * \param type type of the activated node
 * \param text text of the selected node
 *
 * Handles activated nodes from the account browser treeview.
 */
void mainwindow_account_node_activated(GtkWidget *widget, const gchar *account, AccountBrowserTreeViewNodeType type, const gchar *text);

/**
 * \param widget the mainwindow widget
 * \param event_time event time
 * \param account name of the account related to the right-clicked node
 * \param type type of the right-clicked node
 * \param text text of the right-clicked node
 *
 * Handles right-clicked nodes from the account browser treeview.
 */
void mainwindow_account_node_right_clicked(GtkWidget *widget, guint event_time, const gchar *account, AccountBrowserTreeViewNodeType type, const gchar *text);

/**
 * \param widget the mainwindow widget
 * \return a shared Config instance
 *
 * Returns the shared configuration instance and locks it.
 */
Config *mainwindow_lock_config(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 *
 * Unlocks the shared configuration instance.
 */
void mainwindow_unlock_config(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 * \param group name of the callback group
 * \param url url of the image
 * \param callback callback function
 * \param user_data user data
 * \param free_func to free user data
 *
 * Loads an image asynchronously.
 */
void mainwindow_load_pixbuf(GtkWidget *widget, const gchar *group, const gchar *url, PixbufLoaderCallback callback, gpointer user_data, GFreeFunc free_func);

/**
 * \param widget the mainwindow widget
 * \param group name of the callback group
 *
 * Removes all callbacks from the given group.
 */
void mainwindow_remove_pixbuf_group(GtkWidget *widget, const gchar *group);

/**
 * \param widget the mainwindow widget
 * \param lifetime cache item lifetime
 * \return a TwitterClient instance
 *
 * Creates a TwitterClient instance.
 */
TwitterClient *mainwindow_create_twittter_client(GtkWidget *widget, gint lifetime);

/**
 * \param widget the mainwindow widget
 *
 * Gets the current synchronization status.
 */
MainWindowSyncStatus mainwindow_get_current_sync_status(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 *
 * Updates all timelines.
 */
void mainwindow_sync_timelines(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 *
 * Updates all lists.
 */
void mainwindow_sync_lists(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 *
 * Updates the GUI.
 */
void mainwindow_sync_gui(GtkWidget *widget);

/**
 * \param widget the mainwindow widget
 * \param owner owner of the list to update
 * \param old_name the old listname
 * \param new_name new list name
 * \param protected protected
 *
 * Updates a listname.
 */
void mainwindow_notify_list_updated(GtkWidget *widget, const gchar *owner, const gchar *old_name, const gchar *new_name, gboolean protected);

/**
 * \param widget the mainwindow widget
 * \param owner owner of the list to remove
 * \param listname listname name of the list to remove
 *
 * Removes a list.
 */
void mainwindow_notify_list_removed(GtkWidget *widget, const gchar *owner, const gchar *listname);

/**
 * \param widget the mainwindow widget
 * \param query search query
 *
 * Notify mainwindow that a search tab has been opened.
 */
void mainwindow_notify_search_opened(GtkWidget *widget, const gchar *query);

/**
 * \param widget the mainwindow widget
 * \param query search query
 *
 * Notify mainwindow that a search tab has been closed.
 */
void mainwindow_notify_search_closed(GtkWidget *widget, const gchar *query);

/**
 * \param widget the mainwindow widget
 * \param query search query
 *
 * Notify mainwindow that a user timeline tab has been opened.
 */
void mainwindow_notify_user_timeline_opened(GtkWidget *widget, const gchar *user);

/**
 * \param widget the mainwindow widget
 * \param query search query
 *
 * Notify mainwindow that an user timeline tab has been closed.
 */
void mainwindow_notify_user_timeline_closed(GtkWidget *widget, const gchar *query);

/**
 * \param widget the mainwindow widget
 * \param level notification level
 * \param message message to display
 *
 * Shows a notification.
 */
void mainwindow_notify(GtkWidget *widget, NotificationLevel level, const gchar *message);

/**
 * \param widget the mainwindow widget
 * \param title window title
 *
 * Sets the window title.
 */
void mainwindow_set_title(GtkWidget *widget, const gchar *title);

/**
 * @}
 */
#endif


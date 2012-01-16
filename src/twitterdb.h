/***************************************************************************
    begin........: November 2010
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
 * \file twitterdb.h
 * \brief Store data from Twitter.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 16. January 2012
 */

#ifndef __TWITTERDB_H__
#define __TWITTERDB_H__

#include "twitter.h"

#include <glib.h>
#include <gio/gio.h>

#include "sqlite3/sqlite3.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/*! Synchronization sources. */
typedef enum
{
	TWITTERDB_SYNC_SOURCE_TIMELINES,
	TWITTERDB_SYNC_SOURCE_LISTS,
	TWITTERDB_SYNC_SOURCE_LIST_MEMBERS,
	TWITTERDB_SYNC_SOURCE_DIRECT_MESSAGES,
	TWITTERDB_SYNC_SOURCE_FRIENDS,
	TWITTERDB_SYNC_SOURCE_FOLLOWERS
} TwitterDbSyncSource;

/*! A database handle. */
typedef sqlite3 TwitterDbHandle;

/**
 * \param err structure for storing error messages
 * \return a database handle or NULL on failure
 *
 * Open a connection to the database.
 */
TwitterDbHandle *twitterdb_get_handle(GError **err);

/**
 * \param handle a database handle
 *
 * Closes a database connection.
 */
void twitterdb_close_handle(TwitterDbHandle *handle);

/**
 * \param handle a database handle
 * \param table name of the table to check
 * \param err structure for storing error messages
 * \return TRUE if the table does exist
 *
 * Checks if a table does exist.
 */
gboolean twitterdb_table_exists(TwitterDbHandle *handle, const gchar *table, GError **err);

/**
 * \param handle a database handle
 * \param major_version database model major version
 * \param minor_version database minor major version
 * \param set_version set database version
 * \param err structure for storing failure messages
 * \return TRUE on success
 *
 * Prepares required tables.
 */
gboolean twitterdb_init(TwitterDbHandle *handle, gint major_version, gint minor_version, gboolean set_version, GError **err);

/**
 * \param handle a database handle
 * \param major_version location to store database model major version
 * \param minor_version location to store database minor major version
 * \param err structure for storing failure messages
 * \return TRUE on success
 *
 * Prepares required tables.
 */
gboolean twitterdb_get_version(TwitterDbHandle *handle, gint *major_version, gint *minor_version, GError **err);

/**
 * \param handle a database handle
 * \param username name of the user
 * \param guid buffer to store the mapped guid
 * \param size size of the buffer
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Maps a username.
 */
gboolean twitterdb_map_username(TwitterDbHandle *handle, const gchar *username, gchar *guid, gint size, GError **err);

/**
 * \param handle a database handle
 * \param guid guid of the user
 * \param username name of the user
 * \param realname realname of the user
 * \param image image of the user
 * \param location location of the user
 * \param website website of the user
 * \param description description of the user
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Saves an user in the database.
 */
gboolean twitterdb_save_user(TwitterDbHandle *handle, const gchar * restrict guid, const gchar * restrict username, const gchar * restrict realname, 
                             const gchar * restrict image, const gchar * restrict location, const gchar * restrict website, const gchar * restrict description,
                             GError **err);

/**
 * \param handle a database handle
 * \param username name of the user to get
 * \param user location to store user details
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Gets an user from the database.
 */
gboolean twitterdb_get_user_by_name(TwitterDbHandle *handle, const gchar *username, TwitterUser *user, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an user
 * \param err structure for storing error messages
 * \return TRUE if given user does exist
 *
 * Checks if an user does exist.
 */
gboolean twitterdb_user_exists(TwitterDbHandle *handle, const gchar *user_guid, GError **err);

/**
 * \param handle a database handle
 * \param guid guid of the status
 * \param prev_status guid of the previous status
 * \param user_guid guid of the owner
 * \param text text of the status
 * \param timestamp timestamp of the status
 * \param count location to store the amount of new messages
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Saves a status in the database.
 */
gboolean twitterdb_save_status(TwitterDbHandle *handle, const gchar * restrict guid, const gchar * restrict prev_status, const gchar * restrict user_guid,
                               const gchar * restrict text, gint64 timestamp, gint *count, GError **err);

/**
 * \param handle a database handle
 * \param guid guid of a status
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Removes a status from the database.
 */

gboolean twitterdb_remove_status(TwitterDbHandle *handle, const gchar *guid, GError **err);

/**
 * \param handle a database handle
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Marks all stati read.
 */
gboolean twitterdb_mark_statuses_read(TwitterDbHandle *handle, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an user
 * \param err structure for storing error messages
 * \return a doubly-linked list or NULL
 *
 * Gets all friends of an user.
 */
GList *twitterdb_get_friends(TwitterDbHandle *handle, const gchar *user_guid, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an user
 * \param err structure for storing error messages
 * \return a doubly-linked list or NULL
 *
 * Gets all users following the given user.
 */
GList *twitterdb_get_followers(TwitterDbHandle *handle, const gchar *user_guid, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an user
 * \param get_friends TRUE to get friends
 * \param func function invoked for each found follower
 * \param user_data data passed to the given callback function
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Gets all friends/followers of the given user.
 */
gboolean twitterdb_foreach_follower(TwitterDbHandle *handle, const gchar *user_guid, gboolean get_friends, TwitterProcessUserFunc func, gpointer user_data, GError **err);

/**
 * \param handle a database handle
 * \param user1_guid guid of an user
 * \param user2_guid guid of the user who follows user1
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Adds a follower information to the database.
 */
gboolean twitterdb_add_follower(TwitterDbHandle *handle, const gchar * restrict user1_guid, const gchar * restrict user2_guid, GError **err);

/**
 * \param handle a database handle
 * \param user1_guid guid of an user
 * \param user2_guid guid of the user who follows user1
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Removes a follower information from the database.
 */
gboolean twitterdb_remove_follower(TwitterDbHandle *handle, const gchar * restrict user1_guid, const gchar * restrict user2_guid, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an user
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Removes all friends of the given user.
 */
gboolean twitterdb_remove_friends(TwitterDbHandle *handle, const gchar *user_guid, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an user
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Removes all followers of the given user.
 */
gboolean twitterdb_remove_followers(TwitterDbHandle *handle, const gchar *user_guid, GError **err);

/**
 * \param handle a database handle
 * \param user1_guid guid of an user
 * \param user2_guid guid of an user
 * \param err structure for storing error messages
 * \return TRUE if user1 followers user2
 *
 * Checks if an user1 follows user2.
 */
gboolean twitterdb_is_follower(TwitterDbHandle *handle, const gchar * restrict user1_guid, const gchar * restrict user2_guid, GError **err);

/**
 * \param handle a database handle
 * \param guid guid of the list
 * \param user_guid guid of the owner
 * \param listname name of the list
 * \param fullname fullname of the list
 * \param uri uri of the list
 * \param description description of the list
 * \param protected indicates if the list is protected
 * \param subscriber_count number of subscribers
 * \param member_count number of members 
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Saves a list in the database.
 */
gboolean twitterdb_save_list(TwitterDbHandle *handle, const gchar * restrict guid, const gchar * restrict user_guid, const gchar * restrict listname,
                             const gchar * restrict fullname, const gchar * restrict uri, const gchar * restrict description, gboolean protected,
                             gint subscriber_count, gint member_count, GError **err);

/**
 * \param handle a database handle
 * \param guid guid of the list
 * \param err structure for storing error messages
 * \return TRUE on succcess
 *
 * Removes a list from the database.
 */
gboolean twitterdb_remove_list(TwitterDbHandle *handle, const gchar *guid, GError **err);

/**
 * \param handle a database handle
 * \param owner name of a list owner
 * \param listname name of a list
 * \param guid buffer to store the mapped guid
 * \param size size of the buffer
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Maps a list's name.
 */
gboolean twitterdb_map_listname(TwitterDbHandle *handle, const gchar * restrict owner, const gchar * restrict listname, gchar *guid, gint size, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of the related owner
 * \param user location to store user details
 * \param err structure for storing error messages
 * \return a new allocated list or NULL
 *
 * Generates a linked list containing TwitterList elements. Elements are created with the GSlice allocator.
 */
GList *twitterdb_get_lists(TwitterDbHandle *handle, const gchar *user_guid, TwitterUser *user, GError **err);

/**
 * \param handle a database handle
 * \param owner guid name of the list owner
 * \param listname name of the list
 * \param list location to store found list information
 * \param err structure for storing error messages
 * \return TRUE of the list could be found
 *
 * Gets list information from the database.
 */
gboolean twitterdb_get_list(TwitterDbHandle *handle, const gchar * restrict owner, const gchar * restrict listname, TwitterList *list, GError **err);

/**
 * \param list result of twitterdb_get_lists
 *
 * Frees the list returned by twitterdb_get_lists.
 */
void twitterdb_free_get_lists_result(GList *list);

/**
 * \param handle a database handle
 * \param username an username
 * \param accounts location to account names
 * \param lists location to list names
 * \param err structure for storing error messages
 * \return number of found lists or -1 on failure
 *
 * Gets list membership.
 */
gint twitterdb_get_list_membership(TwitterDbHandle *handle, const gchar *username, gchar ***accounts, gchar ***lists, GError **err);

/**
 * \param handle a database handle
 * \param owner a list owner
 * \param listname a list name
 * \param username an username
 * \param err structure for storing error messages
 * \return TRUE if an user is assigned to the given list
 *
 * Checks if an user is assigned to a list.
 */
gboolean twitterdb_user_is_list_member(TwitterDbHandle *handle, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err);

/**
 * \param handle a database handle
 * \param user_guid user guid
 * \param err structure for storing error messages
 * \return a list containing list guids
 *
 * Gets all lists where given user is listed.
 */
gchar **twitterdb_get_lists_following_user(TwitterDbHandle *handle, const gchar *user_guid, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an account
 * \param status_guid guid of the status to append
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Appends a status to a public timeline.
 */
gboolean twitterdb_append_status_to_public_timeline(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an account
 * \param status_guid guid of the status to append
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Appends a status to the list of private messages.
 */
gboolean twitterdb_append_status_to_direct_messages(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an account
 * \param status_guid guid of the status to append
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Appends a status to the list of replies.
 */
gboolean twitterdb_append_status_to_replies(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, GError **err);

/**
 * \param handle a database handle
 * \param user_guid guid of an account
 * \param status_guid guid of the status to append
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Appends a status to an user timeline.
 */
gboolean twitterdb_append_status_to_user_timeline(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, GError **err);

/**
 * \param handle a database handle
 * \param list_guid guid of a list
 * \param status_guid guid of the status to append
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Appends a status to a list.
 */
gboolean twitterdb_append_status_to_list(TwitterDbHandle *handle, const gchar * restrict list_guid, const gchar * restrict status_guid, GError **err);

/**
 * \param handle a database handle
 * \param guid status guid
 * \param err structure for storing error messages
 * \return TRUE if status does exist
 *
 * Tests if a status does exist.
 */
gboolean twitterdb_status_exists(TwitterDbHandle *handle, const gchar *guid, GError **err);

/**
 * \param handle a database handle
 * \param guid status guid
 * \param status location to store status information
 * \param user location to store user information
 * \param err structure for storing error messages
 * \return TRUE if status does exist
 *
 * Gets a status from the database.
 */
gboolean twitterdb_get_status(TwitterDbHandle *handle, const gchar *guid, TwitterStatus *status, TwitterUser *user, GError **err);

/**
 * \param handle a database handle
 * \param list_guid guid of a list
 * \param user_guid guid of an account
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Appends an user to a list.
 */
gboolean twitterdb_append_user_to_list(TwitterDbHandle *handle, const gchar * restrict list_guid, const gchar * restrict user_guid, GError **err);

/**
 * \param handle a database handle
 * \param list_guid guid of a list
 * \param user_guid guid of an account
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Removes an user from a list.
 */
gboolean twitterdb_remove_user_from_list(TwitterDbHandle *handle, const gchar * restrict list_guid, const gchar * restrict user_guid, GError **err);

/**
 * \param handle a database handle
 * \param list_guid guid of a list
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Removes all users from a list.
 */
gboolean twitterdb_remove_users_from_list(TwitterDbHandle *handle, const gchar *list_guid, GError **err);

/**
 * \param handle a database handle
 * \param owner list owner
 * \param listname name of the list
 * \param err structure for storing error messages
 * \return TRUE if the list does exist.
 *
 * Checks if a list does exist.
 */
gboolean twitterdb_list_exists(TwitterDbHandle *handle, const gchar * restrict owner, const gchar * restrict listname, GError **err);

/**
 * \param handle a database handle
 * \param guid guid of the message
 * \param text text of the message
 * \param timestamp timestamp of the message
 * \param sender_guid guid of the sender
 * \param receiver_guid guid of the receiver 
 * \param count location to store the amount of new messages
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Saves a direct message.
 */
gboolean twitterdb_save_direct_message(TwitterDbHandle *handle, const gchar * restrict guid, const gchar * restrict text, gint64 timestamp,
                                       const gchar * restrict sender_guid, const gchar * restrict receiver_guid, gint *count, GError **err);

/**
 * \param handle a database handle
 * \param username a username 
 * \param func function invoked for each found status
 * \param ignore_old ignore old tweets
 * \param user_data data passed to the given callback function
 * \param cancellable a GCancellable to abort the database operation
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Gets tweets from public timeline.
 */
gboolean twitterdb_foreach_status_in_public_timeline(TwitterDbHandle *handle, const gchar *username, TwitterProcessStatusFunc func, gboolean ignore_old, gpointer user_data, GCancellable *cancellable, GError **err);

/**
 * \param handle a database handle
 * \param username a username 
 * \param func function invoked for each found status
 * \param ignore_old ignore old tweets
 * \param user_data data passed to the given callback function
 * \param cancellable a GCancellable to abort the database operation
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Gets replies.
 */
gboolean twitterdb_foreach_status_in_replies(TwitterDbHandle *handle, const gchar *username, TwitterProcessStatusFunc func, gboolean ignore_old, gpointer user_data, GCancellable *cancellable, GError **err);

/**
 * \param handle a database handle
 * \param username a username 
 * \param func function invoked for each found status
 * \param ignore_old ignore old tweets
 * \param user_data data passed to the given callback function
 * \param cancellable a GCancellable to abort the database operation
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Gets a user timline.
 */
gboolean twitterdb_foreach_status_in_usertimeline(TwitterDbHandle *handle, const gchar *username, TwitterProcessStatusFunc func, gboolean ignore_old, gpointer user_data, GCancellable *cancellable, GError **err);

/**
 * \param handle a database handle
 * \param username a username 
 * \param list a list
 * \param func function invoked for each found status
 * \param user_data data passed to the given callback function
 * \param cancellable a GCancellable to abort the database operation
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Gets a list timline.
 */
gboolean twitterdb_foreach_status_in_list(TwitterDbHandle *handle, const gchar * restrict username, const gchar * restrict list, TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err);

/**
 * \param handle a database handle
 * \param username a username 
 * \param list a list
 * \param func function invoked for each found status
 * \param user_data data passed to the given callback function
 * \param cancellable a GCancellable to abort the database operation
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Gets list members.
 */
gboolean twitterdb_foreach_list_member(TwitterDbHandle *handle, const gchar * restrict username, const gchar * restrict list, TwitterProcessListMemberFunc func, gpointer user_data, GCancellable *cancellable, GError **err);

/**
 * \param handle a database handle
 * \param list_guid guid of the list
 * \param err structure for storing error messages
 * \return number of found tweets
 *
 * Counts tweets of a list.
 */
gint twitterdb_count_tweets_from_list(TwitterDbHandle *handle, const gchar *list_guid, GError **err);

/**
 * \param handle a database handle
 * \param source the source
 * \param user_guid guid of the user
 * \param default_val default value
 * \return last synchronization timestamp or default_val
 *
 * Gets the last synchronization timestamp of the given source and user.
 */
gint twitterdb_get_last_sync(TwitterDbHandle *handle, TwitterDbSyncSource source, const gchar *user_guid, gint default_val);

/**
 * \param handle a database handle
 * \param source the source
 * \param user_guid guid of the user
 * \param seconds seconds to set
 *
 * Sets the last synchronization timestamp of the given source.
 */
void twitterdb_set_last_sync(TwitterDbHandle *handle, TwitterDbSyncSource source, const gchar *user_guid, gint seconds);

/**
 * \param handle a database handle
 * \param source the source
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Removes all timestamps related to the given source.
 */
gboolean twitterdb_remove_last_sync_source(TwitterDbHandle *handle, TwitterDbSyncSource source, GError **err);

/**
 * \param handle a database handle
 * \param err structure for storing error messages
 * \return TRUE on success
 *
 * Upgrades database format from version 0.1 to 0.2.
 */
gboolean twitterdb_upgrade_0_1_to_0_2(TwitterDbHandle *handle, GError **err);

/**
 * @}
 * @}
 */
#endif


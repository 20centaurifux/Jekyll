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
 * \file twitterdb_queries.h
 * \brief SQL Query collection.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 16. January 2012
 */

#ifndef __TWITTERDB_QUERIES_H__
#define __TWITTERDB_QUERIES_H__

#include <glib.h>

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/*! Creates tables. */
extern const gchar *twitterdb_queries_create_tables[];
/*! Replaces the database model version. */
extern const gchar *twitterdb_queries_replace_version;
/*! Checks if a table does exist. */
extern const gchar *twitterdb_queries_count_table;
/*! Gets database model version. */
extern const gchar *twitterdb_queries_get_version;
/*! Gets an user filtered by guid. */
extern const gchar *twitterdb_queries_get_user;
/*! Gets the guid of an user filterered by username. */
extern const gchar *twitterdb_queries_map_username;
/*! Counts users filterered by guid. */
extern const gchar *twitterdb_queries_user_exists;
/*! Creates a new user. */
extern const gchar *twitterdb_queries_insert_user;
/*! Updates an user. */
extern const gchar *twitterdb_queries_update_user;
/*! Marks all statuses read. */
extern const gchar *twitterdb_queries_mark_statuses_read;
/*! Counts statuses filtered by guid. */
extern const gchar *twitterdb_queries_status_exists;
/*! Creates a new status. */
extern const gchar *twitterdb_queries_insert_status;
/*! Deletes an status. */
extern const gchar *twitterdb_queries_delete_status;
/*! Gets a status. */
extern const gchar *twitterdb_queries_get_status;
/*! Creates follower information. */
extern const gchar *twitterdb_queries_replace_follower;
/*! Removes a follower. */
extern const gchar *twitterdb_queries_delete_follower;
/*! Gets all friends of an user. */
extern const gchar *twitterdb_queries_get_friends;
/*! Gets all followers of an user. */
extern const gchar *twitterdb_queries_get_followers;
/*! Removes all followers from an user. */
extern const gchar *twitterdb_queries_remove_followers_from_user;
/*! Tests if one user is following another user. */
extern const gchar *twitterdb_queries_is_follower;
/*! Removes all friends of an user. */
extern const gchar *twitterdb_queries_remove_friends_from_user;
/*! Gets a list filtered by its guid. */
extern const gchar *twitterdb_queries_get_list;
/*! Gets list guids filtered by followed user. */
extern const gchar *twitterdb_queries_get_lists_following_user;
/*! Counts tweets from a list. */
extern const gchar *twitterdb_queries_count_tweets_from_list;
/*! Creates a new list. */
extern const gchar *twitterdb_queries_insert_list;
/*! Updates a list. */
extern const gchar *twitterdb_queries_update_list;
/*! Counts lists filterered by guid. */
extern const gchar *twitterdb_queries_list_exists;
/*! Deletes a list. */
extern const gchar *twitterdb_queries_delete_list;
/*! Gets a guid of a list. */
extern const gchar *twitterdb_queries_get_list_guid;
/*! Gets the guids of all lists assigned to an user. */
extern const gchar *twitterdb_queries_get_list_guids_from_user;
/*! Removes all obsolete statuses from a list. */
extern const gchar *twitterdb_queries_remove_obsolete_statuses_from_list;
/*! Inserts a status into a timeline. */
extern const gchar *twitterdb_queries_replace_status_into_timeline;
/*! Inserts a status into the timeline of a list. */
extern const gchar *twitterdb_queries_replace_into_list_timeline;
/*! Deletes all members of a list. */
extern const gchar *twitterdb_queries_delete_list_members;
/*! Assignes an user as list member. */
extern const gchar *twitterdb_queries_replace_into_list_member;
/*! Gets all list members. */
extern const gchar *twitterdb_queries_get_list_members;
/*! Deletes a user from a list. */
extern const gchar *twitterdb_queries_delete_user_from_list;
/*! Counts direct messages filtered by guid. */
extern const gchar *twitterdb_queries_direct_message_exists;
/*! Creates a new direct message. */
extern const gchar *twitterdb_queries_insert_direct_message;
/*! Gets all statuses from a timeline. */
extern const gchar *twitterdb_queries_get_tweets_from_timeline;
/*! Gets all statuses from a timeline which are marked as new. */
extern const gchar *twitterdb_queries_get_new_tweets_from_timeline;
/*! Gets all statuses from a list. */
extern const gchar *twitterdb_queries_get_tweets_from_list;
/*! Gets the lists of an user. */
extern const gchar *twitterdb_queries_get_list_membership;
/*! Checks of an user is assigned to a list. */
extern const gchar *twitterdb_queries_user_is_list_member;
/*! Get synchronization timestamp. */
extern const gchar *twitterdb_queries_get_sync_seconds;
/*! Set synchronization timestamp. */
extern const gchar *twitterdb_queries_replace_sync_seconds;
/*! Delete last synchronization timestamps related to a source. */
extern const gchar *twitterdb_queries_remove_sync_seconds;
/*! Add prev_status column to status table, */
extern const gchar *twitterdb_queries_add_prev_status_column;

/**
 * @}
 * @}
 */
#endif


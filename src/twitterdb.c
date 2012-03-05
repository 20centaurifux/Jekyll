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
 * \file twitterdb.c
 * \brief Store data from Twitter.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 16. January 2012
 */

#include <string.h>

#include "twitterdb.h"
#include "twitterdb_queries.h"
#include "application.h"
#include "pathbuilder.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

enum
{
	TWITTERDB_TIMELINE_TYPE_PUBLIC = 1,
	TWITTERDB_TIMELINE_TYPE_USER_TIMELINE = 2,
	TWITTERDB_TIMELINE_TYPE_REPLIES = 3
};

static GStaticMutex mutex_twitterdb = G_STATIC_MUTEX_INIT;

/*
 *	helpers:
 */
/*! Copy string from sqlite3_column to destination buffer. */
#define TWITTERDB_COPY_TEXT_COLUMN(dest, index, size) if(sqlite3_column_text(stmt, index)) g_strlcpy(dest, (const gchar *)sqlite3_column_text(stmt, index), size)

static void
_twitterdb_set_error(GError **err, TwitterDbHandle *handle)
{
	g_assert(handle != NULL);

	if(err)
	{
		g_set_error(err, 0, 0, "database failure: \"%s\"", sqlite3_errmsg(handle));
	}
}

static void
_twitterdb_set_error_from_string(GError **err, const gchar *message)
{
	if(err)
	{
		g_set_error(err, 0, 0, "%s", message);
	}
}

static gboolean
_twitterdb_execute_non_query(TwitterDbHandle *handle, const gchar *query, GError **err)
{
	gchar *message = NULL;
	gboolean result = FALSE;

	g_assert(handle != NULL);
	g_assert(query != NULL);

	if(sqlite3_exec(handle, query, NULL, NULL, &message) == SQLITE_OK)
	{
		result = TRUE;
	}
	else
	{
		_twitterdb_set_error_from_string(err, message);
	}

	if(message)
	{
		sqlite3_free(message);
	}

	return result;
}

static gboolean
_twitterdb_prepare_statement(TwitterDbHandle *handle, const gchar *sql, sqlite3_stmt **stmt, GError **err)
{
	gint result;
	gboolean success = TRUE;

	g_assert(handle != NULL);
	g_assert(stmt != NULL);

	do
	{
		result = sqlite3_prepare_v2(handle, sql, -1, stmt, NULL);

		if(result == SQLITE_LOCKED || result == SQLITE_BUSY)
		{
			g_usleep(50000);
		}

	} while(result == SQLITE_LOCKED || result == SQLITE_BUSY);

	if(result != SQLITE_OK)
	{
		_twitterdb_set_error(err, handle);
		success = FALSE;
	}

	return success;
}

static gint
_twitterdb_execute_statement(TwitterDbHandle *handle, sqlite3_stmt *stmt, gboolean retry, GError **err)
{
	gint result;

	g_assert(handle != NULL);
	g_assert(stmt != NULL);

	switch((result = sqlite3_step(stmt)))
	{
		case SQLITE_BUSY:
			if(retry)
			{
				g_usleep(5000000);
				return _twitterdb_execute_statement(handle, stmt, retry, err);
			}
			break;

		case SQLITE_DONE:
		case SQLITE_ROW:
			break;

		case SQLITE_ERROR:
			_twitterdb_set_error(err, handle);
			break;

		default:
			g_set_error(err, 0, 0, "Couldn't execute statement: \"%s\"", sqlite3_errmsg(handle));
	}

	return result;
}

static gboolean
_twitterdb_update_follower(TwitterDbHandle *handle, const gchar * restrict user1_guid, const gchar * restrict user2_guid, gboolean remove, GError **err)
{
	sqlite3_stmt *stmt;
	const gchar *query;
	gboolean result = FALSE;

	if(remove)
	{
		query = twitterdb_queries_delete_follower;
	}
	else
	{
		query = twitterdb_queries_replace_follower;
	}

	if(_twitterdb_prepare_statement(handle, query, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, user1_guid, -1, NULL);
		sqlite3_bind_text(stmt, 2, user2_guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	return result;
}

static gboolean
_twitterdb_remove_followers(TwitterDbHandle *handle, const gchar *user_guid, gboolean friend, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	if(_twitterdb_prepare_statement(handle, friend ? twitterdb_queries_remove_friends_from_user : twitterdb_queries_remove_followers_from_user, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, user_guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	return result;
}

static gboolean
_twitterdb_add_status_to_timeline(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, gint type, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_replace_status_into_timeline, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, user_guid, -1, NULL);
		sqlite3_bind_text(stmt, 2, status_guid, -1, NULL);
		sqlite3_bind_int(stmt, 3, type);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	return result;
}

static gboolean
_twitterdb_update_list_member(TwitterDbHandle *handle, const gchar * restrict list_guid, const gchar * restrict user_guid, gboolean remove, GError **err)
{
	sqlite3_stmt *stmt;
	const gchar *query;
	gboolean result = FALSE;

	if(remove)
	{
		query = twitterdb_queries_delete_user_from_list;
	}
	else
	{
		query = twitterdb_queries_replace_into_list_member;
	}

	if(_twitterdb_prepare_statement(handle, query, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, list_guid, -1, NULL);
		sqlite3_bind_text(stmt, 2, user_guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	return result;
}

static gboolean
_twitterdb_get_user(TwitterDbHandle *handle, const gchar *guid, TwitterUser *user, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_user, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			g_strlcpy(user->id, guid, 32);
			TWITTERDB_COPY_TEXT_COLUMN(user->screen_name, 0, 64);
			TWITTERDB_COPY_TEXT_COLUMN(user->name, 1, 64);
			TWITTERDB_COPY_TEXT_COLUMN(user->image, 2, 256);
			TWITTERDB_COPY_TEXT_COLUMN(user->location, 3, 64);
			TWITTERDB_COPY_TEXT_COLUMN(user->url, 4, 256);
			TWITTERDB_COPY_TEXT_COLUMN(user->description, 5, 280);
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	return result;
}

static gboolean
_twitterdb_get_list(TwitterDbHandle *handle, const gchar *guid, TwitterList *list, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_list, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			TWITTERDB_COPY_TEXT_COLUMN(list->id, 0, 32);
			TWITTERDB_COPY_TEXT_COLUMN(list->name, 1, 64);
			TWITTERDB_COPY_TEXT_COLUMN(list->fullname, 2, 64);
			TWITTERDB_COPY_TEXT_COLUMN(list->uri, 3, 256);
			TWITTERDB_COPY_TEXT_COLUMN(list->description, 4, 280);
			list->protected = (sqlite3_column_int(stmt, 5)) ? TRUE : FALSE;
			list->subscriber_count = sqlite3_column_int(stmt, 6);
			list->member_count = sqlite3_column_int(stmt, 7);
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	return result;
}

static gboolean
_twitterdb_fetch_tweets(TwitterDbHandle *handle, sqlite3_stmt *stmt, GList **tweets, GList **users, GCancellable *cancellable, GError **err)
{
	gint status;
	TwitterStatus *tweet;
	TwitterUser *user;
	gint result = TRUE;

	while((status = _twitterdb_execute_statement(handle, stmt, TRUE, err)) != SQLITE_DONE)
	{
		if(cancellable && g_cancellable_is_cancelled(cancellable))
		{
			g_debug("%s: cancelled", __func__);
			result = FALSE;
			break;
		}

		if(status == SQLITE_ROW)
		{
			tweet = (TwitterStatus *)g_slice_alloc(sizeof(TwitterStatus));
			user = (TwitterUser *)g_slice_alloc(sizeof(TwitterUser));

			/* fill status & user structure */
			TWITTERDB_COPY_TEXT_COLUMN(tweet->id, 0, 32);
			TWITTERDB_COPY_TEXT_COLUMN(tweet->text, 1, 280);
			tweet->timestamp = sqlite3_column_int(stmt, 2);
			TWITTERDB_COPY_TEXT_COLUMN(user->id, 4, 32);
			TWITTERDB_COPY_TEXT_COLUMN(user->screen_name, 5, 64);
			TWITTERDB_COPY_TEXT_COLUMN(user->name, 6, 64);
			TWITTERDB_COPY_TEXT_COLUMN(user->image, 7, 256);
			TWITTERDB_COPY_TEXT_COLUMN(user->location, 8, 64);
			TWITTERDB_COPY_TEXT_COLUMN(user->url, 9, 256);
			TWITTERDB_COPY_TEXT_COLUMN(user->description, 10, 280);
			TWITTERDB_COPY_TEXT_COLUMN(tweet->prev_status, 11, 32);

			*tweets = g_list_append(*tweets, tweet);
			*users = g_list_append(*users, user);
		}
		else
		{
			result = FALSE;
			break;
		}
	}

	return result;
}

static void
_twitterdb_foreach_status(GList *tweets, GList *users, TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable)
{
	GList *iter_tweets = tweets;
	GList *iter_users = users;
	iter_tweets = tweets;
	iter_users = users;
	TwitterStatus *tweet;
	TwitterUser *user;

	while(iter_tweets && (!cancellable || !g_cancellable_is_cancelled(cancellable)))
	{
		tweet = (TwitterStatus *)iter_tweets->data;
		user = (TwitterUser *)iter_users->data;

		func(*tweet, *user, user_data);

		iter_tweets = iter_tweets->next;
		iter_users = iter_users->next;
	}
}

static void
_twitterdb_free_list(GList *list, gint element_size)
{
	GList *iter = list;

	while(iter)
	{
		g_slice_free1(element_size, iter->data);
		iter = iter->next;
	}

	g_list_free(list);
}

static gboolean
_twitterdb_foreach_status_in_timeline_locked(TwitterDbHandle *handle, const gchar *username, gint list, TwitterProcessStatusFunc func, gboolean ignore_old, gpointer user_data, GCancellable *cancellable, GError **err)
{
	sqlite3_stmt *stmt;
	GList *tweets = NULL;
	GList *users = NULL;
	gboolean result = FALSE;

	g_assert(handle != NULL);
	g_assert(username != NULL);
	g_assert(func != NULL);

	g_static_mutex_lock(&mutex_twitterdb);

	/* get tweets */
	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_tweets_from_timeline, &stmt, err))
	{
		sqlite3_bind_int(stmt, 1, list);
		sqlite3_bind_text(stmt, 2, username, -1, NULL);
		result = TRUE;

		result = _twitterdb_fetch_tweets(handle, stmt, &tweets, &users, cancellable, err);
		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	/* iterate result */
	if(result)
	{
		_twitterdb_foreach_status(tweets, users, func, user_data, cancellable);
	}

	_twitterdb_free_list(tweets, sizeof(TwitterStatus));
	_twitterdb_free_list(users, sizeof(TwitterUser));

	return result;
}

static GList *
_twitterdb_get_followers(TwitterDbHandle *handle, const gchar *user_guid, gboolean friends, GError **err)
{
	sqlite3_stmt *stmt;
	gint status;
	GList *followers = NULL;
	gboolean success = FALSE;

	g_assert(handle != NULL);
	g_assert(user_guid != NULL);

	/* get follower guids from database */
	if(_twitterdb_prepare_statement(handle, friends ? twitterdb_queries_get_friends : twitterdb_queries_get_followers, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, user_guid, -1, NULL);
		success = TRUE;

		while((status = _twitterdb_execute_statement(handle, stmt, TRUE, err)) != SQLITE_DONE)
		{
			if(status == SQLITE_ROW)
			{
				/* get list details */
				followers = g_list_append(followers, g_strdup((const gchar *)sqlite3_column_text(stmt, 0)));
			}
			else
			{
				success = FALSE;
				break;
			}
		}

		sqlite3_finalize(stmt);
	}

	/* free list on failure */
	if(!success && followers)
	{
		g_list_foreach(followers, (GFunc)&g_free, NULL);
		g_list_free(followers);
		followers = NULL;
	}

	return followers;
}

/*
 *	public:
 */
TwitterDbHandle *
twitterdb_get_handle(GError **err)
{
	gchar *directory;
	gchar *filename = NULL;
	sqlite3 *handle = NULL;
	gboolean directory_exists = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	g_debug("Creating new database handle");

	/* test database folder */
	directory = g_build_filename(pathbuilder_get_user_application_directory(), G_DIR_SEPARATOR_S, TWTTER_DATABASE_FOLDER, NULL);
	g_debug("Testing folder: \"%s\"", directory);

	if(g_file_test(directory, G_FILE_TEST_EXISTS))
	{
		if(!(directory_exists = g_file_test(directory, G_FILE_TEST_IS_DIR)))
		{
			g_set_error(err, 0, 0, "Specified database folder is not a directory: \"%s\"", directory);
		}
	}
	else
	{
		g_debug("Trying to create database folder: \"%s\"", directory);
		if(g_mkdir_with_parents(directory, 500))
		{
			g_set_error(err, 0, 0, "Couldn't create database folder: \"%s\"", directory);
		}
		else
		{
			directory_exists = TRUE;
		}
	}

	/* connect to database */
	if(directory_exists)
	{
		filename = g_build_filename(directory, G_DIR_SEPARATOR_S, TWITTER_DATABASE_FILE, NULL);
		g_debug("Connecting to database: \"%s\"", filename);
	
		if(sqlite3_open_v2(filename, &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_CREATE, NULL) == SQLITE_OK)
		{
			/* activate foreign keys */
			_twitterdb_execute_non_query(handle, "PRAGMA foreign_keys=ON", NULL);

			/* activate asynchronous mode */
			_twitterdb_execute_non_query(handle, "PRAGMA synchronous=OFF", NULL);
		}
		else
		{
			_twitterdb_set_error(err, handle);
			sqlite3_close(handle);
			handle = NULL;
		}
	}

	/* free memory */
	g_free(directory);

	if(filename)
	{
		g_free(filename);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return handle;
}

void
twitterdb_close_handle(TwitterDbHandle *handle)
{
	g_static_mutex_lock(&mutex_twitterdb);

	g_assert(handle != NULL);

	if(sqlite3_close(handle) != SQLITE_OK)
	{
		g_warning("Couldn't close database connection: %s", sqlite3_errmsg(handle));
	}

	g_static_mutex_unlock(&mutex_twitterdb);
}

gboolean
twitterdb_table_exists(TwitterDbHandle *handle, const gchar *table, GError **err)
{
	gint count;
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	g_assert(handle != NULL);

	/* check if table does exist */
	if(_twitterdb_prepare_statement(handle, twitterdb_queries_count_table, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, table, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			if((count = sqlite3_column_int(stmt, 0)))
			{
				result = TRUE;
			}
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_init(TwitterDbHandle *handle, gint major_version, gint minor_version, gboolean set_version, GError **err)
{
	gint i = 0;
	sqlite3_stmt *stmt;
	gboolean result = TRUE;

	g_static_mutex_lock(&mutex_twitterdb);

	while(twitterdb_queries_create_tables[i] && result)
	{
		result = _twitterdb_execute_non_query(handle, twitterdb_queries_create_tables[i], err);
		++i;
	}

	if(result && set_version)
	{
		if(_twitterdb_prepare_statement(handle, twitterdb_queries_replace_version, &stmt, err))
		{
			sqlite3_bind_int(stmt, 1, major_version);
			sqlite3_bind_int(stmt, 2, minor_version);
	
			if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
			{
				result = TRUE;
			}

			sqlite3_finalize(stmt);
		}
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_get_version(TwitterDbHandle *handle, gint *major_version, gint *minor_version, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_version, &stmt, err))
	{
		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			*major_version = (gint)sqlite3_column_int(stmt, 0);
			*minor_version = (gint)sqlite3_column_int(stmt, 1);
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_map_username(TwitterDbHandle *handle, const gchar *username, gchar *guid, gint size, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_map_username, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, username, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			g_strlcpy(guid, (const gchar *)sqlite3_column_text(stmt, 0), size);
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_save_user(TwitterDbHandle *handle, const gchar * restrict guid, const gchar * restrict username, const gchar * restrict realname, 
                   const gchar * restrict image, const gchar * restrict location, const gchar * restrict website, const gchar * restrict description, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean success = FALSE;
	gint count = 0;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	/* check if user does exist */
	if(_twitterdb_prepare_statement(handle, twitterdb_queries_user_exists, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			count = sqlite3_column_int(stmt, 0);
			success = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	if(success)
	{
		/* create or update user */
		success = FALSE;

		if(count == 0)
		{
			g_debug("Creating user (\"%s\")", username);
			if((success = _twitterdb_prepare_statement(handle, twitterdb_queries_insert_user, &stmt, err)))
			{
				sqlite3_bind_text(stmt, 1, guid, -1, NULL);
				sqlite3_bind_text(stmt, 2, username, -1, NULL);
				sqlite3_bind_text(stmt, 3, realname, -1, NULL);
				sqlite3_bind_text(stmt, 4, image, -1, NULL);
				sqlite3_bind_text(stmt, 5, location, -1, NULL);
				sqlite3_bind_text(stmt, 6, website, -1, NULL);
				sqlite3_bind_text(stmt, 7, description, -1, NULL);
			}
		}
		else
		{
			g_debug("Updating user (\"%s\")", username);
			if((success = _twitterdb_prepare_statement(handle, twitterdb_queries_update_user, &stmt, err)))
			{
				sqlite3_bind_text(stmt, 1, username, -1, NULL);
				sqlite3_bind_text(stmt, 2, realname, -1, NULL);
				sqlite3_bind_text(stmt, 3, image, -1, NULL);
				sqlite3_bind_text(stmt, 4, location, -1, NULL);
				sqlite3_bind_text(stmt, 5, website, -1, NULL);
				sqlite3_bind_text(stmt, 6, description, -1, NULL);
				sqlite3_bind_text(stmt, 7, guid, -1, NULL);
			}
		}
	}

	if(success)
	{
		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_get_user_by_name(TwitterDbHandle *handle, const gchar *username, TwitterUser *user, GError **err)
{
	gchar guid[32];
	gboolean result = FALSE;

	if(twitterdb_map_username(handle, username, guid, 32, err))
	{
		g_static_mutex_lock(&mutex_twitterdb);
		result = _twitterdb_get_user(handle, guid, user, err);
		g_static_mutex_unlock(&mutex_twitterdb);
	}

	return result;
}

gboolean
twitterdb_user_exists(TwitterDbHandle *handle, const gchar *user_guid, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_user_exists, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, user_guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			 if(sqlite3_column_int(stmt, 0) > 0)
			 {
				 result = TRUE;
			 }
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_save_status(TwitterDbHandle *handle, const gchar * restrict guid, const gchar * restrict prev_status, const gchar * restrict user_guid, const gchar * restrict text, gint64 timestamp, gint *count, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean exists = FALSE;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	/* check if status does already exist */
	if(_twitterdb_prepare_statement(handle, twitterdb_queries_status_exists, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			if(sqlite3_column_int(stmt, 0) == 0)
			{
				++(*count);
			}
			else
			{
				exists = TRUE;
			}

			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	/* insert status */
	if(!exists && result)
	{
		result = FALSE;

		if(_twitterdb_prepare_statement(handle, twitterdb_queries_insert_status, &stmt, err))
		{
			sqlite3_bind_text(stmt, 1, guid, -1, NULL);
			sqlite3_bind_text(stmt, 2, prev_status, -1, NULL);
			sqlite3_bind_text(stmt, 3, text, -1, NULL);
			sqlite3_bind_text(stmt, 4, user_guid, -1, NULL);
			sqlite3_bind_int(stmt, 5, (sqlite3_int64)timestamp);
			if(!(result = (_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE) ? TRUE : FALSE))
			{
				_twitterdb_set_error(err, handle);
			}

			sqlite3_finalize(stmt);
		}
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_remove_status(TwitterDbHandle *handle, const gchar *guid, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_delete_status, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}
	}

	sqlite3_finalize(stmt);

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_mark_statuses_read(TwitterDbHandle *handle, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_execute_non_query(handle, twitterdb_queries_mark_statuses_read, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

GList *
twitterdb_get_friends(TwitterDbHandle *handle, const gchar *user_guid, GError **err)
{
	GList *result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_get_followers(handle, user_guid, TRUE, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

GList *
twitterdb_get_followers(TwitterDbHandle *handle, const gchar *user_guid, GError **err)
{
	GList *result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_get_followers(handle, user_guid, FALSE, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_foreach_follower(TwitterDbHandle *handle, const gchar *user_guid, gboolean get_friends, TwitterProcessUserFunc func, gpointer user_data, GError **err)
{
	GList *ids;
	GList *iter;
	TwitterUser user;
	gboolean result = TRUE;

	g_static_mutex_lock(&mutex_twitterdb);

	if((iter = ids = _twitterdb_get_followers(handle, user_guid, get_friends, err)))
	{
		while(iter && !*err && result)
		{
			if((result = _twitterdb_get_user(handle, (const gchar *)iter->data, &user, err)))
			{
				func(user, user_data);
			}
					
			iter = iter->next;
		}

		g_list_foreach(ids, (GFunc)&g_free, NULL);
		g_list_free(ids);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	if(result && *err)
	{
		result = FALSE;
	}

	return result;
}

gboolean
twitterdb_add_follower(TwitterDbHandle *handle, const gchar * restrict user1_guid, const gchar * restrict user2_guid, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_update_follower(handle, user1_guid, user2_guid, FALSE, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_remove_follower(TwitterDbHandle *handle, const gchar * restrict user1_guid, const gchar * restrict user2_guid, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_update_follower(handle, user1_guid, user2_guid, TRUE, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_remove_friends(TwitterDbHandle *handle, const gchar *user_guid, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_remove_followers(handle, user_guid, TRUE, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_remove_followers(TwitterDbHandle *handle, const gchar *user_guid, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_remove_followers(handle, user_guid, FALSE, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_is_follower(TwitterDbHandle *handle, const gchar * restrict user1, const gchar * restrict user2, GError **err)
{
	sqlite3_stmt *stmt;
	gint count;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_is_follower, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, user1, -1, NULL);
		sqlite3_bind_text(stmt, 2, user2, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			if((count = sqlite3_column_int(stmt, 0)))
			{
				result = TRUE;
			}
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_save_list(TwitterDbHandle *handle, const gchar * restrict guid, const gchar * restrict user_guid, const gchar * restrict listname, const gchar * restrict fullname,
                    const gchar * restrict uri, const gchar * restrict description, gboolean protected, gint subscriber_count, gint member_count,
                    GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;
	gboolean success = FALSE;
	gint count = 0;

	g_static_mutex_lock(&mutex_twitterdb);

	/* check if list does exist */
	if(_twitterdb_prepare_statement(handle, twitterdb_queries_list_exists, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			count = sqlite3_column_int(stmt, 0);
			success = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	if(success)
	{
		/* create or update list*/
		success = FALSE;

		if(count == 0)
		{
			g_debug("Creating list (\"%s\")", fullname);
			if((success = _twitterdb_prepare_statement(handle, twitterdb_queries_insert_list, &stmt, err)))
			{
				sqlite3_bind_text(stmt, 1, guid, -1, NULL);
				sqlite3_bind_text(stmt, 2, user_guid, -1, NULL);
				sqlite3_bind_text(stmt, 3, listname, -1, NULL);
				sqlite3_bind_text(stmt, 4, fullname, -1, NULL);
				sqlite3_bind_int(stmt, 5, protected);
				sqlite3_bind_text(stmt, 6, uri, -1, NULL);
				sqlite3_bind_text(stmt, 7, description, -1, NULL);
				sqlite3_bind_int(stmt, 8, subscriber_count);
				sqlite3_bind_int(stmt, 9, member_count);
			}
		}
		else
		{
			g_debug("Updating list (\"%s\")", fullname);
			if((success = _twitterdb_prepare_statement(handle, twitterdb_queries_update_list, &stmt, err)))
			{
				sqlite3_bind_text(stmt, 1, user_guid, -1, NULL);
				sqlite3_bind_text(stmt, 2, listname, -1, NULL);
				sqlite3_bind_text(stmt, 3, fullname, -1, NULL);
				sqlite3_bind_int(stmt, 4, protected);
				sqlite3_bind_text(stmt, 5, uri, -1, NULL);
				sqlite3_bind_text(stmt, 6, description, -1, NULL);
				sqlite3_bind_int(stmt, 7, subscriber_count);
				sqlite3_bind_int(stmt, 8, member_count);
				sqlite3_bind_text(stmt, 9, guid, -1, NULL);
			}
		}


		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}
	}

	if(success)
	{
		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_remove_list(TwitterDbHandle *handle, const gchar *guid, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_delete_list, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_map_listname(TwitterDbHandle *handle, const gchar * restrict owner, const gchar * restrict listname, gchar *guid, gint size, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_list_guid, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, listname, -1, NULL);
		sqlite3_bind_text(stmt, 2, owner, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			g_strlcpy(guid, (const gchar *)sqlite3_column_text(stmt, 0), size);
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

GList *
twitterdb_get_lists(TwitterDbHandle *handle, const gchar *user_guid, TwitterUser *user, GError **err)
{
	sqlite3_stmt *stmt;
	gint status;
	TwitterList *list;
	GList *lists = NULL;
	static gint list_size = 0;
	gboolean success = FALSE;

	g_assert(handle != NULL);
	g_assert(user_guid != NULL);

	if(!list_size)
	{
		list_size = sizeof(TwitterList);
	}

	g_static_mutex_lock(&mutex_twitterdb);

	/* get user details */
	if(_twitterdb_get_user(handle, user_guid, user, err))
	{
		/* get list guids from database */
		if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_list_guids_from_user, &stmt, err))
		{
			sqlite3_bind_text(stmt, 1, user_guid, -1, NULL);
			success = TRUE;

			while((status = _twitterdb_execute_statement(handle, stmt, TRUE, err)) != SQLITE_DONE)
			{
				if(status == SQLITE_ROW)
				{
					/* get list details */
					g_debug("Found list: %s", sqlite3_column_text(stmt, 0));
					list = g_slice_alloc(list_size);
			
					if(_twitterdb_get_list(handle, (const gchar *)sqlite3_column_text(stmt, 0), list, err))
					{
						/* append details to list */
						lists = g_list_append(lists, list);
					}
					else
					{
						g_slice_free1(list_size, list);
						break;
					}
				}
				else
				{
					success = FALSE;
					break;
				}
			}

			sqlite3_finalize(stmt);
		}
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	/* free list on failure */
	if(!success && lists)
	{
		twitterdb_free_get_lists_result(lists);
		lists = NULL;
	}

	return lists;
}

gboolean
twitterdb_get_list(TwitterDbHandle *handle, const gchar * restrict owner, const gchar * restrict listname, TwitterList *list, GError **err)
{
	gchar guid[16];
	gboolean result = FALSE;


	if(twitterdb_map_listname(handle, owner, listname, guid, 16, err))
	{
		g_static_mutex_lock(&mutex_twitterdb);
		result = _twitterdb_get_list(handle, guid, list, err);
		g_static_mutex_unlock(&mutex_twitterdb);

	}

	return result;
}

void
twitterdb_free_get_lists_result(GList *list)
{
	GList *iter;
	static gint list_size = 0;

	if(!list_size)
	{
		list_size = sizeof(TwitterList);
	}

	if((iter = list))
	{
		while(iter)
		{
			g_slice_free1(list_size, iter->data);
			iter = iter->next;
		}

		g_list_free(list);
	}
}

gint
twitterdb_get_list_membership(TwitterDbHandle *handle, const gchar *username, gchar ***accounts, gchar ***lists, GError **err)
{
	sqlite3_stmt *stmt;
	gint dbstatus;
	gboolean count = -1;

	g_assert(handle != NULL);
	g_assert(username != NULL);

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_list_membership, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, username, -1, NULL);
		count = 0;

		while((dbstatus = _twitterdb_execute_statement(handle, stmt, TRUE, err)) != SQLITE_DONE)
		{
			if(dbstatus == SQLITE_ROW)
			{
				++count;

				*accounts = (gchar **)g_realloc(*accounts, sizeof(gchar **) * count);
				*lists = (gchar **)g_realloc(*lists, sizeof(gchar **) * count);

				(*accounts)[count - 1] = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));
				(*lists)[count - 1] = g_strdup((const gchar *)sqlite3_column_text(stmt, 1));
			}
			else
			{
				for(gint i = 0; i < count; ++count)
				{
					g_free((*accounts)[i]);
					g_free((*lists)[i]);
				}

				g_free(*accounts);
				g_free(*lists);
				count = -1;

				break;
			}
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return count;
}

gboolean
twitterdb_user_is_list_member(TwitterDbHandle *handle, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err)
{
	sqlite3_stmt *stmt;
	gint dbstatus;
	gint count = 0;

	g_assert(handle != NULL);
	g_assert(username != NULL);
	g_assert(listname != NULL);

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_user_is_list_member, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, username, -1, NULL);
		sqlite3_bind_text(stmt, 2, listname, -1, NULL);
		sqlite3_bind_text(stmt, 3, owner, -1, NULL);
		count = 0;

		if((dbstatus = _twitterdb_execute_statement(handle, stmt, TRUE, err)) == SQLITE_ROW)
		{
			count = sqlite3_column_int(stmt, 0);
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return count ? TRUE : FALSE;
}

gchar **
twitterdb_get_lists_following_user(TwitterDbHandle *handle, const gchar *user_guid, GError **err)
{
	sqlite3_stmt *stmt;
	gint status;
	gboolean success = FALSE;
	gint count = 0;
	gchar **list_guids = NULL;

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_lists_following_user, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, user_guid, -1, NULL);
		success = TRUE;

		while((status = _twitterdb_execute_statement(handle, stmt, TRUE, err)) != SQLITE_DONE)
		{
			if(status == SQLITE_ROW)
			{
				g_debug("Found list: %s", sqlite3_column_text(stmt, 0));
				++count;
				list_guids = (gchar **)g_realloc(list_guids, sizeof(gchar **) * count);
				list_guids[count - 1] = g_strdup((gchar *)sqlite3_column_text(stmt, 0));
			}
			else
			{
				success = FALSE;
				break;
			}
		}

		sqlite3_finalize(stmt);

		list_guids = (gchar **)g_realloc(list_guids, sizeof(gchar **) * (count + 1));
		list_guids[count] = NULL;
	}

	if(!success && list_guids)
	{
		g_strfreev(list_guids);
		list_guids = NULL;
	}

	return list_guids;
}

gboolean
twitterdb_append_status_to_public_timeline(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_add_status_to_timeline(handle, user_guid, status_guid, TWITTERDB_TIMELINE_TYPE_PUBLIC, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_append_status_to_replies(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_add_status_to_timeline(handle, user_guid, status_guid, TWITTERDB_TIMELINE_TYPE_REPLIES, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_append_status_to_user_timeline(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_add_status_to_timeline(handle, user_guid, status_guid, TWITTERDB_TIMELINE_TYPE_USER_TIMELINE, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_append_status_to_list(TwitterDbHandle *handle, const gchar * restrict list_guid, const gchar * restrict status_guid, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_replace_into_list_timeline, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, list_guid, -1, NULL);
		sqlite3_bind_text(stmt, 2, status_guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}
	}

	sqlite3_finalize(stmt);

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_status_exists(TwitterDbHandle *handle, const gchar *guid, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_status_exists, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			 if(sqlite3_column_int(stmt, 0) > 0)
			 {
				 result = TRUE;
			 }
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_get_status(TwitterDbHandle *handle, const gchar *guid, TwitterStatus *status, TwitterUser *user, GError **err)
{
	sqlite3_stmt *stmt;
	gchar user_guid[32];
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_status, &stmt, err))
	{
		memset(status, 0, sizeof(TwitterStatus));
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			g_strlcpy(status->id, guid, 32);
			TWITTERDB_COPY_TEXT_COLUMN(user_guid, 1, 32);
			status->timestamp = sqlite3_column_int(stmt, 1);
			TWITTERDB_COPY_TEXT_COLUMN(status->text, 0, 280);
			TWITTERDB_COPY_TEXT_COLUMN(status->prev_status, 3, 32);
			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	if(result)
	{
		result = _twitterdb_get_user(handle, user_guid, user, err);
	}

	return result;
}

gboolean
twitterdb_append_user_to_list(TwitterDbHandle *handle, const gchar * restrict list_guid, const gchar * restrict user_guid, GError **err)
{
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);
	result = _twitterdb_update_list_member(handle, list_guid, user_guid, FALSE, err);
	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_remove_user_from_list(TwitterDbHandle *handle, const gchar * restrict list_guid, const gchar * restrict user_guid, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result;

	g_static_mutex_lock(&mutex_twitterdb);

	/* remove list member */
	if((result = _twitterdb_update_list_member(handle, list_guid, user_guid, TRUE, err)))
	{
		/* remove obsolete statuses */
		if(_twitterdb_prepare_statement(handle, twitterdb_queries_remove_obsolete_statuses_from_list, &stmt, err))
		{
			sqlite3_bind_text(stmt, 1, list_guid, -1, NULL);
			sqlite3_bind_text(stmt, 2, list_guid, -1, NULL);
			sqlite3_bind_text(stmt, 3, list_guid, -1, NULL);

			if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
			{
				result = TRUE;
			}
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_remove_users_from_list(TwitterDbHandle *handle, const gchar *list_guid, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_delete_list_members, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, list_guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}
	}

	sqlite3_finalize(stmt);

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_list_exists(TwitterDbHandle *handle, const gchar * restrict owner, const gchar * restrict listname, GError **err)
{
	gchar guid[32];

	return twitterdb_map_listname(handle, owner, listname, guid, 32, err);
}

gboolean
twitterdb_save_direct_message(TwitterDbHandle *handle, const gchar * restrict guid, const gchar * restrict text, gint64 timestamp,
                              const gchar * restrict sender_guid, const gchar * restrict receiver_guid, gint *count, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean exists = FALSE;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	/* check if direct message does already exist */
	if(_twitterdb_prepare_statement(handle, twitterdb_queries_direct_message_exists, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			if(sqlite3_column_int(stmt, 0) == 0)
			{
				++(*count);
			}
			else
			{
				exists = TRUE;
			}

			result = TRUE;
		}

		sqlite3_finalize(stmt);
	}

	/* insert direct message */
	if(!exists && result)
	{
		result = FALSE;

		if(_twitterdb_prepare_statement(handle, twitterdb_queries_insert_direct_message, &stmt, err))
		{
			sqlite3_bind_text(stmt, 1, guid, -1, NULL);
			sqlite3_bind_text(stmt, 2, text, -1, NULL);
			sqlite3_bind_int(stmt, 3, (sqlite3_int64)timestamp);
			sqlite3_bind_text(stmt, 4, sender_guid, -1, NULL);
			sqlite3_bind_text(stmt, 5, receiver_guid, -1, NULL);
			if(!(result = (_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE) ? TRUE : FALSE))
			{
				_twitterdb_set_error(err, handle);
			}
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_foreach_status_in_public_timeline(TwitterDbHandle *handle, const gchar *username, TwitterProcessStatusFunc func, gboolean ignore_old, gpointer user_data, GCancellable *cancellable, GError **err)
{
	gboolean result;

	result = _twitterdb_foreach_status_in_timeline_locked(handle, username, TWITTERDB_TIMELINE_TYPE_PUBLIC, func, ignore_old, user_data, cancellable, err);

	return result;
}

gboolean
twitterdb_foreach_status_in_replies(TwitterDbHandle *handle, const gchar *username, TwitterProcessStatusFunc func, gboolean ignore_old, gpointer user_data, GCancellable *cancellable, GError **err)
{
	gboolean result;

	result = _twitterdb_foreach_status_in_timeline_locked(handle, username, TWITTERDB_TIMELINE_TYPE_REPLIES, func, ignore_old, user_data, cancellable, err);

	return result;

}

gboolean
twitterdb_foreach_status_in_usertimeline(TwitterDbHandle *handle, const gchar *username, TwitterProcessStatusFunc func, gboolean ignore_old, gpointer user_data, GCancellable *cancellable, GError **err)
{
	gboolean result;

	result = _twitterdb_foreach_status_in_timeline_locked(handle, username, TWITTERDB_TIMELINE_TYPE_USER_TIMELINE, func, ignore_old, user_data, cancellable, err);

	return result;
}

gboolean
twitterdb_foreach_status_in_list(TwitterDbHandle *handle, const gchar * restrict username, const gchar * restrict list, TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	sqlite3_stmt *stmt;
	GList *tweets = NULL;
	GList *users = NULL;
	gboolean result = FALSE;

	g_assert(handle != NULL);
	g_assert(username != NULL);
	g_assert(func != NULL);

	/* get tweets */
	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_tweets_from_list, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, username, -1, NULL);
		sqlite3_bind_text(stmt, 2, list, -1, NULL);
		result = TRUE;

		result = _twitterdb_fetch_tweets(handle, stmt, &tweets, &users, cancellable, err);
		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	/* iterate result */
	if(result)
	{
		_twitterdb_foreach_status(tweets, users, func, user_data, cancellable);
	}

	_twitterdb_free_list(tweets, sizeof(TwitterStatus));
	_twitterdb_free_list(users, sizeof(TwitterUser));

	return result;
}

gboolean
twitterdb_foreach_list_member(TwitterDbHandle *handle, const gchar * restrict username, const gchar * restrict list, TwitterProcessListMemberFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	sqlite3_stmt *stmt;
	gint dbstatus;
	TwitterUser user;
	gboolean result = FALSE;

	g_assert(handle != NULL);
	g_assert(username != NULL);
	g_assert(func != NULL);

	g_static_mutex_lock(&mutex_twitterdb);

	memset(&user, 0, sizeof(user));

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_list_members, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, username, -1, NULL);
		sqlite3_bind_text(stmt, 2, list, -1, NULL);
		result = TRUE;

		while((dbstatus = _twitterdb_execute_statement(handle, stmt, TRUE, err)) != SQLITE_DONE)
		{
			if(cancellable && g_cancellable_is_cancelled(cancellable))
			{
				g_debug("%s: cancelled", __func__);
				result = FALSE;
				break;
			}

			if(dbstatus == SQLITE_ROW)
			{
				/* copy data */
				TWITTERDB_COPY_TEXT_COLUMN(user.id, 0, 32);
				TWITTERDB_COPY_TEXT_COLUMN(user.screen_name, 1, 64);
				TWITTERDB_COPY_TEXT_COLUMN(user.name, 2, 64);
				TWITTERDB_COPY_TEXT_COLUMN(user.image, 3, 256);
				TWITTERDB_COPY_TEXT_COLUMN(user.location, 4, 64);
				TWITTERDB_COPY_TEXT_COLUMN(user.url, 5, 256);
				TWITTERDB_COPY_TEXT_COLUMN(user.description, 6, 280);

				/* invoke callback */
				func(user, user_data);
			}
			else
			{
				result = FALSE;
				break;
			}
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gint
twitterdb_count_tweets_from_list(TwitterDbHandle *handle, const gchar *list_guid, GError **err)
{
	sqlite3_stmt *stmt;
	gint count = 0;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_count_tweets_from_list, &stmt, err))
	{
		sqlite3_bind_text(stmt, 1, list_guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_ROW)
		{
			count = sqlite3_column_int(stmt, 0);
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	return count;
}

gint
twitterdb_get_last_sync(TwitterDbHandle *handle, TwitterDbSyncSource source, const gchar *user_guid, gint default_val)
{
	sqlite3_stmt *stmt;
	gint result = default_val;
	GError *err = NULL;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_get_sync_seconds, &stmt, &err))
	{
		sqlite3_bind_int(stmt, 1, source);
		sqlite3_bind_text(stmt, 2, user_guid, -1, NULL);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, &err) == SQLITE_ROW)
		{
			result = sqlite3_column_int(stmt, 0);
		}

		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	return result;
}

void
twitterdb_set_last_sync(TwitterDbHandle *handle, TwitterDbSyncSource source, const gchar *user_guid, gint seconds)
{
	sqlite3_stmt *stmt;
	GError *err = NULL;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_replace_sync_seconds, &stmt, &err))
	{
		sqlite3_bind_int(stmt, 1, source);
		sqlite3_bind_text(stmt, 2, user_guid, -1, NULL);
		sqlite3_bind_int(stmt, 3, seconds);
		_twitterdb_execute_statement(handle, stmt, TRUE, &err);
		sqlite3_finalize(stmt);
	}

	g_static_mutex_unlock(&mutex_twitterdb);

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}
}

gboolean
twitterdb_remove_last_sync_source(TwitterDbHandle *handle, TwitterDbSyncSource source, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_static_mutex_lock(&mutex_twitterdb);

	if(_twitterdb_prepare_statement(handle, twitterdb_queries_remove_sync_seconds, &stmt, err))
	{
		sqlite3_bind_int(stmt, 1, source);

		if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
		{
			result = TRUE;
		}
	}

	sqlite3_finalize(stmt);

	g_static_mutex_unlock(&mutex_twitterdb);

	return result;
}

gboolean
twitterdb_upgrade_0_1_to_0_2(TwitterDbHandle *handle, GError **err)
{
	sqlite3_stmt *stmt;
	gboolean result = FALSE;

	g_debug("Altering status table: \"%s\"", twitterdb_queries_add_prev_status_column);
	if(_twitterdb_execute_non_query(handle, twitterdb_queries_add_prev_status_column, err))
	{
		g_debug("Updating version to 0.2");
		if(_twitterdb_prepare_statement(handle, twitterdb_queries_replace_version, &stmt, err))
		{
			sqlite3_bind_int(stmt, 1, 0);
			sqlite3_bind_int(stmt, 2, 2);
	
			if(_twitterdb_execute_statement(handle, stmt, TRUE, err) == SQLITE_DONE)
			{
				result = TRUE;
			}

			sqlite3_finalize(stmt);
		}
	}

	return result;
}

/**
 * @}
 * @}
 */


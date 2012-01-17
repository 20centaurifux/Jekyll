/***************************************************************************
    begin........: December 2010
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
 * \file twittersync.c
 * \brief Twitter synchronization functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 11. January 2012
 */

#include <string.h>

#include "twittersync.h"
#include "twitterxmlparser.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/*
 *	helpers:
 */
/*! Frees a string and set it to NULL. */
#define _twittersync_free_buffer(b) if(b) { g_free(b); b = NULL; }

static gboolean
_twittersync_save_status(TwitterDbHandle *handle, TwitterStatus status, TwitterUser user, gint *status_count)
{
	gboolean result = FALSE;
	gint64 timestamp;
	GError *err = NULL;

	/* save user */
	g_debug("Registering user \"%s\" (%s)", user.screen_name, user.id);
	if(twitterdb_save_user(handle, user.id, user.screen_name, user.name, user.image, user.location, user.url, user.description, &err))
	{
		/* save status */
		g_debug("Registering status (\"%s\")", status.id);
		timestamp = twitter_timestamp_to_unix_timestamp(status.created_at);
		result = twitterdb_save_status(handle, status.id, status.prev_status, user.id, status.text, timestamp, status_count, &err);
	}

	/* display & free error message */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	return result;
}

/*
 *	get user guid from database or Twitter service:
 */
static void
_twittersync_copy_user_from_details(TwitterUser user, gpointer destination)
{
	memcpy(destination, &user, sizeof(TwitterUser));
}

static gboolean
_twittersync_get_user_guid(TwitterDbHandle *handle, TwitterWebClient *client, const gchar *username, gchar guid[32])
{
	gchar *buffer = NULL;
	gint length;
	TwitterUser user;
	GError *err = NULL;
	gboolean result;

	memset(&user, 0, sizeof(TwitterUser));

	/* try to find username in database */
	g_debug("Mapping username \"%s\"", username);
	if((result = twitterdb_map_username(handle, username, guid, 32, &err)))
	{
		g_debug("Mapped username: \"%s\" => \"%s\"", username, guid);
	}
	else
	{
		/* get user details from Twitter */
		g_debug("Couldn't map username, sending Twitter request");
		if(twitter_web_client_get_user_details(client, username, &buffer, &length))
		{
			/* parse user information */
			twitter_xml_parse_user_details(buffer, length, _twittersync_copy_user_from_details, &user);
			if(user.id[0])
			{
				/* write user details to database  */
				g_debug("Got user details from Twitter service: \"%s\" (%s)", user.name, user.id);
				if(twitterdb_save_user(handle, user.id, user.screen_name, user.name, user.image, user.location, user.url, user.description, &err))
				{
					/* copy guid */
					g_strlcpy(guid, user.id, 32);
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
	}

	/* free memory */
	g_free(buffer);

	return result;
}

/*
 *	synchronize timelines:
 */

/*! Timeline types. */
typedef enum
{
	TWITTERSYNC_TIMELINE_HOME,
	TWITTERSYNC_TIMELINE_REPLIES,
	TWITTERSYNC_TIMELINE_USER_TIMELINE
} _TwitterSyncTimlineType;

/*!
 * \struct _TwitterSyncTimelinesData
 * \brief This structure holds data for saving timeline data.
 */
typedef struct
{
	/*! Database handle. */
	TwitterDbHandle *db;
	/*! Pointer to status counter. */
	gint *status_count;
	/*! Name of the user. */
	const gchar *username;
	/*! Guid of the user. */
	gchar user_guid[32];
	/*! Type of the timeline. */
	_TwitterSyncTimlineType type;
	/*! AGCancellable. */
	GCancellable *cancellable;
} _TwitterSyncTimelinesData;

static void
_twittersync_update_timeline(TwitterStatus status, TwitterUser user, gpointer user_data)
{
	_TwitterSyncTimelinesData *arg = (_TwitterSyncTimelinesData *)user_data;
	gboolean (* func)(TwitterDbHandle *handle, const gchar *user_guid, const gchar *status_guid, GError **err) = NULL;
	GError *err = NULL;

	g_debug("%s: status \"%s\" from \"%s\"", __func__, status.id, user.name);
	
	/* save status */
	if(_twittersync_save_status(arg->db, status, user, arg->status_count))
	{
		/* set callback function */
		g_debug("Appending status \"%s\" to timeline", status.id);
		switch(arg->type)
		{
			case TWITTERSYNC_TIMELINE_HOME:
				func = twitterdb_append_status_to_public_timeline;
				break;
			
			case TWITTERSYNC_TIMELINE_REPLIES:
				func = twitterdb_append_status_to_replies;
				break;
			
			case TWITTERSYNC_TIMELINE_USER_TIMELINE:
				func = twitterdb_append_status_to_user_timeline;
				break;
	
			default:
				g_warning("Invalid timeline identifier: %d", arg->type);
		}

		/* invoke callback */
		if(!func(arg->db, arg->user_guid, status.id, &err))
		{
			g_warning("Couldn't append status \"%s\" to timeline(%d)", status.id, arg->type);
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
		}
	}
}

static void
_twittersync_process_timeline(const gchar *buffer, gint length, _TwitterSyncTimlineType type, _TwitterSyncTimelinesData *arg)
{
	arg->type = type;
	twitter_xml_parse_timeline(buffer, length, _twittersync_update_timeline, arg, arg->cancellable);
}

gboolean
twittersync_update_timelines(TwitterDbHandle *handle, TwitterWebClient *client, gint *status_count, GCancellable *cancellable, GError **err)
{
	_TwitterSyncTimelinesData *arg = (_TwitterSyncTimelinesData *)g_alloca(sizeof(_TwitterSyncTimelinesData));
	gchar *buffer = NULL;
	gint length;
	gboolean result = FALSE;

	g_assert(handle != NULL);
	g_assert(client != NULL);

	arg->db = handle;
	arg->username = NULL;
	memset(arg->user_guid, 0, 32);
	arg->status_count = status_count;
	arg->cancellable = cancellable;

	/* get username and guid */
	if((arg->username = twitter_web_client_get_username(client)))
	{
		_twittersync_get_user_guid(handle, client, arg->username, arg->user_guid);
	}
	else
	{
		g_warning("twitter_web_client_get_username(client) == NULL");
	}

	if(arg->user_guid[0])
	{
		result = TRUE;

		/* home timeline */
		if(!cancellable || !g_cancellable_is_cancelled(cancellable))
		{
			g_debug("Fetching home timeline: \"%s\"", arg->username);
			if((result = twitter_web_client_get_home_timeline(client, &buffer, &length)))
			{
				_twittersync_process_timeline(buffer, length, TWITTERSYNC_TIMELINE_HOME, arg);
			}
	
			_twittersync_free_buffer(buffer);
		}

		/* replies */
		if(result && (!cancellable || !g_cancellable_is_cancelled(cancellable)))
		{
			g_debug("Fetching replies: \"%s\"", arg->username);
			if((result = twitter_web_client_get_mentions(client, &buffer, &length)))
			{
				_twittersync_process_timeline(buffer, length, TWITTERSYNC_TIMELINE_REPLIES, arg);
			}

			_twittersync_free_buffer(buffer);
		}

		/* user timeline */
		if(result && (!cancellable || !g_cancellable_is_cancelled(cancellable)))
		{
			g_debug("Fetching user timeline: \"%s\"", arg->username);
			if(twitter_web_client_get_user_timeline(client, NULL, &buffer, &length))
			{
				_twittersync_process_timeline(buffer, length, TWITTERSYNC_TIMELINE_USER_TIMELINE, arg);
			}

			_twittersync_free_buffer(buffer);
		}

		if(result && (!cancellable || !g_cancellable_is_cancelled(cancellable)))
		{
			result = TRUE;
		}
	}

	return result;
}

/*
 *	synchronize lists:
 */

/*!
 * \struct _TwitterSyncListsData
 * \brief This structure holds data for saving list information.
 */
typedef struct
{
	/*! Database handle. */
	TwitterDbHandle *db;
	/*! Pointer to status counter. */
	gint *status_count;
	/*! Array containing found list guids. */
	GList *found_lists;
	/*! A TwitterWebClient instance. */
	TwitterWebClient *client;
	/*! Owner of current list. */
	const gchar *owner;
	/*! Pointer to current list data */
	TwitterList *list;
	/*! TRUE if list members should be synchronized. */
	gboolean sync_members;
	/*! AGCancellable. */
	GCancellable *cancellable;
} _TwitterSyncListsData;

static void
_twittersync_save_tweet_from_list(TwitterStatus status, TwitterUser user, gpointer user_data)
{
	_TwitterSyncListsData *arg = (_TwitterSyncListsData *)user_data;
	GError *err = NULL;

	/* save status */
	if(_twittersync_save_status(arg->db, status, user, arg->status_count))
	{
		/* append status to list */
		g_debug("Appending status %s to list \"%s\" (%s)", status.id, arg->list->name, arg->list->id);
		if(!twitterdb_append_status_to_list(arg->db, arg->list->id, status.id, &err))
		{
			g_warning("Couldn't append status %s to list \"%s\" (%s)", status.id, arg->list->name, arg->list->id);
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
		}
	}
}

static gboolean
_twittersync_save_tweets_from_list(_TwitterSyncListsData *arg)
{
	gchar *buffer = NULL;
	gint length;
	GError *err = NULL;
	gboolean result = FALSE;

	/* get tweets from list */
	g_debug("Getting tweets from list: \"%s\"", arg->list->id);
	if(twitter_web_client_get_timeline_from_list(arg->client, arg->owner, arg->list->id, &buffer, &length))
	{
		/* save tweets */
		twitter_xml_parse_timeline(buffer, length, _twittersync_save_tweet_from_list, arg, arg->cancellable);

		if(!arg->cancellable || !g_cancellable_is_cancelled(arg->cancellable))
		{
			result = TRUE;
		}
	}
	else
	{
		g_warning("Couldn't get tweets from list: \"%s\"", arg->list->id);
		if((err = (GError *)twitter_web_client_get_last_error(arg->client)))
		{
			g_warning("%s", err->message);
		}
	}

	/* free memory */
	g_free(buffer);

	return result;
}

static void
_twittersync_add_list_member(TwitterUser user, gpointer user_data)
{
	_TwitterSyncListsData *arg = (_TwitterSyncListsData *)user_data;
	GError *err = NULL;

	/* save user */
	g_debug("Registering user \"%s\" (%s)", user.screen_name, user.id);
	if(twitterdb_save_user(arg->db, user.id, user.screen_name, user.name, user.image, user.location, user.url, user.description, &err))
	{
		g_debug("Adding member \"%s\" to list \"@%s/%s\" (%s)", user.name, arg->owner, arg->list->name, arg->list->id);
		if(!twitterdb_append_user_to_list(arg->db, arg->list->id, user.id, &err))
		{
			g_debug("Couldn't append member \"%s\" to list \"@%s/%s\" (%s)", user.name, arg->owner, arg->list->name, arg->list->id);
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
		}
	}
	else
	{
		g_debug("Couldn't append member \"%s\" to list \"@%s/%s\"", user.name, arg->owner, arg->list->name);
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}
}

static gboolean
_twittersync_update_list_members(_TwitterSyncListsData *arg)
{
	gchar *buffer = NULL;
	gint length;
	gchar next_cursor[64] = "-1";
	GError *err = NULL;
	gboolean result = FALSE;

	/* remove followers */
	g_debug("Removing followers from \"%s\"", arg->list->name);
	if(twitterdb_remove_users_from_list(arg->db, arg->list->id, &err))
	{
		result = TRUE;

		while(g_strcmp0(next_cursor, "0") && (!arg->cancellable || !g_cancellable_is_cancelled(arg->cancellable)) && result)
		{
			/* get list members from Twitter service */
			g_debug("Getting members from list, cursor=\"%s\"", next_cursor);

			if((twitter_web_client_get_users_from_list(arg->client, arg->owner, arg->list->id, next_cursor, &buffer, &length)))
			{
				/* add list members */
				twitter_xml_parse_list_members(buffer, length, _twittersync_add_list_member, next_cursor, 64, arg);
			}
			else
			{
				g_warning("Couldn't get list members from: \"%s\"", arg->list->name);
				if((err = (GError *)twitter_web_client_get_last_error(arg->client)))
				{
					g_warning("%s", err->message);
					err = NULL;
					result = FALSE;
				}
			}

			/* free memory */
			_twittersync_free_buffer(buffer);

			if(err)
			{
				g_error_free(err);
				err = NULL;
			}
		}
	}
	else
	{
		g_warning("Couldn't remove users from list: \"%s\"", arg->list->name);

		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	return result;
}

static gboolean
_twittersync_process_list_from_database(TwitterList list, TwitterUser user, _TwitterSyncListsData *arg)
{
	GError *err = NULL;
	gboolean exists;
	gboolean remove = FALSE;
	gboolean result = FALSE;

	g_debug("Processing list: \"@%s/%s\" (%s)", user.name, list.name, list.id);

	/* set list data */
	arg->list = &list;

	/* check if list does still exist online */
	if((exists = (g_list_find_custom(arg->found_lists, list.id, (GCompareFunc)&g_strcmp0) ? TRUE : FALSE)))
	{
		g_debug("Updating list: \"%s\" (%s)", list.name, list.id);

		if(arg->sync_members)
		{
			/* update list members */
			result = _twittersync_update_list_members(arg);
		}

		/* save tweets */
		if(result)
		{
			result = _twittersync_save_tweets_from_list(arg);
		}
	}

	/* remove list if timeline is empty (I implemented this workaround because Twitter sometimes
	 * sends deleted lists)
	 */
	if(!twitterdb_count_tweets_from_list(arg->db, list.id, &err))
	{
		remove = TRUE;
	}

	/* display failure message & free memory */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
		err = NULL;
	}

	/* remove list from database */
	if(!exists || remove)
	{
		g_debug("Removing list: \"%s\" (%s)", list.name, list.id);
		if(!(result = twitterdb_remove_list(arg->db, list.id, &err)))
		{
			g_warning("Couldn't remove list: \"%s\" (%s)", list.name, list.id);
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
		}
	}

	return result;
}

static void
_twittersync_save_list(TwitterList list, TwitterUser user, gpointer user_data)
{
	_TwitterSyncListsData *arg = (_TwitterSyncListsData *)user_data;
	GError *err = NULL;

	/* save user */
	g_debug("Registering user \"%s\" (%s)", user.screen_name, user.id);
	if(twitterdb_save_user(arg->db, user.id, user.screen_name, user.name, user.image, user.location, user.url, user.description, &err))
	{
		/* save list */
		g_debug("Registering list \"%s\" (%s)", list.name, list.id);
		twitterdb_save_list(arg->db, list.id, user.id, list.name, list.name, list.uri,
                                    list.description, list.protected, list.subscriber_count,
                                    list.member_count, &err);
		arg->found_lists = g_list_append(arg->found_lists, g_strdup(list.id));
	}

	/* display & free error message */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}
}

gboolean
twittersync_update_lists(TwitterDbHandle *handle, TwitterWebClient *client, gint *status_count, gboolean sync_members, GCancellable *cancellable, GError **err)
{
	_TwitterSyncListsData *arg = (_TwitterSyncListsData *)g_alloca(sizeof(_TwitterSyncListsData));
	gchar *buffer = NULL;
	gint length;
	gchar user_guid[32] = { 0 };
	const GError *last_error;
	GList *lists;
	GList *iter;
	static gint list_size = 0;
	TwitterUser user;
	gboolean result = FALSE;

	g_assert(handle != NULL);
	g_assert(client != NULL);

	arg->db = handle;
	arg->found_lists = NULL;
	arg->client = client;
	arg->owner = NULL;
	arg->list = NULL;
	arg->status_count = status_count;
	arg->cancellable = cancellable;

	if(!list_size)
	{
		list_size = sizeof(TwitterList);
	}

	/* set owner */
	if((arg->owner = twitter_web_client_get_username(client)))
	{
		_twittersync_get_user_guid(handle, client, arg->owner, user_guid);
	}
	else
	{
		g_warning("twitter_web_client_get_username(client) == NULL");
	}

	if(user_guid[0])
	{
		/* get lists from twitter service */
		if(twitter_web_client_get_lists(client, NULL, &buffer, &length))
		{
			/* process data */
			if((result = buffer ? TRUE : FALSE))
			{
				/* save lists in database */
				twitter_xml_parse_lists(buffer, length, _twittersync_save_list, arg);

				/* remove non-existing lists from database, update list members & get tweets */
				if((lists = iter = twitterdb_get_lists(handle, user_guid, &user, NULL)))
				{
					while(iter)
					{
						if(cancellable && g_cancellable_is_cancelled(cancellable))
						{
							result = FALSE;
							break;
						}
					
						 _twittersync_process_list_from_database(*(TwitterList *)iter->data, user, arg);
						iter = iter->next;
					}

					twitterdb_free_get_lists_result(lists);
				}
			}
		}
		else
		{
			g_warning("Couldn't get lists from Twitter service");
			if((last_error = twitter_web_client_get_last_error(client)))
			{
				g_warning("%s", last_error->message);
			}
		}

		/* free memory */
		g_free(buffer);

		if(arg->found_lists)
		{
			g_list_foreach(arg->found_lists, (GFunc)g_free, NULL);
			g_list_free(arg->found_lists);
		}
	}

	return result;
}

/*
 *	synchronize direct messages
 */

/*!
 * \struct _TwitterSyncDirectMessageData
 * \brief This structure holds data for saving direct messages.
 */
typedef struct
{
	/*! Database handle. */
	TwitterDbHandle *db;
	/*! Pointer to message counter. */
	gint *message_count;
} _TwitterSyncDirectMessageData;

static void
_twittersync_save_direct_message(TwitterDirectMessage message, TwitterUser sender, TwitterUser receiver, gpointer user_data)
{
	_TwitterSyncDirectMessageData *arg = (_TwitterSyncDirectMessageData *)user_data;
	gint64 timestamp;
	GError *err = NULL;

	/* save sender */
	g_debug("Registering sender \"%s\" (%s)", sender.screen_name, sender.id);
	if(twitterdb_save_user(arg->db, sender.id, sender.screen_name, sender.name, sender.image, sender.location, sender.url, sender.description, &err))
	{
		/* save receiver */
		g_debug("Registering receiver \"%s\" (%s)", receiver.screen_name, receiver.id);
		if(twitterdb_save_user(arg->db, receiver.id, receiver.screen_name, receiver.name, receiver.image, receiver.location, receiver.url, receiver.description, &err))
		{
			/* save direct message */
			g_debug("Saving direct message \"%s\" from \"%s\" to \"%s\"", message.id, sender.name, receiver.name);
			timestamp = twitter_timestamp_to_unix_timestamp(message.created_at);
			twitterdb_save_direct_message(arg->db, message.id, message.text, timestamp, sender.id, receiver.id, arg->message_count, &err);
		}
	}

	/* show & free error message */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}
}

gboolean
twittersync_update_direct_messages(TwitterDbHandle *handle, TwitterWebClient *client, gint *message_count, GError **err)
{
	_TwitterSyncDirectMessageData *arg = (_TwitterSyncDirectMessageData*)g_alloca(sizeof(_TwitterSyncDirectMessageData));
	gchar *buffer = NULL;
	gint length;
	gboolean result = FALSE;

	g_assert(handle != NULL);
	g_assert(client != NULL);

	arg->db = handle;
	arg->message_count = message_count;

	/* get direct messages */
	g_debug("Fetching direct messages for user: \"%s\"", twitter_web_client_get_username(client));
	if(twitter_web_client_get_direct_messages(client, &buffer, &length))
	{
		twitter_xml_parse_direct_messages(buffer, length, _twittersync_save_direct_message, arg);
	}

	/* free memory */
	if(buffer)
	{
		g_free(buffer);
	}

	return result;
}

/*
 *	synchronize friends:
 */

/*!
 * \struct _TwitterSyncFriendData
 * \brief This structure holds data for saving friendship information.
 */
typedef struct
{
	/*! Database handle. */
	TwitterDbHandle *db;
	/*! A TwitterWebClient instance. */
	TwitterWebClient *client;
	/*! List containing friend ids. */
	GList *follower_ids;
	/*! Status. */
	gboolean success;
	/*! Guid of the user. */
	gchar user_guid[64];
	/*! Name of the user. */
	gchar username[64];
	/*! TRUE to synchronize friends. */
	gboolean sync_friends;
	/*! AGCancellable. */
	GCancellable *cancellable;
} _TwitterSyncFriendData;

/* create friend id list */
static void
_twittersync_parse_follower_id(const gchar *user_id, _TwitterSyncFriendData *arg)
{
	arg->follower_ids = g_list_append(arg->follower_ids, g_strdup(user_id));
}

static gboolean
_twittersync_get_follower_ids(_TwitterSyncFriendData *arg, GError **err)
{
	gchar *buffer = NULL;
	gint length;
	gchar next_cursor[64] = "-1";
	const GError *client_err;

	g_assert(arg->success == TRUE);

	g_debug("Getting follower ids.");

	/* map username */
	g_debug("Mapping username: \"%s\"", arg->username);
	if(twitterdb_map_username(arg->db, arg->username, arg->user_guid, 64, err))
	{
		g_debug("username mapped successfully: \"%s\" => \"%s\"", arg->username, arg->user_guid);

		/* get friend ids */
		while(g_strcmp0(next_cursor, "0") && (!arg->cancellable || !g_cancellable_is_cancelled(arg->cancellable)) && arg->success)
		{
			g_debug("Fetching %s of user: \"%s\", cursor=\"%s\"", arg->sync_friends ? "friends" : "followers", arg->username, next_cursor);

			if(arg->sync_friends)
			{
				arg->success = twitter_web_client_get_friends(arg->client, arg->username, next_cursor, &buffer, &length);
			}
			else
			{
				arg->success = twitter_web_client_get_followers(arg->client, arg->username, next_cursor, &buffer, &length);
			}

			if(arg->success)
			{
				twitter_xml_parse_ids(buffer, length, (TwitterProcessIdFunc)_twittersync_parse_follower_id, next_cursor, 64, arg->cancellable, arg);
			}
			else
			{
				g_warning("Couldn't get friends of user \"%s\"", arg->user_guid);

				if(!err && (client_err = (GError *)twitter_web_client_get_last_error(arg->client)))
				{
					g_set_error(err, 0, 0, "%s", client_err->message);
					client_err = NULL;
					arg->success = FALSE;
				}
			}

			g_free(buffer);
			buffer = NULL;
		}
	}

	if(arg->cancellable && g_cancellable_is_cancelled(arg->cancellable))
	{
		arg->success = FALSE;
	}

	return arg->success;
}

/* remove deleted friends */
static gboolean
_twittersync_remove_deleted_friends(_TwitterSyncFriendData *arg, GError **err)
{
	gchar *follower_id;
	GList *old_followers;
	GList *iter = NULL;

	g_assert(arg->success == TRUE);

	/* get old friend ids */
	g_debug("Getting old %s of user \"%s\" (\"%s\")", arg->sync_friends ? "friends" : "followers", arg->username, arg->user_guid);

	if(arg->sync_friends)
	{
		old_followers = twitterdb_get_friends(arg->db, arg->user_guid, err);
	}
	else
	{
		old_followers = twitterdb_get_followers(arg->db, arg->user_guid, err);
	}

	if(*err)
	{
		arg->success = FALSE;
	}
	else
	{
		iter = old_followers;
	}

	/* find deleted ids */
	while(iter && arg->success)
	{
		follower_id = (gchar *)iter->data;
		g_debug("Testing %s: \"%s\"", arg->sync_friends ? "friend" : "follower", follower_id);

		if(!g_list_find_custom(arg->follower_ids, follower_id, (GCompareFunc)&g_strcmp0))
		{
			g_debug("Removing friendship: \"%s\" => \"%s\"", arg->user_guid, follower_id);
			arg->success = twitterdb_remove_follower(arg->db, arg->user_guid, follower_id, err);
		}

		if(arg->cancellable && g_cancellable_is_cancelled(arg->cancellable))
		{
			arg->success = FALSE;
			break;
		}

		iter = iter->next;
	}

	/* cleanup */
	if(old_followers)
	{
		g_list_foreach(old_followers, (GFunc)&g_free, NULL);
		g_list_free(old_followers);
	}

	return arg->success;
}

/* add friends */
static void
_twittersync_register_follower(TwitterUser user, _TwitterSyncFriendData *arg)
{
	GError *err = NULL;

	/* save user */
	g_debug("Registering user \"%s\" (%s)", user.screen_name, user.id);
	if((arg->success = twitterdb_save_user(arg->db, user.id, user.screen_name, user.name, user.image, user.location, user.url, user.description, &err)))
	{
		/* update friendship */
		g_debug("Saving friendship (\"%s\" => \"%s\")", arg->username, user.screen_name);

		if(arg->sync_friends)
		{
			arg->success = twitterdb_add_follower(arg->db, arg->user_guid, user.id, &err);
		}
		else
		{
			arg->success = twitterdb_add_follower(arg->db, user.id, arg->user_guid, &err);
		}
	}

	/* display & free error message */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}
}

static void
_twittersync_add_follower(const gchar *user_id, _TwitterSyncFriendData *arg)
{
	gchar *buffer = NULL;
	gint length;
	GError *err = NULL;

	/* check if user has already been registerd */
	if(twitterdb_user_exists(arg->db, user_id, &err))
	{
		/* user exists => update friendship */
		if(arg->sync_friends)
		{
			arg->success = twitterdb_add_follower(arg->db, arg->user_guid, user_id, &err);
		}
		else
		{
			arg->success = twitterdb_add_follower(arg->db, user_id, arg->user_guid, &err);
		}
	}
	else if(!err)
	{
		/* user doesn't exist => register user & update friendship */
		g_debug("Fetching user details (id=\"%s\")", user_id);
		if((arg->success = twitter_web_client_get_user_details_by_id(arg->client, user_id, &buffer, &length)))
		{
			twitter_xml_parse_user_details(buffer, length, (TwitterProcessUserFunc)_twittersync_register_follower, arg);
		}
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
		arg->success = FALSE;
	}

	g_free(buffer);
}

static gboolean
_twittersync_save_friends(_TwitterSyncFriendData *arg, GError **err)
{
	GList *iter = arg->follower_ids;

	g_assert(arg->success);

	g_debug("Adding received friends.");

	while(iter && arg->success)
	{
		if(arg->sync_friends)
		{
			g_debug("Adding friend \"%s\"", (gchar *)iter->data);
		}
		else
		{
			g_debug("Adding follower \"%s\"", (gchar *)iter->data);
		}

		_twittersync_add_follower((gchar *)iter->data, arg);

		if(arg->cancellable && g_cancellable_is_cancelled(arg->cancellable))
		{
			arg->success = FALSE;
			break;
		}

		iter = iter->next;
	}

	return arg->success;
}

static gboolean
_twittersync_update_followers(TwitterDbHandle *handle, TwitterWebClient *client, gboolean sync_friends, GCancellable *cancellable, GError **err)
{
	_TwitterSyncFriendData *arg = (_TwitterSyncFriendData *)g_alloca(sizeof(_TwitterSyncFriendData));

	/* initialize argument */
	arg->db = handle;
	arg->client = client;
	arg->follower_ids = NULL;
	arg->success = TRUE;
	arg->sync_friends = sync_friends;
	g_strlcpy(arg->username, twitter_web_client_get_username(client), 64);
	memset(arg->user_guid, 0, 64);
	arg->cancellable = cancellable;

	/* get friends */
	if(_twittersync_get_follower_ids(arg, err))
	{
		/* removing deleted followers */
		if(_twittersync_remove_deleted_friends(arg, err))
		{
			/* save new friends */
			_twittersync_save_friends(arg, err);
		}
	}

	/* cleanup */
	if(arg->follower_ids)
	{
		g_list_foreach(arg->follower_ids, (GFunc)&g_free, NULL);
		g_list_free(arg->follower_ids);
	}

	return arg->success;
}

/* public */
gboolean
twittersync_update_friends(TwitterDbHandle *handle, TwitterWebClient *client, GCancellable *cancellable, GError **err)
{
	return _twittersync_update_followers(handle, client, TRUE, cancellable, err);
}

gboolean
twittersync_update_followers(TwitterDbHandle *handle, TwitterWebClient *client, GCancellable *cancellable, GError **err)
{
	return _twittersync_update_followers(handle, client, FALSE, cancellable, err);
}

/**
 * @}
 * @}
 */


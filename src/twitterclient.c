/***************************************************************************
    begin........: February 2011
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
 * \file twitterclient.c
 * \brief Access to Twitter webservice and data caching.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 11. January 2012
 */

#include "twitterclient.h"
#include "twitter.h"
#include "twitterdb.h"
#include "twitterxmlparser.h"
#include "twitterjsonparser.h"
#include "application.h"
#include "net/httpclient.h"
#include "net/twitterwebclient.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/*! Define TwitterClient. */
G_DEFINE_TYPE(TwitterClient, twitter_client, G_TYPE_OBJECT);

enum
{
	PROP_0,
	PROP_CACHE,
	PROP_LIFETIME
};

/**
 * \struct _TwitterAccountData
 * \brief Holds account data.
 */
typedef struct
{
	/*! Username of the account.*/
	gchar *username;
	/*! OAuth access key. */
	gchar *access_key;
	/*! OAuth access secret. */
	gchar *access_secret;
} _TwitterAccountData;

/**
 * \struct _TwitterClientPrivate
 * \brief Private _TwitterClient data.
 */
struct _TwitterClientPrivate
{
	/*! Internal cache. */
	Cache *cache;
	/*! Lifetime of stored data. */
	gint lifetime;
	/*! A list containing accounts. */
	GList *accounts;
};

/*!
 * Twitter timeline type.
 */
typedef enum
{
	TWITTER_CLIENT_PUBLIC_TIMELINE,
	TWITTER_CLIENT_REPLIES,
	TWITTER_CLIENT_USER_TIMELINE
} TwitterClientTimelineType;

/*
	 helpers:
 */
static gboolean
_twitter_client_account_exists(GList *accounts, const gchar *username)
{
	_TwitterAccountData *account;

	while(accounts)
	{
		account = (_TwitterAccountData *)accounts->data;
		if(!g_ascii_strcasecmp(username, account->username))
		{
			return TRUE;
		}

		accounts = accounts->next;
	}

	return FALSE;
}

static const _TwitterAccountData *
_twitter_client_find_preferred_account(GList *accounts, const gchar *username, GCancellable *cancellable)
{
	HttpClient *client;
	GString *path;
	gchar *xml;
	gint length;
	TwitterFriendship friendship;
	_TwitterAccountData *account;
	GError *err = NULL;
	_TwitterAccountData *result = NULL;

	if(!username)
	{
		return NULL;
	}

	path = g_string_sized_new(128);
	client = http_client_new("hostname", TWITTER_API_HOSTNAME, "port", HTTP_DEFAULT_PORT, NULL);
	g_assert(client != NULL);

	while(accounts && ! g_cancellable_is_cancelled(cancellable))
	{
		account = (_TwitterAccountData *)accounts->data;
		g_debug("Testing account: \"%s\"", account->username);

		g_debug("Receiving friendship information (%s => %s)", username, account->username);
		g_string_printf(path, "/1/friendships/show.xml?source_screen_name=");
		g_string_append_uri_escaped(path, account->username, NULL, TRUE);
		g_string_append_printf(path, "&target_screen_name=");
		g_string_append_uri_escaped(path, username, NULL, TRUE);
		g_debug("Friendship url: \"%s\"", path->str);

		if(http_client_get(client, path->str, &err) == HTTP_OK)
		{
			http_client_read_content(client, &xml, &length);
			memset(&friendship, 0, sizeof(friendship));
			if(twitter_xml_parse_friendship(xml, length, &friendship))
			{
				if(friendship.source_followed_by)
				{
					g_debug("Found account: \"%s\"", account->username);
					result = account;
					break;
				}
			}

			g_free(xml);
		}
		else
		{
			g_warning("Couldn't lookup friendship information: \"%s\"", path->str);
		}

		accounts = accounts->next;

		/* cleanup */
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
			err = NULL;
		}
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_string_free(path, TRUE);
	g_object_unref(client);

	return result;
}

static TwitterWebClient *
_twitter_client_create_web_client(GList *accounts, const gchar *username, GError **err)
{
	const _TwitterAccountData *account;
	TwitterWebClient *client = NULL;
	GList *iter = accounts;

	g_return_val_if_fail(accounts != NULL, NULL);

	/* find given account */
	g_debug("Searching for account \"%s\"", username);

	while(iter)
	{
		if(!g_strcmp0(username, ((_TwitterAccountData *)iter->data)->username))
		{
			account = (_TwitterAccountData *)iter->data;

			client = twitter_web_client_new();
			twitter_web_client_set_format(client, "xml");
			twitter_web_client_set_username(client, account->username);
			twitter_web_client_set_oauth_authorization(client, OAUTH_CONSUMER_KEY, OAUTH_CONSUMER_SECRET, account->access_key, account->access_secret);

			return client;
		}

		iter = iter->next;
	}

	return NULL;
}

static TwitterWebClient *
_twitter_client_create_web_client_for_friend(GList *accounts, const gchar *friend, GCancellable *cancellable, GError **err)
{
	const _TwitterAccountData *account;
	TwitterWebClient *client = NULL;

	g_return_val_if_fail(accounts != NULL, NULL);

	/* find preferred account */
	g_debug("Searching for preferred account to open timeline of user \"%s\"", friend);
	if(!(account = _twitter_client_find_preferred_account(accounts, friend, cancellable)))
	{
		/* couldn't find preferred account => use first item from account list */
		account = (_TwitterAccountData *)accounts->data;
	}

	if(account)
	{
		client = twitter_web_client_new();
		twitter_web_client_set_format(client, "xml");
		twitter_web_client_set_username(client, account->username);
		twitter_web_client_set_oauth_authorization(client, OAUTH_CONSUMER_KEY, OAUTH_CONSUMER_SECRET, account->access_key, account->access_secret);
		
	}

	return client;
}

static void
_twitter_client_copy_user(TwitterUser user, gpointer destination)
{
	memcpy(destination, &user, sizeof(TwitterUser));
}

static gboolean
_twitter_client_map_username(TwitterClient *twitter_client, TwitterDbHandle *handle, const gchar *username, gchar *guid, gint size, GError **err)
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
		if(!*err && (client = _twitter_client_create_web_client_for_friend(twitter_client->priv->accounts, username, NULL, err)))
		{
			if(twitter_web_client_get_user_details(client, username, &buffer, &length))
			{
				/* parse user information */
				twitter_xml_parse_user_details(buffer, length, _twitter_client_copy_user, &user);

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

	/* cleanup */
	g_free(buffer);

	return result;
}

static gboolean
_twitter_client_process_timline_from_db(TwitterClientTimelineType type, const gchar *username, TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	TwitterDbHandle *handle;
	gboolean result = FALSE;

	if(!(handle = twitterdb_get_handle(err)))
	{
		return FALSE;
	}

	switch(type)
	{
		case TWITTER_CLIENT_PUBLIC_TIMELINE:
			result = twitterdb_foreach_status_in_public_timeline(handle, username, func, FALSE, user_data, cancellable, err);
			break;

		case TWITTER_CLIENT_REPLIES:
			result = twitterdb_foreach_status_in_replies(handle, username, func, FALSE, user_data, cancellable, err);
			break;

		case TWITTER_CLIENT_USER_TIMELINE:
			result = twitterdb_foreach_status_in_usertimeline(handle, username, func, FALSE, user_data, cancellable, err);
			break;

		default:
			g_warning("%s: invalid timeline type", __func__);
	}

	twitterdb_close_handle(handle);

	return result;
}

static gboolean
_twitter_client_process_usertimeline_from_server(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	TwitterWebClient *client = NULL;
	gchar *key;
	gchar *xml = NULL;;
	gint length = - 1;

	/* try to get data from cache */
	key = (gchar *)g_alloca(6 + strlen(username));
	sprintf(key, "user.%s", username);
	g_debug("Searching for \"%s\" in cache", key);

	if((length = cache_load(twitter_client->priv->cache, key, &xml)) == -1)
	{
		/* item couldn't be found in cache => fetch data from Twitter service */
		g_debug("Couldn't find \"%s\" in cache, fetching data from Twitter service", key);
		if((client = _twitter_client_create_web_client_for_friend(twitter_client->priv->accounts, username, cancellable, err)))
		{
			if(twitter_web_client_get_user_timeline(client, username, &xml, &length))
			{
				/* save data in cache */
				cache_save(twitter_client->priv->cache, key, xml, length, twitter_client->priv->lifetime);
			}
			else
			{
				g_set_error(err, 0, 0, "Couldn't get usertimline \"%s\" from Twitter service", username);
			}
		}
		else
		{
			g_warning("Couldn't create TwitterWebClient instance");
		}
	}

	/* process timeline */
	if(length != -1 && xml)
	{
		twitter_xml_parse_timeline(xml, length, func, user_data, cancellable);
	}

	/* cleanup */
	if(client)
	{
		g_object_unref(client);
	}

	if(xml)
	{
		g_free(xml);
	}

	return FALSE;
}

static gboolean
_twitter_client_process_list_from_db(const gchar *username, const gchar *list, TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	TwitterDbHandle *handle;
	gboolean result = FALSE;

	if(!(handle = twitterdb_get_handle(err)))
	{
		return FALSE;
	}

	result = twitterdb_foreach_status_in_list(handle, username, list, func, user_data, cancellable, err);
	twitterdb_close_handle(handle);

	return result;
}

static gboolean
_twitter_client_update_list_membership(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, gboolean remove, GError **err)
{
	TwitterClientPrivate *priv = twitter_client->priv;
	TwitterWebClient *client = NULL;
	gchar *buffer = NULL;
	gint length;
	const GError *client_err ;
	TwitterDbHandle *handle;
	gchar list_guid[32];
	gchar user_guid[32];
	gboolean result = FALSE;

	if((handle = twitterdb_get_handle(err)))
	{
		/* map listname */
		g_debug("Mapping list: \"%s\"@\"%s\"", owner, listname);
		if(twitterdb_map_listname(handle, owner, listname, list_guid, 32, err))
		{
			g_debug("list guid => \"%s\"", list_guid);

			/* update membership */
			if((client = _twitter_client_create_web_client(priv->accounts, owner, err)))
			{
				if(remove)
				{
					result = twitter_web_client_remove_user_from_list(client, list_guid, username, &buffer, &length);
				}
				else
				{
					result = twitter_web_client_add_user_to_list(client, list_guid, username, &buffer, &length);
				}
			}

			if(result)
			{
				/* update database */
				g_debug("Mapping user: \"%s\"", username);

				if(_twitter_client_map_username(twitter_client, handle, username, user_guid, 32, err))
				{
					g_debug("user guid => \"%s\"", user_guid);

					if(remove)
					{
						g_debug("Database update: removing user \"%s\" from list \"%s\"@\"%s\"", username, owner, listname);
						result = twitterdb_remove_user_from_list(handle, list_guid, user_guid, err);
					}
					else
					{
						g_debug("Datebase update: appending user \"%s\" to list \"%s\"@\"%s\"", username, owner, listname);
						result = twitterdb_append_user_to_list(handle, list_guid, user_guid, err);
					}
				}
			}
			else
			{
				if(client && (client_err = twitter_web_client_get_last_error(client)))
				{
					g_set_error(err, 0, 0, "%s", client_err->message);
				}
			}
		}

		twitterdb_close_handle(handle);
	}

	if(client)
	{
		g_object_unref(client);
		g_free(buffer);
	}

	return result;
}

static gboolean
_twitter_client_update_friendship(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict friend, gboolean remove, GError **err)
{
	TwitterClientPrivate *priv = twitter_client->priv;
	TwitterWebClient *client = NULL;
	gchar *buffer = NULL;
	gint length;
	const GError *client_err ;
	TwitterDbHandle *handle;
	gchar account_guid[32];
	gchar user_guid[32];
	gboolean result = FALSE;

	if((handle = twitterdb_get_handle(err)))
	{
		/* map accounts */
		g_debug("Mapping users: \"%s\", \"%s\"", account, friend);
		if(_twitter_client_map_username(twitter_client, handle, account, account_guid, 32, err) && (_twitter_client_map_username(twitter_client, handle, friend, user_guid, 32, err)))
		{
			g_debug("account guid => \"%s\", user guid => \"%s\"", account_guid, user_guid);

			/* update Twitter service */
			if((client = _twitter_client_create_web_client(priv->accounts, account, err)))
			{
				if(remove)
				{
					result = twitter_web_client_remove_friend(client, friend, &buffer, &length);
				}
				else
				{
					result = twitter_web_client_add_friend(client, friend, &buffer, &length);
				}

				if(result)
				{
					/* update database */
					if(remove)
					{
						result = twitterdb_remove_follower(handle, account_guid, user_guid, err);
					}
					else
					{
						result = twitterdb_add_follower(handle, account_guid, user_guid, err);
					}
				}
				else
				{
					if(client && (client_err = twitter_web_client_get_last_error(client)))
					{
						g_set_error(err, 0, 0, "%s", client_err->message);
					}
				}
			}
		}

		twitterdb_close_handle(handle);
	}

	if(client)
	{
		g_object_unref(client);
		g_free(buffer);
	}

	return result;
}

static gboolean
_twitter_client_append_tweet_to_lists(TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict status_guid, GError **err)
{
	gchar **list_guids = NULL;
	gint i = 0;
	gboolean result = FALSE;

	g_debug("Saving tweet in related lists");
	if((list_guids = twitterdb_get_lists_following_user(handle, user_guid, err)))
	{
		g_debug("Found lists...");
		result = TRUE;

		while(list_guids[i] && result)
		{
			g_debug("Appending status \"%s\" to list \"%s\"", status_guid, list_guids[i]);
			if(!twitterdb_append_status_to_list(handle, list_guids[i], status_guid, err))
			{
				result = FALSE;
			}

			++i;
		}

		g_strfreev(list_guids);
	}
	else
	{
		g_debug("Didn't find related lists...");
		if(!(*err))
		{
			result = TRUE;
		}
	}

	return result;
}

static gboolean
_twitter_client_append_tweet_to_timelines(TwitterDbHandle *handle, GList *accounts, const gchar * restrict user_guid, const gchar * restrict username, const gchar * restrict status_guid, GError **err)
{
	GList *iter = accounts;
	gchar *account;
	gchar guid[32];
	gboolean result = TRUE;

	while(iter && result)
	{
		account = ((_TwitterAccountData *)iter->data)->username;
		g_debug("___CMP: %s == %s", account, username);

		if(g_strcmp0(account, username))
		{
			g_debug("Testing if \"%s\" is following \"%s\"", account, username);
			if(twitterdb_is_follower(handle, account, username, err))
			{
				if((result = twitterdb_map_username(handle, account, guid, 32, err)))
				{
					g_debug("Appending tweet \"%s\" to public timeline of user \"%s\"", status_guid, account);
					result = twitterdb_append_status_to_public_timeline(handle, guid, status_guid, err);
				}
			}
		}
		else
		{
			g_debug("Appending tweet \"%s\" to user timeline", status_guid);
			if((result = twitterdb_append_status_to_user_timeline(handle, user_guid, status_guid, err)))
			{
				g_debug("Appending tweet \"%s\" to public timeline of user \"%s\"", status_guid, account);
				result = twitterdb_append_status_to_public_timeline(handle, user_guid, status_guid, err);
			}
		}

		iter = iter->next;
	}

	return result;
}

static gboolean
_twitter_client_save_tweet(TwitterClient *twitter_client, TwitterDbHandle *handle, const gchar * restrict user_guid, const gchar * restrict buffer, gint length, GError **err)
{
	TwitterClientPrivate *priv = twitter_client->priv;
	TwitterStatus status;
	TwitterUser user;
	gint64 timestamp;
	gint status_count;
	gboolean result = FALSE;

	/* parse recevied data */
	g_debug("Parsing received tweet data");
	if(twitter_xml_parse_status(buffer, length, &status, &user))
	{
		/* save status */
		g_debug("Registering status (\"%s\")", status.id);
		timestamp = twitter_timestamp_to_unix_timestamp(status.created_at);
		if(twitterdb_save_status(handle, status.id, status.prev_status, user.id, status.text, timestamp, &status_count, err))
		{
			/* add status to related lists */
			g_debug("Saving tweet in lists");
			if(_twitter_client_append_tweet_to_lists(handle, user_guid, status.id, err))
			{
				/* add status to related timelines */
				g_debug("Saving tweet in timelines");
				result = _twitter_client_append_tweet_to_timelines(handle, priv->accounts, user.id, user.screen_name, status.id, err);
			}
		}
	}
	else
	{
		g_warning("Couldn't parse tweet.");
	}

	return result;
}

static gboolean
_twitter_client_publish(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict arg, gboolean retweet, GError **err)
{
	TwitterClientPrivate *priv = twitter_client->priv;
	TwitterDbHandle *handle;
	gchar user_guid[32];
	TwitterWebClient *client;
	gchar *buffer = NULL;
	gint length = 0;
	const GError *client_err = NULL;
	gboolean result = FALSE;

	if((handle = twitterdb_get_handle(err)))
	{
		/* map username */
		g_debug("Mapping username: \"%s\"", account);
		if(_twitter_client_map_username(twitter_client, handle, account, user_guid, 32, err))
		{
			g_debug("user guid => \"%s\"", user_guid);

			/* retweet data */
			if((client = _twitter_client_create_web_client(priv->accounts, account, err)))
			{
				if(retweet)
				{
					result = twitter_web_client_retweet(client, arg, &buffer, &length);
				}
				else
				{
					result = twitter_web_client_post_tweet(client, arg, &buffer, &length);
				}

				if(result)
				{
					result = _twitter_client_save_tweet(twitter_client, handle, user_guid, buffer, length, err);
				}
				else
				{
					if(client && (client_err = twitter_web_client_get_last_error(client)))
					{
						g_set_error(err, 0, 0, "%s", client_err->message);
					}
				}

				g_free(buffer);
				g_object_unref(client);
			}
		}

		twitterdb_close_handle(handle);
	}

	return result;
}

/*
 *	private:
 */
static void
_twitter_client_add_account(TwitterClient *twitter_client, const gchar * restrict username, const gchar * restrict access_key, const gchar * restrict access_secret)
{
	_TwitterAccountData *account;

	g_assert(username != NULL);
	g_assert(access_secret != NULL);
	g_assert(access_key != NULL);

	/* check if account does already exist */
	g_return_if_fail(_twitter_client_account_exists(twitter_client->priv->accounts, username) == FALSE);

	/* insert account data */
	account = (_TwitterAccountData *)g_malloc(sizeof(_TwitterAccountData));
	account->username = g_strdup(username);
	account->access_key = g_strdup(access_key);
	account->access_secret = g_strdup(access_secret);
	twitter_client->priv->accounts = g_list_append(twitter_client->priv->accounts, account);
}

static void
_twitter_client_clear_accounts(TwitterClient *twitter_client)
{
	GList *iter;
	_TwitterAccountData *account;

	if(!(iter = twitter_client->priv->accounts))
	{
		return;
	}

	while(iter)
	{
		account = (_TwitterAccountData *)iter->data;
	
		if(account->username)
		{
			g_free(account->username);
		}

		if(account->access_key)
		{
			g_free(account->access_key);
		}
	
		if(account->access_secret)
		{
			g_free(account->access_secret);
		}

		g_free(account);
		iter = iter->next;
	}

	g_list_free(twitter_client->priv->accounts);
	twitter_client->priv->accounts = NULL;
}

static gboolean
_twitter_client_process_public_timeline(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                                        gpointer user_data, GCancellable *cancellable, GError **err)

{
	return _twitter_client_process_timline_from_db(TWITTER_CLIENT_PUBLIC_TIMELINE, username, func, user_data, cancellable, err);
}

static gboolean
_twitter_client_process_replies(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                                gpointer user_data, GCancellable *cancellable, GError **err)
{
	return _twitter_client_process_timline_from_db(TWITTER_CLIENT_REPLIES, username, func, user_data, cancellable, err);
}

static gboolean
_twitter_client_process_usertimeline(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                                     gpointer user_data, GCancellable *cancellable, GError **err)
{
	/* check if specified username is registered as account */
	if(_twitter_client_account_exists(twitter_client->priv->accounts, username))
	{
		/* given username is an account => get tweets from database */
		g_debug("Fetching tweets from database");
		return _twitter_client_process_timline_from_db(TWITTER_CLIENT_USER_TIMELINE, username, func, user_data, cancellable, err);
	}
	else
	{
		/* given username is not an account => get tweets from Twitter service */
		g_debug("Fetching tweets from Twitter service");
		return _twitter_client_process_usertimeline_from_server(twitter_client, username, func, user_data, cancellable, err);
	}

	return FALSE;
}

static gboolean
_twitter_client_process_list(TwitterClient *twitter_client, const gchar * restrict username, const gchar * restrict list,
                             TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	/* check if specified username is registered as account */
	if(_twitter_client_account_exists(twitter_client->priv->accounts, username))
	{
		/* given username is an account => get tweets from database */
		g_debug("Fetching tweets from database");
		return _twitter_client_process_list_from_db(username, list, func, user_data, cancellable, err);
	}
	else
	{
		/* given username is not an account => get tweets from Twitter service */
		g_debug("Fetching tweets from Twitter service");
		g_warning("Warning: fetching list from Twitter server not implemented yet");
	}

	return FALSE;
}

static gboolean
_twitter_client_search(TwitterClient *twitter_client, const gchar * restrict username, const gchar * restrict query,
                       TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	gchar *key;
	TwitterWebClient *client = NULL;
	gchar *buffer = NULL;
	gint length;
	gboolean result = FALSE;

	/* try to get data from cache */
	key = g_strdup_printf("search.%s.%s", username, query);
	g_debug("Searching for \"%s\" in cache", key);

	if((length = cache_load(twitter_client->priv->cache, key, &buffer)) == -1)
	{
		/* get search result from Twitter */
		g_debug("Trying to get search result from Twitter");

		if((client = _twitter_client_create_web_client(twitter_client->priv->accounts, username, err)))
		{
			g_object_set(G_OBJECT(client), "format", "json", "status-count", TWITTER_CLIENT_SEARCH_STATUS_COUNT, NULL);

			if(twitter_web_client_search(client, query, &buffer, &length))
			{
				twitter_json_parse_search_result(buffer, length, func, user_data, cancellable);

				/* save search result in cache */
				cache_save(twitter_client->priv->cache, key, buffer, length, twitter_client->priv->lifetime);
			}

			g_object_unref(client);
		}
	}
	else
	{
		twitter_json_parse_search_result(buffer, length, func, user_data, cancellable);
	}

	g_free(key);
	g_free(buffer);

	return result;
}


static gboolean
_twitter_client_remove_user_from_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err)
{
	return _twitter_client_update_list_membership(twitter_client, owner, listname, username, TRUE, err);
}

static gboolean
_twitter_client_add_user_to_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err)
{
	return _twitter_client_update_list_membership(twitter_client, owner, listname, username, FALSE, err);
}

static gchar *
_twitter_client_update_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict new_listname, const gchar * restrict description, gboolean protected, GError **err)
{
	TwitterWebClient *client = NULL;
	gchar *buffer = NULL;
	gint length;
	gchar guid[32];
	TwitterUser user_data;
	TwitterList list_data;
	TwitterDbHandle *handle;
	gchar *result = NULL;

	/* update list information */
	if((handle = twitterdb_get_handle(err)))
	{
		if(twitterdb_map_listname(handle, owner, listname, guid, 32, err))
		{
			if((client = _twitter_client_create_web_client(twitter_client->priv->accounts, owner, err)))
			{
				g_debug("Updating list: \"%s\"@\"%s\" (guid=\"%s\")", owner, listname, guid);
				if(twitter_web_client_update_list(client, guid, new_listname, description, protected, &buffer, &length))
				{
					g_debug("Parsing list data");
					if(twitter_xml_parse_list(buffer, length, &user_data, &list_data))
					{
						g_debug("Found new list name: \"%s\"@\"%s\"", user_data.screen_name, list_data.name);
						result = g_strdup(list_data.name);

						/* update database */
						g_debug("Updating database");
						if(!twitterdb_save_list(handle,
									list_data.id,
									user_data.id,
									list_data.name,
									list_data.fullname,
									list_data.uri,
									list_data.description,
									list_data.protected,
									list_data.subscriber_count,
									list_data.member_count,
									err))
						{
							g_warning("Couldn't update list database entry");
						}
					}
					else
					{
						g_set_error(err, 0, 0, "Couldn't parse list data.");
					}
				}
				else
				{
					g_set_error(err, 0, 0, "Couldn't update list: \"%s\"@\"%s\". Please try again later.", owner, listname);
				}
			}

			twitterdb_close_handle(handle);
		}

		g_free(buffer);
		g_object_unref(client);
	}
	else
	{
		g_warning("Couldn't create TwitterWebClient instance");
	}

	return result;
}

static gboolean
_twitter_client_add_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, gboolean protected, GError **err)
{
	TwitterClientPrivate *priv = twitter_client->priv;
	TwitterWebClient *client;
	gchar *buffer = NULL;
	gint length;
	const GError *client_err;
	TwitterDbHandle *handle;
	TwitterUser user_data;
	TwitterList list_data;
	gboolean result = FALSE;

	/* add list */
	if((client = _twitter_client_create_web_client(priv->accounts, owner, err)))
	{
		if((result = twitter_web_client_add_list(client, listname, protected, &buffer, &length)))
		{
			/* parse buffer */
			g_debug("Parsing list data");
			if(twitter_xml_parse_list(buffer, length, &user_data, &list_data))
			{
				/* update database */
				if((handle = twitterdb_get_handle(err)))
				{
					if(!twitterdb_save_list(handle,
					                        list_data.id,
					                        user_data.id,
					                        list_data.name,
					                        list_data.fullname,
					                        list_data.uri,
					                        list_data.description,
					                        list_data.protected,
					                        list_data.subscriber_count,
					                        list_data.member_count,
					                        err))
					{
						g_warning("Couldn't update list database entry");
					}

					twitterdb_close_handle(handle);
				}
			}
			else
			{
				g_set_error(err, 0, 0, "Couldn't parse list data.");
			}
		}
		else
		{
			if(client && (client_err = twitter_web_client_get_last_error(client)))
			{
				g_set_error(err, 0, 0, "%s", client_err->message);
			}
		}

		g_object_unref(client);
	}

	/* cleanup */
	g_free(buffer);

	return result;
}

static gboolean
_twitter_client_remove_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, GError **err)
{
	TwitterClientPrivate *priv = twitter_client->priv;
	TwitterWebClient *client = NULL;
	gchar *buffer = NULL;
	gint length;
	const GError *client_err;
	TwitterDbHandle *handle;
	gchar list_guid[32];
	gboolean result = FALSE;

	if((handle = twitterdb_get_handle(err)))
	{
		/* map listname */
		g_debug("Mapping list: \"%s\"@\"%s\"", owner, listname);
		if(twitterdb_map_listname(handle, owner, listname, list_guid, 32, err))
		{
			g_debug("list guid => \"%s\"", list_guid);

			/* remove list */
			if((client = _twitter_client_create_web_client(priv->accounts, owner, err)))
			{
				result = twitter_web_client_remove_list(client, list_guid, &buffer, &length);
			}

			if(result)
			{
				g_debug("Removing list \"%s\"@\"%s\"", owner, listname);
				result = twitterdb_remove_list(handle, list_guid, err);
			}
			else
			{
				if(client && (client_err = twitter_web_client_get_last_error(client)))
				{
					g_set_error(err, 0, 0, "%s", client_err->message);
				}
			}
		}

		twitterdb_close_handle(handle);
	}

	if(client)
	{
		g_object_unref(client);
		g_free(buffer);
	}

	return result;
}

static void
_twitter_client_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	TwitterClient *twitter_client = TWITTER_CLIENT(object);

	switch(prop_id)
	{
		case PROP_CACHE:
			g_value_set_object(value, twitter_client->priv->cache);
			break;
	
		case PROP_LIFETIME:
			g_value_set_int(value, twitter_client->priv->lifetime);
			break;
	
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
_twitter_client_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	TwitterClient *twitter_client = TWITTER_CLIENT(object);

	switch(prop_id)
	{
		case PROP_CACHE:
			if(twitter_client->priv->cache)
			{
				g_object_unref(twitter_client->priv->cache);
			}
			twitter_client->priv->cache = g_value_get_object(value);
			g_object_ref(twitter_client->priv->cache);
			break;

		case PROP_LIFETIME:
			twitter_client->priv->lifetime = g_value_get_int(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static gboolean
_twitter_client_block_follower(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict follower, GError **err)
{
	TwitterClientPrivate *priv = twitter_client->priv;
	TwitterWebClient *client = NULL;
	gchar *buffer = NULL;
	gint length;
	const GError *client_err ;
	TwitterDbHandle *handle;
	gchar account_guid[32];
	gchar user_guid[32];
	gboolean result = FALSE;

	if((handle = twitterdb_get_handle(err)))
	{
		/* map account */
		g_debug("Mapping users: \"%s\", \"%s\"", account, follower);
		if(_twitter_client_map_username(twitter_client, handle, account, account_guid, 32, err) && (_twitter_client_map_username(twitter_client, handle, follower, user_guid, 32, err)))
		{
			g_debug("account guid => \"%s\", user guid => \"%s\"", account_guid, user_guid);

			/* update Twitter server */
			if((client = _twitter_client_create_web_client(priv->accounts, account, err)))
			{
				if(twitter_web_client_block_user(client, follower, &buffer, &length))
				{
					/* update database */
					result = twitterdb_remove_follower(handle, user_guid, account_guid, err);
				}
				else
				{
					if(client && (client_err = twitter_web_client_get_last_error(client)))
					{
						g_set_error(err, 0, 0, "%s", client_err->message);
					}
				}
			}
		}

		twitterdb_close_handle(handle);
	}

	if(client)
	{
		g_object_unref(client);
		g_free(buffer);
	}

	return result;
}

static gboolean
_twitter_client_remove_friend(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict friend, GError **err)
{
	return _twitter_client_update_friendship(twitter_client, account, friend, TRUE, err);
}

static gboolean
_twitter_client_add_friend(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict friend, GError **err)
{
	return _twitter_client_update_friendship(twitter_client, account, friend, FALSE, err);
}

static gboolean
_twitter_client_post(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict text, GError **err)
{
	return _twitter_client_publish(twitter_client, account, text, FALSE, err);
}

static gboolean
_twitter_client_retweet(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict text, GError **err)
{
	return _twitter_client_publish(twitter_client, account, text, TRUE, err);
}

/*
 *	public:
 */
void
twitter_client_add_account(TwitterClient *twitter_client, const gchar * restrict username, const gchar * restrict access_key, const gchar * restrict access_secret)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->add_account(twitter_client, username, access_key, access_secret);
}

void
twitter_client_clear_accounts(TwitterClient *twitter_client)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->clear_accounts(twitter_client);
}

gboolean
twitter_client_process_public_timeline(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                                       gpointer user_data, GCancellable *cancellable, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->process_public_timeline(twitter_client, username, func, user_data, cancellable, err);
}

gboolean
twitter_client_process_replies(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                               gpointer user_data, GCancellable *cancellable, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->process_replies(twitter_client, username, func, user_data, cancellable, err);
}

gboolean
twitter_client_process_usertimeline(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                                    gpointer user_data, GCancellable *cancellable, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->process_usertimeline(twitter_client, username, func, user_data, cancellable, err);
}

gboolean
twitter_client_process_list(TwitterClient *twitter_client, const gchar * restrict username, const gchar * restrict list,
                            TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->process_list(twitter_client, username, list, func, user_data, cancellable, err);
}

gboolean
twitter_client_search(TwitterClient *twitter_client, const gchar * restrict username, const gchar * restrict query,
                      TwitterProcessStatusFunc func, gpointer user_data, GCancellable *cancellable, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->search(twitter_client, username, query, func, user_data, cancellable, err);
}

gboolean
twitter_client_add_user_to_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->add_user_to_list(twitter_client, owner, listname, username, err);
}

gboolean
twitter_client_remove_user_from_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->remove_user_from_list(twitter_client, owner, listname, username, err);
}

gchar *
twitter_client_update_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict new_listname, const gchar * restrict description, gboolean protected, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->update_list(twitter_client, owner, listname, new_listname, description, protected, err);
}

gboolean
twitter_client_add_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, gboolean protected, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->add_list(twitter_client, owner, listname, protected, err);
}

gboolean
twitter_client_remove_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->remove_list(twitter_client, owner, listname, err);
}

gboolean
twitter_client_block_follower(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict follower, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->block_follower(twitter_client, account, follower, err);
}

gboolean
twitter_client_remove_friend(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict friend, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->remove_friend(twitter_client, account, friend, err);
}

gboolean
twitter_client_add_friend(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict friend, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->add_friend(twitter_client, account, friend, err);
}

gboolean
twitter_client_post(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict text, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->post(twitter_client, account, text, err);
}

gboolean
twitter_client_retweet(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict guid, GError **err)
{
	return TWITTER_CLIENT_GET_CLASS(twitter_client)->retweet(twitter_client, account, guid, err);
}

TwitterClient *
twitter_client_new(Cache *cache, gint lifetime)
{
	TwitterClient *twitter_client;

	twitter_client = (TwitterClient *)g_object_new(TWITTER_CLIENT_TYPE, "cache", cache, "lifetime", lifetime, NULL);

	return twitter_client;
}

/*
 *	initialization/finalization:
 */
static void
_twitter_client_finalize(GObject *object)
{
	TwitterClient *twitter_client = TWITTER_CLIENT(object);

	_twitter_client_clear_accounts(twitter_client);

	if(twitter_client->priv->cache)
	{
		g_object_unref(twitter_client->priv->cache);
	}

	if(G_OBJECT_CLASS(twitter_client_parent_class)->finalize)
	{
		(*G_OBJECT_CLASS(twitter_client_parent_class)->finalize)(object);
	}
}

static void
twitter_client_class_init(TwitterClientClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(TwitterClientClass));

	gobject_class->finalize = _twitter_client_finalize;
	gobject_class->get_property = _twitter_client_get_property;
	gobject_class->set_property = _twitter_client_set_property;

	klass->add_account = _twitter_client_add_account;
	klass->clear_accounts = _twitter_client_clear_accounts;
	klass->process_public_timeline = _twitter_client_process_public_timeline;
	klass->process_replies = _twitter_client_process_replies;
	klass->process_usertimeline = _twitter_client_process_usertimeline;
	klass->process_list = _twitter_client_process_list;
	klass->search = _twitter_client_search;
	klass->add_user_to_list = _twitter_client_add_user_to_list;
	klass->remove_user_from_list = _twitter_client_remove_user_from_list;
	klass->update_list = _twitter_client_update_list;
	klass->add_list = _twitter_client_add_list;
	klass->remove_list = _twitter_client_remove_list;
	klass->block_follower = _twitter_client_block_follower;
	klass->remove_friend = _twitter_client_remove_friend;
	klass->add_friend = _twitter_client_add_friend;
	klass->post = _twitter_client_post;
	klass->retweet = _twitter_client_retweet;

	g_object_class_install_property(gobject_class, PROP_CACHE,
	                                g_param_spec_object("cache", NULL, NULL, CACHE_TYPE, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_LIFETIME,
	                                g_param_spec_int("lifetime", NULL, NULL, 30, G_MAXINT, 30, G_PARAM_READWRITE));
}

static void
twitter_client_init(TwitterClient *twitterclient)
{
	/* register private data */
	twitterclient->priv = G_TYPE_INSTANCE_GET_PRIVATE(twitterclient, TWITTER_CLIENT_TYPE, TwitterClientPrivate);
}

/**
 * @}
 * @}
 */


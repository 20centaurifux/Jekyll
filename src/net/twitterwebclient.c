/***************************************************************************
    begin........: April 2010
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
 * \file twitterwebclient.c
 * \brief A Twitter client.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 23. December 2011
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "twitterwebclient.h"
#include "httpclient.h"
#include "../oauth/oauth.h"

/**
 * @addtogroup Net
 * @{
 *	@addtogroup Twitter
 *	@{
 */

/*! Defines TwitterWebClient. */
G_DEFINE_TYPE(TwitterWebClient, twitter_web_client, G_TYPE_OBJECT);

enum
{
	PROP_0,
	PROP_USERNAME,
	PROP_OAUTH_CONSUMER_KEY,
	PROP_OAUTH_CONSUMER_SECRET,
	PROP_OAUTH_ACCESS_KEY,
	PROP_OAUTH_ACCESS_SECRET,
	PROP_FORMAT,
	PROP_STATUS_COUNT
};

/**
 * \struct _TwitterWebClientPrivate
 * \brief Private _TwitterWebClient data.
 */
struct _TwitterWebClientPrivate
{
	/*! Name of the user account. */
	gchar *username;
	/*! OAuth consumer key. */
	gchar *consumer_key;
	/*! OAuth consumer secret. */
	gchar *consumer_secret;
	/*! OAuth access key. */
	gchar *access_key;
	/*! OAuth access secret. */
	gchar *access_secret;
	/*! The desired data format. */
	gchar *format;
	/*! Number of statuses to receive. */
	gint status_count;
	/*! Holds error messages. */
	GError *err;
};

/*
 *	helpers:
 */
static void
_twitter_web_client_clear_last_error(TwitterWebClient *twitterwebclient)
{
	if(twitterwebclient->priv->err)
	{
		g_error_free(twitterwebclient->priv->err);
		twitterwebclient->priv->err = NULL;
	}
}

static void
_twitter_web_client_set_last_error(TwitterWebClient *twitterwebclient, gint code, const gchar *format, ...)
{
	va_list ap;

	_twitter_web_client_clear_last_error(twitterwebclient);

	va_start(ap, format);
	twitterwebclient->priv->err = g_error_new_valist(0, code, format, ap);
	va_end(ap);
}

static void
_twitter_web_client_extract_key_and_value(const gchar *text, gchar **key, gchar **value)
{
	gint length = strlen(text);
	gint pos = -1;

	g_assert(text != NULL);

	*key = NULL;
	*value = NULL;

	for(gint i = 0; i < length; ++i)
	{
		if(text[i] == '=')
		{
			pos = i;
			break;
		}
	}

	*key = (gchar *)g_malloc(sizeof(gchar) * (pos + 1));
	memcpy(*key, text, pos);
	(*key)[pos] = 0;
	*value = g_strdup(text + pos + 1);
}

static gboolean
_twitter_web_client_send_request(TwitterWebClient *twitterwebclient, const gchar *path, gchar * restrict keys[], gchar * restrict values[], gint argc, gboolean post, gchar **buffer, gint *length)
{
	gchar *api_url;
	gchar *url;
	gint i;
	gint array_length = 0;
	gchar *post_data = NULL;
	gchar **list = NULL;
	gchar **post_keys = NULL;
	gchar **post_values = NULL;
	HttpClient *client;
	gint status;

	//_twitter_web_client_test_rate_limit(twitterwebclient);

	/* clear last error */
	_twitter_web_client_clear_last_error(twitterwebclient);

	/* build API url */
	api_url = g_strdup_printf("http://api.twitter.com%s", path);

	/* sign url */
	if(post)
	{
		url = oauth_sign_url2(api_url,
	                              &post_data,
	                              OA_HMAC,
	                              NULL,
	                              twitterwebclient->priv->consumer_key,
	                              twitterwebclient->priv->consumer_secret,
	                              twitterwebclient->priv->access_key,
	                              twitterwebclient->priv->access_secret);

		/* create post arrays */
		list = g_strsplit(post_data, "&", -1);
		array_length = g_strv_length(list);

		post_keys = (gchar **)g_malloc(sizeof(gchar *) * (array_length + argc));
		post_values = (gchar **)g_malloc(sizeof(gchar *) * (array_length + argc));

		for(i = 0; i < array_length; ++i)
		{
			_twitter_web_client_extract_key_and_value(list[i], &post_keys[i], &post_values[i]);
		}
	}
	else
	{
		url = oauth_sign_url2(api_url,
	                              NULL,
	                              OA_HMAC,
	                              NULL,
	                              twitterwebclient->priv->consumer_key,
	                              twitterwebclient->priv->consumer_secret,
	                              twitterwebclient->priv->access_key,
	                              twitterwebclient->priv->access_secret);
	}

	client = http_client_new("hostname", TWITTER_API_HOSTNAME, "port", HTTP_DEFAULT_PORT, NULL);
	
	if(post)
	{
		g_object_set(client, "auto-escape", FALSE, NULL);
		status = http_client_post(client, url + 22, (const gchar **)post_keys, (const gchar **)post_values, array_length, &twitterwebclient->priv->err);
	}
	else
	{
		status = http_client_get(client, url + 22, &twitterwebclient->priv->err);
	}

	if(status == HTTP_OK)
	{
		/* read response */
		http_client_read_content(client, buffer, length);

		if(*length == -1 && *buffer)
		{
			g_free(*buffer);
			*buffer = NULL;
		}
	}
	else
	{
		if(status == HTTP_UNAUTHORIZED || status == HTTP_FORBIDDEN)
		{
			_twitter_web_client_set_last_error(twitterwebclient, status, "Couldn't connect to %s: user is not authorized.", TWITTER_API_HOSTNAME);
		}
		else
		{
			_twitter_web_client_set_last_error(twitterwebclient, status, "Couldn't connect to %s, please try again later.", TWITTER_API_HOSTNAME);
		}
	}

	/* free memory */
	g_object_unref(client);

	if(post)
	{
		for(i = 0; i < array_length; ++i)
		{
			g_free(post_keys[i]);
			g_free(post_values[i]);
		}

		g_free(post_keys);
		g_free(post_values);
		g_strfreev(list);
		free(post_data);
	}

	g_free(api_url);
	free(url);

	return (status == HTTP_OK) ? TRUE : FALSE;
}

/*
 *	implementation:
 */
static const GError *
_twitter_web_client_get_last_error(TwitterWebClient *twitterwebclient)
{
	return twitterwebclient->priv->err;
}

static void
_twitter_web_client_set_oauth_authorization(TwitterWebClient *twitterwebclient, const gchar * restrict consumer_key, const gchar * restrict consumer_secret, const gchar * restrict access_key, const gchar * restrict access_secret)
{
	g_object_set(G_OBJECT(twitterwebclient), "oauth-consumer-key", consumer_key, "oauth-consumer-secret", consumer_secret, "oauth-access-key", access_key, "oauth-access-secret", access_secret, NULL);
}

static void
_twitter_web_client_set_username(TwitterWebClient *twitterwebclient, const gchar *username)
{
	g_object_set(G_OBJECT(twitterwebclient), "username", username, NULL);
}

static const gchar *
_twitter_web_client_get_username(TwitterWebClient *twitterwebclient)
{
	return twitterwebclient->priv->username;
}

static void
_twitter_web_client_set_format(TwitterWebClient *twitterwebclient, const gchar *format)
{
	g_object_set(G_OBJECT(twitterwebclient), "format", format, NULL);
}

static gboolean
_twitter_web_client_get_home_timeline(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length)
{
	gchar path[48];
	gboolean result;

	sprintf(path, "/1/statuses/home_timeline.%s?count=%d", twitterwebclient->priv->format, twitterwebclient->priv->status_count);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);

	return result;
}

static gboolean
_twitter_web_client_get_mentions(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length)
{
	gchar path[48];
	gboolean result;

	sprintf(path, "/1/statuses/mentions.%s?count=%d", twitterwebclient->priv->format, twitterwebclient->priv->status_count);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);

	return result;
}

static gboolean
_twitter_web_client_get_direct_messages(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length)
{
	gchar path[48];
	gboolean result;

	sprintf(path, "/1/direct_messages.%s?count=%d", twitterwebclient->priv->format, twitterwebclient->priv->status_count);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);

	return result;
}

static gboolean
_twitter_web_client_get_user_timeline(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length)
{
	gchar *path;
	gboolean result;

	if(!username)
	{
		username = twitterwebclient->priv->username;
	}

	path = g_markup_printf_escaped("/1/statuses/user_timeline.%s?screen_name=%s&count=%d", twitterwebclient->priv->format, username, twitterwebclient->priv->status_count);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_get_timeline_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict listname, gchar **buffer, gint *length)
{
	gchar *path;
	const gchar *user = username;
	gboolean result;

	if(!username)
	{
		user = twitterwebclient->priv->username;
	}

	path = g_markup_printf_escaped("/1/%s/lists/%s/statuses.%s?count=%d", user, listname, twitterwebclient->priv->format, twitterwebclient->priv->status_count);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_get_lists(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length)
{
	gchar *path;
	const gchar *user = username;
	gboolean result;

	if(!username)
	{
		user = twitterwebclient->priv->username;
	}

	path = g_markup_printf_escaped("/1/%s/lists.%s?count=%d", user, twitterwebclient->priv->format, twitterwebclient->priv->status_count);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_get_users_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict listname, const gchar * restrict cursor, gchar **buffer, gint *length)
{
	gchar *path;
	gboolean result;

	path = g_markup_printf_escaped("/1/%s/%s/members.%s?cursor=%s", twitterwebclient->priv->username, listname, twitterwebclient->priv->format, cursor);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_add_list(TwitterWebClient *twitterwebclient, const gchar *listname, gboolean protected, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "name", "mode" };
	gchar *values[2];
	gboolean result;

	values[0] = g_uri_escape_string(listname, NULL, TRUE);
	values[1] = protected ? "private" : "public";

	path = g_strdup_printf("/1/%s/lists.%s?name=%s&mode=%s", twitterwebclient->priv->username, twitterwebclient->priv->format, values[0], values[1]);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 2, TRUE, buffer, length);
	g_free(path);
	g_free(values[0]);

	return result;
}

static gboolean
_twitter_web_client_remove_list(TwitterWebClient *twitterwebclient, const gchar *listname, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "_method" };
	gchar *values[] = { "DELETE" };
	gboolean result;

	values[0] = g_uri_escape_string(listname, NULL, TRUE);

	path = g_markup_printf_escaped("/1/%s/lists/%s.%s?_method=DELETE", twitterwebclient->priv->username, listname, twitterwebclient->priv->format);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 1, TRUE, buffer, length);
	g_free(path);
	g_free(values[0]);

	return result;
}

static gboolean
_twitter_web_client_update_list(TwitterWebClient *twitterwebclient, const gchar *listname, const gchar *new_listname, const gchar *description, gboolean protected, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "name", "mode", "description" };
	gchar *values[3];
	gboolean result;

	values[0] = g_uri_escape_string(new_listname, NULL, TRUE);
	values[1] = protected ? "private" : "public";
	values[2] = g_uri_escape_string(description, NULL, TRUE);

	path = g_markup_printf_escaped("/1/%s/lists/%s.%s?name=%s&mode=%s&description=%s", twitterwebclient->priv->username, listname, twitterwebclient->priv->format, values[0], values[1], values[2]);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 3, TRUE, buffer, length);
	g_free(path);
	g_free(values[0]);
	g_free(values[2]);

	return result;
}

static gboolean
_twitter_web_client_add_user_to_list(TwitterWebClient *twitterwebclient, const gchar * restrict listname, const gchar * restrict username, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "list_id", "id" };
	gchar *values[2];
	gboolean result;

	values[0] = g_uri_escape_string(listname, NULL, TRUE);
	values[1] = g_uri_escape_string(username, NULL, TRUE);

	path = g_markup_printf_escaped("/1/%s/%s/members.%s?list_id=%s&id=%s", twitterwebclient->priv->username, listname, twitterwebclient->priv->format, values[0], values[1]);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 2, TRUE, buffer, length);
	g_free(values[0]);
	g_free(values[1]);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_remove_user_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict listname, const gchar * restrict username, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "list_id", "id", "_method" };
	gchar *values[3];
	gboolean result;

	values[0] = g_uri_escape_string(listname, NULL, TRUE);
	values[1] = g_uri_escape_string(username, NULL, TRUE);
	values[2] = "DELETE";

	path = g_markup_printf_escaped("/1/%s/%s/members.%s?list_id=%s&id=%s&_method=%s", twitterwebclient->priv->username, listname, twitterwebclient->priv->format, values[0], values[1], values[2]);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 3, TRUE, buffer, length);
	g_free(values[0]);
	g_free(values[1]);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_search(TwitterWebClient *twitterwebclient, const gchar *word, gchar **buffer, gint *length)
{
	GString *path;
	HttpClient *client;
	gint status;
	gboolean result = FALSE;

	path = g_string_new("/search.rss?rpp=40&amp;include_entities=true&amp;result_type=mixed&q=");
	path = g_string_append_uri_escaped(path, word, NULL, TRUE);

	client = http_client_new("hostname", TWITTER_SEARCH_API_HOSTNAME, "port", HTTP_DEFAULT_PORT, NULL);
	status = http_client_get(client, path->str, &twitterwebclient->priv->err);

	if(status == HTTP_OK)
	{
		/* read response */
		http_client_read_content(client, buffer, length);
		result = TRUE;
	}

	g_string_free(path, TRUE);
	g_object_unref(client);

	return result;
}

static gboolean
_twitter_web_client_get_followers(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict cursor, gchar **buffer, gint *length)
{
	gchar *path;
	const gchar *user = username;
	gboolean result;

	if(!username)
	{
		user = twitterwebclient->priv->username;
	}

	path = g_markup_printf_escaped("/1/followers/ids.%s?screen_name=%s&cursor=%s", twitterwebclient->priv->format, user, cursor);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_get_friends(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict cursor, gchar **buffer, gint *length)
{
	gchar *path;
	const gchar *user = username;
	gboolean result;

	if(!username)
	{
		user = twitterwebclient->priv->username;
	}

	path = g_markup_printf_escaped("/1/friends/ids.%s?screen_name=%s&cursor=%s", twitterwebclient->priv->format, user, cursor);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_add_friend(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "screen_name" };
	gchar *values[1];
	gboolean result;

	values[0] = g_uri_escape_string(friend, NULL, TRUE);

	path = g_markup_printf_escaped("/1/friendships/create.%s?screen_name=%s", twitterwebclient->priv->format, values[0]);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 1, TRUE, buffer, length);
	g_free(path);
	g_free(values[0]);

	return result;
}

static gboolean
_twitter_web_client_remove_friend(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "screen_name" };
	gchar *values[1];
	gboolean result;

	values[0] = g_uri_escape_string(friend, NULL, TRUE);

	path = g_markup_printf_escaped("/1/friendships/destroy.%s?screen_name=%s", twitterwebclient->priv->format, values[0]);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 1, TRUE, buffer, length);
	g_free(path);
	g_free(values[0]);

	return result;
}

static gboolean
_twitter_web_client_block_user(TwitterWebClient *twitterwebclient, const gchar *user, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "screen_name" };
	gchar *values[1];
	gboolean result;

	values[0] = g_uri_escape_string(user, NULL, TRUE);

	path = g_markup_printf_escaped("/1/blocks/create.%s?screen_name=%s", twitterwebclient->priv->format, values[0]);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 1, TRUE, buffer, length);
	g_free(path);
	g_free(values[0]);

	return result;
}

static gboolean
_twitter_web_client_get_user_details(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length)
{
	gchar *path;
	gboolean result;

	path = g_markup_printf_escaped("/1/users/show.%s?screen_name=%s", twitterwebclient->priv->format, username);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_get_user_details_by_id(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length)
{
	gchar *path;
	gboolean result;

	path = g_markup_printf_escaped("/1/users/show.%s?user_id=%s", twitterwebclient->priv->format, id);
	result = _twitter_web_client_send_request(twitterwebclient, path, NULL, NULL, 0, FALSE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_post_tweet(TwitterWebClient *twitterwebclient, const gchar *text, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "status" };
	gchar *values[1];
	gboolean result;

	values[0] = g_uri_escape_string(text, NULL, TRUE);

	path = g_markup_printf_escaped("/1/statuses/update.%s?status=%s", twitterwebclient->priv->format, values[0]);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 1, TRUE, buffer, length);
	g_free(path);
	g_free(values[0]);

	return result;
}

static gboolean
_twitter_web_client_remove_tweet(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "id" };
	gchar *values[] = { (gchar *)id };
	gboolean result;

	path = g_markup_printf_escaped("/1/statuses/destroy/%s.%s", values[0], twitterwebclient->priv->format);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 1, TRUE, buffer, length);
	g_free(path);

	return result;
}

static gboolean
_twitter_web_client_retweet(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length)
{
	gchar *path;
	gchar *keys[] = { "id" };
	gchar *values[] = { (gchar *)id };
	gboolean result;

	path = g_markup_printf_escaped("/1/statuses/retweet/%s.%s", values[0], twitterwebclient->priv->format);
	result = _twitter_web_client_send_request(twitterwebclient, path, keys, values, 1, TRUE, buffer, length);
	g_free(path);

	return result;
}

static void
_twitter_web_client_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	TwitterWebClient *twitterwebclient = TWITTER_WEB_CLIENT(object);

	switch(prop_id)
	{
		case PROP_USERNAME:
			g_value_set_string(value, twitterwebclient->priv->username);
			break;

		case PROP_OAUTH_CONSUMER_KEY:
			g_value_set_string(value, twitterwebclient->priv->consumer_key);
			break;
	
		case PROP_OAUTH_CONSUMER_SECRET:
			g_value_set_string(value, twitterwebclient->priv->consumer_secret);
			break;

		case PROP_OAUTH_ACCESS_KEY:
			g_value_set_string(value, twitterwebclient->priv->access_key);
			break;
	
		case PROP_OAUTH_ACCESS_SECRET:
			g_value_set_string(value, twitterwebclient->priv->access_secret);
			break;
	
		case PROP_FORMAT:
			g_value_set_string(value, twitterwebclient->priv->format);
			break;
	
		case PROP_STATUS_COUNT:
			g_value_set_int(value, twitterwebclient->priv->status_count);
			break;
	
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
_twitter_web_client_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	TwitterWebClient *twitterwebclient = TWITTER_WEB_CLIENT(object);

	switch(prop_id)
	{
		case PROP_USERNAME:
			if(twitterwebclient->priv->username)
			{
				g_free(twitterwebclient->priv->username);
			}
			twitterwebclient->priv->username = g_value_dup_string(value);
			break;
	
		case PROP_OAUTH_CONSUMER_KEY:
			if(twitterwebclient->priv->consumer_key)
			{
				g_free(twitterwebclient->priv->consumer_key);
			}
			twitterwebclient->priv->consumer_key = g_value_dup_string(value);
			break;

		case PROP_OAUTH_CONSUMER_SECRET:
			if(twitterwebclient->priv->consumer_secret)
			{
				g_free(twitterwebclient->priv->consumer_secret);
			}
			twitterwebclient->priv->consumer_secret = g_value_dup_string(value);
			break;

		case PROP_OAUTH_ACCESS_KEY:
			if(twitterwebclient->priv->access_key)
			{
				g_free(twitterwebclient->priv->access_key);
			}
			twitterwebclient->priv->access_key = g_value_dup_string(value);
			break;

		case PROP_OAUTH_ACCESS_SECRET:
			if(twitterwebclient->priv->access_secret)
			{
				g_free(twitterwebclient->priv->access_secret);
			}
			twitterwebclient->priv->access_secret = g_value_dup_string(value);
			break;

		case PROP_FORMAT:
			if(twitterwebclient->priv->format)
			{
				g_free(twitterwebclient->priv->format);
			}
			twitterwebclient->priv->format = g_value_dup_string(value);
			break;

		case PROP_STATUS_COUNT:
			twitterwebclient->priv->status_count = g_value_get_int(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/*
 *	public:
 */
const GError *
twitter_web_client_get_last_error(TwitterWebClient *twitterwebclient)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_last_error(twitterwebclient);
}

void
twitter_web_client_set_oauth_authorization(TwitterWebClient *twitterwebclient, const gchar * restrict consumer_key, const gchar * restrict consumer_secret, const gchar * restrict access_key, const gchar * restrict access_secret)
{
	TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->set_oauth_authorization(twitterwebclient, consumer_key, consumer_secret, access_key, access_secret);
}

const gchar *
twitter_web_client_get_username(TwitterWebClient *twitterwebclient)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_username(twitterwebclient);
}

void
twitter_web_client_set_username(TwitterWebClient *twitterwebclient, const gchar *username)
{
	TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->set_username(twitterwebclient, username);
}

void
twitter_web_client_set_format(TwitterWebClient *twitterwebclient, const gchar *format)
{
	TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->set_format(twitterwebclient, format);
}

gboolean
twitter_web_client_get_home_timeline(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_home_timeline(twitterwebclient, buffer, length);
}

gboolean
twitter_web_client_get_mentions(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_mentions(twitterwebclient, buffer, length);
}

gboolean
twitter_web_client_get_direct_messages(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_direct_messages(twitterwebclient, buffer, length);
}

gboolean
twitter_web_client_get_user_timeline(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_user_timeline(twitterwebclient, username, buffer, length);
}

gboolean
twitter_web_client_get_timeline_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict listname, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_timeline_from_list(twitterwebclient, username, listname, buffer, length);
}

gboolean
twitter_web_client_get_lists(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_lists(twitterwebclient, username, buffer, length);
}

gboolean
twitter_web_client_get_users_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict listname, const gchar * restrict cursor, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_users_from_list(twitterwebclient, username, listname, cursor, buffer, length);
}

gboolean
twitter_web_client_add_list(TwitterWebClient *twitterwebclient, const gchar *listname, gboolean protected, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->add_list(twitterwebclient, listname, protected, buffer, length);
}

gboolean
twitter_web_client_remove_list(TwitterWebClient *twitterwebclient, const gchar *listname, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->remove_list(twitterwebclient, listname, buffer, length);
}

gboolean
twitter_web_client_update_list(TwitterWebClient *twitterwebclient, const gchar *listname, const gchar *new_listname, const gchar *description, gboolean protected, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->update_list(twitterwebclient, listname, new_listname, description, protected, buffer, length);
}


gboolean
twitter_web_client_add_user_to_list(TwitterWebClient *twitterwebclient, const gchar * restrict listname, const gchar * restrict username, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->add_user_to_list(twitterwebclient, listname, username, buffer, length);
}

gboolean
twitter_web_client_remove_user_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict listname, const gchar * restrict username, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->remove_user_from_list(twitterwebclient, listname, username, buffer, length);
}

gboolean
twitter_web_client_search(TwitterWebClient *twitterwebclient, const gchar *word, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->search(twitterwebclient, word, buffer, length);
}

gboolean
twitter_web_client_get_followers(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict cursor, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_followers(twitterwebclient, username, cursor, buffer, length);
}

gboolean
twitter_web_client_get_friends(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict cursor, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_friends(twitterwebclient, username, cursor, buffer, length);
}

gboolean
twitter_web_client_add_friend(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->add_friend(twitterwebclient, friend, buffer, length);
}

gboolean
twitter_web_client_remove_friend(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->remove_friend(twitterwebclient, friend, buffer, length);
}

gboolean
twitter_web_client_block_user(TwitterWebClient *twitterwebclient, const gchar *user, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->block_user(twitterwebclient, user, buffer, length);
}

gboolean
twitter_web_client_get_user_details(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_user_details(twitterwebclient, username, buffer, length);
}

gboolean
twitter_web_client_get_user_details_by_id(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->get_user_details_by_id(twitterwebclient, id, buffer, length);
}

gboolean
twitter_web_client_post_tweet(TwitterWebClient *twitterwebclient, const gchar *text, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->post_tweet(twitterwebclient, text, buffer, length);
}

gboolean
twitter_web_client_remove_tweet(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->remove_tweet(twitterwebclient, id, buffer, length);
}

gboolean
twitter_web_client_retweet(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length)
{
	return TWITTER_WEB_CLIENT_GET_CLASS(twitterwebclient)->retweet(twitterwebclient, id, buffer, length);
}

TwitterWebClient *
twitter_web_client_new(void)
{
	TwitterWebClient *twitterwebclient;

	twitterwebclient = (TwitterWebClient *)g_object_new(TWITTER_WEB_CLIENT_TYPE, NULL);

	return twitterwebclient;
}

/*
 *	initialization/finalization:
 */
static void
_twitter_web_client_finalize(GObject *object)
{
	TwitterWebClient *twitterwebclient = TWITTER_WEB_CLIENT(object);

	if(twitterwebclient->priv->username)
	{
		g_free(twitterwebclient->priv->username);
	}

	if(twitterwebclient->priv->consumer_key)
	{
		g_free(twitterwebclient->priv->consumer_key);
	}

	if(twitterwebclient->priv->consumer_secret)
	{
		g_free(twitterwebclient->priv->consumer_secret);
	}

	if(twitterwebclient->priv->access_key)
	{
		g_free(twitterwebclient->priv->access_key);
	}

	if(twitterwebclient->priv->access_secret)
	{
		g_free(twitterwebclient->priv->access_secret);
	}

	if(twitterwebclient->priv->format)
	{
		g_free(twitterwebclient->priv->format);
	}

	if(twitterwebclient->priv->err)
	{
		g_error_free(twitterwebclient->priv->err);
	}

	if(G_OBJECT_CLASS(twitter_web_client_parent_class)->finalize)
	{
		(*G_OBJECT_CLASS(twitter_web_client_parent_class)->finalize)(object);
	}
}

static void
twitter_web_client_class_init(TwitterWebClientClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(TwitterWebClientClass));

	gobject_class->finalize = _twitter_web_client_finalize;
	gobject_class->get_property = _twitter_web_client_get_property;
	gobject_class->set_property = _twitter_web_client_set_property;

	klass->get_last_error = _twitter_web_client_get_last_error;
	klass->set_username = _twitter_web_client_set_username;
	klass->get_username = _twitter_web_client_get_username;
	klass->set_oauth_authorization = _twitter_web_client_set_oauth_authorization;
	klass->set_format = _twitter_web_client_set_format;
	klass->get_home_timeline = _twitter_web_client_get_home_timeline;
	klass->get_mentions = _twitter_web_client_get_mentions;
	klass->get_direct_messages = _twitter_web_client_get_direct_messages;
	klass->get_user_timeline = _twitter_web_client_get_user_timeline;
	klass->get_timeline_from_list = _twitter_web_client_get_timeline_from_list;
	klass->get_lists = _twitter_web_client_get_lists;
	klass->get_users_from_list = _twitter_web_client_get_users_from_list;
	klass->add_list = _twitter_web_client_add_list;
	klass->remove_list = _twitter_web_client_remove_list;
	klass->update_list = _twitter_web_client_update_list;
	klass->add_user_to_list = _twitter_web_client_add_user_to_list;
	klass->remove_user_from_list = _twitter_web_client_remove_user_from_list;
	klass->search = _twitter_web_client_search;
	klass->get_followers = _twitter_web_client_get_followers;
	klass->get_friends = _twitter_web_client_get_friends;
	klass->add_friend = _twitter_web_client_add_friend;
	klass->remove_friend = _twitter_web_client_remove_friend;
	klass->block_user = _twitter_web_client_block_user;
	klass->get_user_details = _twitter_web_client_get_user_details;
	klass->get_user_details_by_id = _twitter_web_client_get_user_details_by_id;
	klass->post_tweet = _twitter_web_client_post_tweet;
	klass->remove_tweet = _twitter_web_client_remove_tweet;
	klass->retweet = _twitter_web_client_retweet;

	g_object_class_install_property(gobject_class, PROP_USERNAME,
	                                g_param_spec_string("username", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_OAUTH_CONSUMER_KEY,
	                                g_param_spec_string("oauth-consumer-key", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_OAUTH_CONSUMER_SECRET,
	                                g_param_spec_string("oauth-consumer-secret", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_OAUTH_ACCESS_KEY,
	                                g_param_spec_string("oauth-access-key", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_OAUTH_ACCESS_SECRET,
	                                g_param_spec_string("oauth-access-secret", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_FORMAT,
	                                g_param_spec_string("format", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_STATUS_COUNT,
	                                g_param_spec_int("status-count", NULL, NULL, 20, 200, 20, G_PARAM_READWRITE));
}

static void
twitter_web_client_init(TwitterWebClient *twitterwebclient)
{
	/* register private data */
	twitterwebclient->priv = G_TYPE_INSTANCE_GET_PRIVATE(twitterwebclient, TWITTER_WEB_CLIENT_TYPE, TwitterWebClientPrivate);
	twitterwebclient->priv->status_count = 20;
}

/**
 * @}
 * @}
 */


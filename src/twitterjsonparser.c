/***************************************************************************
    begin........: December 2011
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
 * \file twitterjsonparser.c
 * \brief Parsing JSON data from Twitter.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 25. December 2011
 */
#include <string.h>
#include <glib/gprintf.h>

#include "twitterjsonparser.h"
#include "yail/yajl_parse.h"
#include "yail/yajl_gen.h"

/*
 *	parse search results:
 */

/*! Clear current key name. */
#define _twitter_json_search_result_clear_key(ctx) ((_twitter_json_search_result_data *)ctx)->key[0] = '\0'
/*! Test if cancellable has been cancelled and return from function if this is the case. */
#define _twitter_json_search_result_test_cancel(ctx) if(((_twitter_json_search_result_data *)ctx)->cancellable && g_cancellable_is_cancelled(((_twitter_json_search_result_data *)ctx)->cancellable)) return 0;

/**
 * \struct _twitter_json_search_result_data
 * \brief This structure holds data passed to the JSON parser functions.
 */
typedef struct
{
	/*! Current depth in the tree. */
	gint depth;
	/*! Current key. */
	gchar key[32];
	/*! Found user. */
	TwitterUser user;
	/*! Found status. */
	TwitterStatus status;
	/*! A cancellable. */
	GCancellable *cancellable;
	/*! Callback function. */
	TwitterProcessStatusFunc func;
	/*! User data. */
	gpointer user_data;
} _twitter_json_search_result_data;

static int
_twitter_json_search_result_map_open(void *ctx)
{
	_twitter_json_search_result_data *data = (_twitter_json_search_result_data *)ctx;

	_twitter_json_search_result_test_cancel(ctx);

	if(data->depth == 1)
	{
		memset(&data->user, 0, sizeof(TwitterUser));
		memset(&data->status, 0, sizeof(TwitterStatus));
	}

	++data->depth;

	return 1;
}

static int
_twitter_json_search_result_map_close(void *ctx)
{
	_twitter_json_search_result_data *data = (_twitter_json_search_result_data *)ctx;

	_twitter_json_search_result_test_cancel(ctx);

	if(data->depth == 2)
	{
		data->func(data->status, data->user, data->user_data);
	}

	--data->depth;

	return 1;
}

static int
_twitter_json_search_result_map_key(void *ctx, const unsigned char *value, size_t length)
{
	_twitter_json_search_result_data *data = (_twitter_json_search_result_data *)ctx;

	_twitter_json_search_result_test_cancel(ctx);

	if(length > 31)
	{
		length = 31;
	}

	memcpy(data->key, value, length);
	data->key[length] = '\0';

	return 1;
}

static int
_twitter_json_search_result_handle_null(void *ctx)
{
	_twitter_json_search_result_test_cancel(ctx);
	_twitter_json_search_result_clear_key(ctx);

	return 1;
}

static int
_twitter_json_search_result_handle_string(void *ctx, const unsigned char *value, size_t length)
{
	_twitter_json_search_result_data *data = (_twitter_json_search_result_data *)ctx;
	gchar *text = (gchar *)g_alloca(length + 1);
	gchar **parts;

	_twitter_json_search_result_test_cancel(ctx);

	memcpy(text, value, length);
	text[length] = '\0';

	if(data->depth == 2)
	{
		if(!g_strcmp0("id_str", data->key))
		{
			g_strlcpy(data->status.id, text, 32);
		}
		else if(!g_strcmp0("created_at", data->key))
		{
			if((parts = g_strsplit(text, " ", 6)))
			{
				parts[0][strlen(parts[0]) - 1] = '\0';
				g_snprintf(data->status.created_at, 24, "%s %s %s %s %s %s", parts[0], parts[2], parts[1], parts[4], parts[5], parts[3]);		
				g_strfreev(parts);
			}
		}
		else if(!g_ascii_strcasecmp("text", data->key))
		{
			g_strlcpy(data->status.text, text, 280);
		}
		else if(!g_ascii_strcasecmp("from_user", data->key))
		{
			g_strlcpy(data->user.name, text, 64);
		}
		else if(!g_ascii_strcasecmp("from_user_id_str", data->key))
		{
			g_strlcpy(data->user.id, text, 32);
		}
		else if(!g_ascii_strcasecmp("from_user_name", data->key))
		{
			g_strlcpy(data->user.screen_name, text, 64);
		}
		else if(!g_ascii_strcasecmp("profile_image_url", data->key))
		{
			g_strlcpy(data->user.image, text, 256);
		}
	}

	_twitter_json_search_result_clear_key(ctx);

	return 1;
}

static yajl_callbacks _twitter_json_search_result_funcs =
{
	_twitter_json_search_result_handle_null,
	NULL,
	NULL,
	NULL,
	NULL,
	_twitter_json_search_result_handle_string,
	_twitter_json_search_result_map_open,
	_twitter_json_search_result_map_key,
	_twitter_json_search_result_map_close,
	NULL,
	NULL
};

void
twitter_json_parse_search_result(const gchar *json, gint length, TwitterProcessStatusFunc parser_func, gpointer user_data, GCancellable *cancellable)
{
	yajl_handle handle;
	_twitter_json_search_result_data data;

	g_assert(parser_func != NULL);

	memset(&data, 0, sizeof(_twitter_json_search_result_data));
	data.func = parser_func;
	data.user_data = user_data;

	handle = yajl_alloc(&_twitter_json_search_result_funcs, NULL, (void *)&data);
	yajl_parse(handle, (const unsigned char *)json, length);
	yajl_free(handle);
}


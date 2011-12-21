/***************************************************************************
    begin........: March 2010
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
 * \file twitterxmlparser.c
 * \brief Parsing XML data from Twitter.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 21. June 2011
 */

#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>

#include "twitter.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/*
 *	helpers:
 */

/* parse user sections */
static void
_twitter_xml_process_user_section(TwitterUser *user, const gchar *element_name, GString *buffer)
{
	if(!g_ascii_strcasecmp(element_name, "id"))
	{
		g_strlcpy(user->id, buffer->str, 32);
	}
	else if(!g_ascii_strcasecmp(element_name, "name"))
	{
		g_strlcpy(user->name, buffer->str, 64);
	}
	else if(!g_ascii_strcasecmp(element_name, "screen_name"))
	{
		g_strlcpy(user->screen_name, buffer->str, 64);
	}
	else if(!g_ascii_strcasecmp(element_name, "description"))
	{
		g_strlcpy(user->description, buffer->str, 280);
	}
	else if(!g_ascii_strcasecmp(element_name, "profile_image_url"))
	{
		g_strlcpy(user->image, buffer->str, 256);
	}
	else if(!g_ascii_strcasecmp(element_name, "url"))
	{
		g_strlcpy(user->url, buffer->str, 256);
	}
	else if(!g_ascii_strcasecmp(element_name, "following"))
	{
		if(!g_ascii_strcasecmp(buffer->str, "true"))
		{
			user->following = TRUE;
		}
		else
		{
			user->following = FALSE;
		}
	}
	else if(!g_ascii_strcasecmp(element_name, "location"))
	{
		g_strlcpy(user->location, buffer->str, 64);
	}
}

/*
 *	Twitter timeline parsing:
 */

/*!
 * \struct _TwitterXMLTimelineParserData
 * \brief This structure holds data passed to the GMarkupParser functions.
 */
typedef struct
{
	/*! Current depth in the XML tree. */
	gint depth;
	/*! Buffer to store characters. */
	GString *buffer;
	/*! Holds the found status information. */
	TwitterStatus status;
	/*! Holds the found user information. */
	TwitterUser user;
	/*! Callback invoked when a status is found. */
	TwitterProcessStatusFunc parser_func;
	/*! User data. */
	gpointer user_data;
} _TwitterXMLTimelineParserData;

static void
_twitter_xml_timeline_data_init(_TwitterXMLTimelineParserData *data, TwitterProcessStatusFunc parser_func, gpointer user_data)
{
	memset(data, 0, sizeof(_TwitterXMLTimelineParserData));
	data->parser_func = parser_func;
	data->user_data = user_data;
}

static void
_twitter_xml_timeline_data_free(_TwitterXMLTimelineParserData data)
{
	if(data.buffer)
	{
		g_string_free(data.buffer, TRUE);
	}
}

/* Handle status sections */
static void
_twitter_xml_process_status_section(TwitterStatus *status, const gchar *element_name, GString *buffer)
{
	if(!g_ascii_strcasecmp(element_name, "created_at"))
	{
		g_strlcpy(status->created_at, buffer->str, 32);
	}
	else if(!g_ascii_strcasecmp(element_name, "id"))
	{
		g_strlcpy(status->id, buffer->str, 32);
	}
	else if(!g_ascii_strcasecmp(element_name, "text"))
	{
		g_strlcpy(status->text, buffer->str, 280);
	}
}

/* GMarkupParser callbacks */
static void
_twitter_xml_timeline_parser_start_element(GMarkupParseContext *context,
                                           const gchar *element_name,
                                           const gchar **attribute_names,
                                           const gchar **attribute_values,
                                           gpointer user_data,
                                           GError **error)
{
	_TwitterXMLTimelineParserData *data = (_TwitterXMLTimelineParserData*)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 3)
	{	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_timeline_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLTimelineParserData *data = (_TwitterXMLTimelineParserData *)user_data;

	if(data->depth == 2 && !g_ascii_strcasecmp(element_name, "status"))
	{
		data->parser_func(data->status, data->user, data->user_data);
	}
	else if(data->depth == 3)
	{
		_twitter_xml_process_status_section(&data->status, element_name, data->buffer);
	}
	else if(data->depth == 4)
	{
		_twitter_xml_process_user_section(&data->user, element_name, data->buffer);
	}

	--data->depth;
}

static void
_twitter_xml_timeline_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLTimelineParserData *data = (_TwitterXMLTimelineParserData *)user_data;

	if(data->depth >= 3)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_timeline_parser =
{
	&_twitter_xml_timeline_parser_start_element,
	&_twitter_xml_timeline_parser_end_element,
	&_twitter_xml_timeline_parser_text,
	NULL,
	NULL
};

void
twitter_xml_parse_timeline(const gchar *xml, gint length, TwitterProcessStatusFunc parser_func, gpointer user_data, GCancellable *cancellable)
{
	GMarkupParseContext *ctx;
	_TwitterXMLTimelineParserData data;
	gint offset = 0;
	gint size = 128;

	g_return_if_fail(parser_func != NULL);

	_twitter_xml_timeline_data_init(&data, parser_func, user_data);
	ctx = g_markup_parse_context_new(&_twitter_xml_timeline_parser, 0, (gpointer)&data, NULL);

	while((offset < length) && (!cancellable || !g_cancellable_is_cancelled(cancellable)))
	{
		if(offset + size > length)
		{
			size = length - offset;
		}
	
		g_markup_parse_context_parse(ctx, xml + offset, size, NULL);
		offset += size;
	}

	g_markup_parse_context_free(ctx);
	_twitter_xml_timeline_data_free(data);
}

/*
 *	Twitter list parsing:
 */

/*!
 * \struct _TwitterXMLListParserData
 * \brief This structure holds data passed to the GMarkupParser functions.
 */
typedef struct
{
	/*! Current depth in the XML tree. */
	gint depth;
	/*! Buffer to store characters. */
	GString *buffer;
	/*! Holds the found list information. */
	TwitterList list;
	/*! Holds the found user information. */
	TwitterUser user;
	/*! Callback invoked when a list is found. */
	TwitterProcessListFunc parser_func;
	/*! User data. */
	gpointer user_data;
} _TwitterXMLListParserData;

static void
_twitter_xml_list_data_init(_TwitterXMLListParserData *data, TwitterProcessListFunc parser_func, gpointer user_data)
{
	memset(data, 0, sizeof(_TwitterXMLListParserData));
	data->parser_func = parser_func;
	data->user_data = user_data;
}

static void
_twitter_xml_list_data_free(_TwitterXMLListParserData data)
{
	if(data.buffer)
	{
		g_string_free(data.buffer, TRUE);
	}
}

/* Handles list sections */
static void
_twitter_xml_process_list_section(TwitterList *list, const gchar *element_name, GString *buffer)
{
	if(!g_ascii_strcasecmp(element_name, "id"))
	{
		g_strlcpy(list->id, buffer->str, 32);
	}
	else if(!g_ascii_strcasecmp(element_name, "name"))
	{
		g_strlcpy(list->name, buffer->str, 64);
	}
	else if(!g_ascii_strcasecmp(element_name, "full_name"))
	{
		g_strlcpy(list->fullname, buffer->str, 64);
	}
	else if(!g_ascii_strcasecmp(element_name, "description"))
	{
		g_strlcpy(list->description, buffer->str, 280);
	}
	else if(!g_ascii_strcasecmp(element_name, "uri"))
	{
		g_strlcpy(list->uri, buffer->str, 256);
	}
	else if(!g_ascii_strcasecmp(element_name, "mode"))
	{
		if(!g_ascii_strcasecmp(buffer->str, "public"))
		{
			list->protected = FALSE;
		}
		else
		{
			list->protected = TRUE;
		}
	}
	else if(!g_ascii_strcasecmp(element_name, "following"))
	{
		if(!g_ascii_strcasecmp(buffer->str, "true"))
		{
			list->following = TRUE;
		}
		else
		{
			list->following = FALSE;
		}
	}
	else if(!g_ascii_strcasecmp(element_name, "subscriber_count"))
	{
		list->subscriber_count = atoi(buffer->str);
	}
	else if(!g_ascii_strcasecmp(element_name, "member_count"))
	{
		list->member_count = atoi(buffer->str);
	}
}

/* GMarkupParser callbacks (multiple lists) */
static void
_twitter_xml_lists_parser_start_element(GMarkupParseContext *context,
                                        const gchar *element_name,
                                        const gchar **attribute_names,
                                        const gchar **attribute_values,
                                        gpointer user_data,
                                        GError **error)
{
	_TwitterXMLListParserData *data = (_TwitterXMLListParserData*)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 3)
	{	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_lists_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLListParserData *data = (_TwitterXMLListParserData *)user_data;

	if(data->depth == 3 && !g_ascii_strcasecmp(element_name, "list"))
	{
		data->parser_func(data->list, data->user, data->user_data);
	}
	else if(data->depth == 4)
	{
		_twitter_xml_process_list_section(&data->list, element_name, data->buffer);
	}
	else if(data->depth == 5)
	{
		_twitter_xml_process_user_section(&data->user, element_name, data->buffer);
	}

	--data->depth;
}

static void
_twitter_xml_lists_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLListParserData *data = (_TwitterXMLListParserData *)user_data;

	if(data->depth >= 4)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_lists_parser =
{
	&_twitter_xml_lists_parser_start_element,
	&_twitter_xml_lists_parser_end_element,
	&_twitter_xml_lists_parser_text,
	NULL,
	NULL
};

/* GMarkupParser callbacks (single list) */
static void
_twitter_xml_list_parser_start_element(GMarkupParseContext *context,
                                       const gchar *element_name,
                                       const gchar **attribute_names,
                                       const gchar **attribute_values,
                                       gpointer user_data,
                                       GError **error)
{
	_TwitterXMLListParserData *data = (_TwitterXMLListParserData*)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 2)
	{	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_list_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLListParserData *data = (_TwitterXMLListParserData *)user_data;

	if(data->depth == 2)
	{
		_twitter_xml_process_list_section(&data->list, element_name, data->buffer);
	}
	else if(data->depth == 3)
	{
		_twitter_xml_process_user_section(&data->user, element_name, data->buffer);
	}

	--data->depth;
}

static void
_twitter_xml_list_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLListParserData *data = (_TwitterXMLListParserData *)user_data;

	if(data->depth >= 2)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_list_parser =
{
	&_twitter_xml_list_parser_start_element,
	&_twitter_xml_list_parser_end_element,
	&_twitter_xml_list_parser_text,
	NULL,
	NULL
};

void
twitter_xml_parse_lists(const gchar *xml, gint length, TwitterProcessListFunc parser_func, gpointer user_data)
{
	GMarkupParseContext *ctx;
	_TwitterXMLListParserData data;

	g_return_if_fail(parser_func != NULL);
	
	_twitter_xml_list_data_init(&data, parser_func, user_data);
	ctx = g_markup_parse_context_new(&_twitter_xml_lists_parser, 0, (gpointer)&data, NULL);
	g_markup_parse_context_parse(ctx, xml, length, NULL);
	g_markup_parse_context_free(ctx);
	_twitter_xml_list_data_free(data);
}

gboolean
twitter_xml_parse_list(const gchar *xml, gint length, TwitterUser *user, TwitterList *list)
{
	GMarkupParseContext *ctx;
	_TwitterXMLListParserData data;
	gboolean result = FALSE;

	_twitter_xml_list_data_init(&data, NULL, NULL);
	ctx = g_markup_parse_context_new(&_twitter_xml_list_parser, 0, (gpointer)&data, NULL);
	g_markup_parse_context_parse(ctx, xml, length, NULL);
	g_markup_parse_context_free(ctx);

	if(data.list.name[0] && data.user.screen_name[0])
	{
		memcpy(user, &data.user, sizeof(TwitterUser));
		memcpy(list, &data.list, sizeof(TwitterList));
		result = TRUE;
	}

	_twitter_xml_list_data_free(data);

	return result;
}

/*
 *	Twitter list member parsing:
 */

/*!
 * \struct _TwitterXMLListMembersParserData
 * \brief This structure holds data passed to the GMarkupParser functions.
 */
typedef struct
{
	/*! Current depth in the XML tree. */
	gint depth;
	/*! Buffer to store characters. */
	GString *buffer;
	/*! Holds the found user information. */
	TwitterUser user;
	/*! Callback invoked when a list member is found. */
	TwitterProcessListMemberFunc parser_func;
	/*! User data. */
	gpointer user_data;
	/*! Next cursor. */
	gchar next_cursor[64];
} _TwitterXMLListMembersParserData;

static void
_twitter_xml_list_members_data_init(_TwitterXMLListMembersParserData *data, TwitterProcessListMemberFunc parser_func, gpointer user_data)
{
	memset(data, 0, sizeof(_TwitterXMLListMembersParserData));
	data->parser_func = parser_func;
	data->user_data = user_data;
}

static void
_twitter_xml_list_members_data_free(_TwitterXMLListMembersParserData data)
{
	if(data.buffer)
	{
		g_string_free(data.buffer, TRUE);
	}
}

/* GMarkupParser callbacks */
static void
_twitter_xml_list_members_parser_start_element(GMarkupParseContext *context,
                                               const gchar *element_name,
                                               const gchar **attribute_names,
                                               const gchar **attribute_values,
                                               gpointer user_data,
                                               GError **error)
{
	_TwitterXMLListMembersParserData *data = (_TwitterXMLListMembersParserData*)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 2)
	{	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_list_members_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLListMembersParserData *data = (_TwitterXMLListMembersParserData *)user_data;

	if(data->depth == 2 && !g_ascii_strcasecmp(element_name, "next_cursor"))
	{
		g_strlcpy(data->next_cursor, data->buffer->str, 64);
	}
	if(data->depth == 3 && !g_ascii_strcasecmp(element_name, "user"))
	{
		data->parser_func(data->user, data->user_data);
	}
	else if(data->depth == 4)
	{
		_twitter_xml_process_user_section(&data->user, element_name, data->buffer);
	}

	--data->depth;
}

static void
_twitter_xml_list_members_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLListParserData *data = (_TwitterXMLListParserData *)user_data;

	if(data->depth >= 2)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_list_members_parser =
{
	&_twitter_xml_list_members_parser_start_element,
	&_twitter_xml_list_members_parser_end_element,
	&_twitter_xml_list_members_parser_text,
	NULL,
	NULL
};

void
twitter_xml_parse_list_members(const gchar *xml, gint length, TwitterProcessListMemberFunc parser_func, gchar next_cursor[], gint cursor_size, gpointer user_data)
{
	GMarkupParseContext *ctx;
	_TwitterXMLListMembersParserData data;

	g_return_if_fail(parser_func != NULL);
	g_assert(cursor_size <= 64);

	_twitter_xml_list_members_data_init(&data, parser_func, user_data);
	ctx = g_markup_parse_context_new(&_twitter_xml_list_members_parser, 0, (gpointer)&data, NULL);
	g_markup_parse_context_parse(ctx, xml, length, NULL);
	g_markup_parse_context_free(ctx);
	g_strlcpy(next_cursor, data.next_cursor, cursor_size);
	_twitter_xml_list_members_data_free(data);
}

/*
 *	Twitter user details parsing:
 */

/*!
 * \struct _TwitterXMLUserParserData
 * \brief This structure holds data passed to the GMarkupParser functions.
 */
typedef struct
{
	/*! Current depth in the XML tree. */
	gint depth;
	/*! Buffer to store characters. */
	GString *buffer;
	/*! Holds the found user information. */
	TwitterUser user;
	/*! Callback invoked when a user is found. */
	TwitterProcessUserFunc parser_func;
	/*! User data. */
	gpointer user_data;
} _TwitterXMLUserParserData;

static void
_twitter_xml_user_data_init(_TwitterXMLUserParserData *data, TwitterProcessUserFunc parser_func, gpointer user_data)
{
	memset(data, 0, sizeof(_TwitterXMLUserParserData));
	data->parser_func = parser_func;
	data->user_data = user_data;
}

static void
_twitter_xml_user_data_free(_TwitterXMLUserParserData data)
{
	if(data.buffer)
	{
		g_string_free(data.buffer, TRUE);
	}
}

/* GMarkupParser callbacks */
static void
_twitter_xml_user_parser_start_element(GMarkupParseContext *context,
                                       const gchar *element_name,
                                       const gchar **attribute_names,
                                       const gchar **attribute_values,
                                       gpointer user_data,
                                       GError **error)
{
	_TwitterXMLUserParserData *data = (_TwitterXMLUserParserData *)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 1)
	{	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_user_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLUserParserData *data = (_TwitterXMLUserParserData *)user_data;

	if(data->depth == 1 && !g_ascii_strcasecmp(element_name, "user"))
	{
		data->parser_func(data->user, data->user_data);
	}
	else if(data->depth == 2)
	{
		_twitter_xml_process_user_section(&data->user, element_name, data->buffer);
	}

	--data->depth;
}

static void
_twitter_xml_user_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLUserParserData *data = (_TwitterXMLUserParserData *)user_data;

	if(data->depth >= 1)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_user_parser =
{
	&_twitter_xml_user_parser_start_element,
	&_twitter_xml_user_parser_end_element,
	&_twitter_xml_user_parser_text,
	NULL,
	NULL
};

void
twitter_xml_parse_user_details(const gchar *xml, gint length, TwitterProcessUserFunc parser_func, gpointer user_data)
{
	GMarkupParseContext *ctx;
	_TwitterXMLUserParserData data;

	g_return_if_fail(parser_func != NULL);

	_twitter_xml_user_data_init(&data, parser_func, user_data);
	ctx = g_markup_parse_context_new(&_twitter_xml_user_parser, 0, (gpointer)&data, NULL);
	g_markup_parse_context_parse(ctx, xml, length, NULL);
	g_markup_parse_context_free(ctx);
	_twitter_xml_user_data_free(data);
}

/*
 *	Twitter direct message parsing:
 */

/*!
 * \struct _TwitterXMLDirectMessageParserData
 * \brief This structure holds data for parsing direct messages.
 */
typedef struct
{
	/*! Current depth in the XML tree. */
	gint depth;
	/*! Buffer to store characters. */
	GString *buffer;
	/*! Holds the direct message information. */
	TwitterDirectMessage message;
	/*! Holds the found sender information. */
	TwitterUser sender;
	/*! Holds the found receiver information. */
	TwitterUser receiver;
	/*! Callback invoked when a direct message is found. */
	TwitterProcessDirectMessageFunc parser_func;
	/*! Indicates if the sender or the receiver should be parsed. */
	gboolean parse_sender;
	/*! User data. */
	gpointer user_data;
} _TwitterXMLDirectMessageParserData;

static void
_twitter_xml_direct_message_data_init(_TwitterXMLDirectMessageParserData *data, TwitterProcessDirectMessageFunc parser_func, gpointer user_data)
{
	memset(data, 0, sizeof(_TwitterXMLDirectMessageParserData));
	data->parser_func = parser_func;
	data->user_data = user_data;
}

static void
_twitter_xml_direct_message_data_free(_TwitterXMLDirectMessageParserData data)
{
	if(data.buffer)
	{
		g_string_free(data.buffer, TRUE);
	}
}

/* Handles direct message sections */
static void
_twitter_xml_process_direct_message_section(TwitterDirectMessage *message, const gchar *element_name, GString *buffer)
{
	if(!g_ascii_strcasecmp(element_name, "id"))
	{
		g_strlcpy(message->id, buffer->str, 32);
	}
	else if(!g_ascii_strcasecmp(element_name, "text"))
	{
		g_strlcpy(message->text, buffer->str, 280);
	}
	else if(!g_ascii_strcasecmp(element_name, "created_at"))
	{
		g_strlcpy(message->created_at, buffer->str, 30);
	}
}

/* GMarkupParser callbacks */
static void
_twitter_xml_direct_message_parser_start_element(GMarkupParseContext *context,
                                                 const gchar *element_name,
                                                 const gchar **attribute_names,
                                                 const gchar **attribute_values,
                                                 gpointer user_data,
                                                 GError **error)
{
	_TwitterXMLDirectMessageParserData *data = (_TwitterXMLDirectMessageParserData *)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 2)
	{
		if(data->depth == 3)
		{
			if(!g_ascii_strcasecmp(element_name, "sender"))
			{
				data->parse_sender = TRUE;
			}
			else if(!g_ascii_strcasecmp(element_name, "recipient"))
			{
				data->parse_sender = FALSE;
			}
		}
	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_direct_message_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLDirectMessageParserData *data = (_TwitterXMLDirectMessageParserData *)user_data;

	if(data->depth == 2 && !g_ascii_strcasecmp(element_name, "direct_message"))
	{
		data->parser_func(data->message, data->sender, data->receiver, data->user_data);
	}
	else if(data->depth == 3)
	{
		_twitter_xml_process_direct_message_section(&data->message, element_name, data->buffer);
	}
	else if(data->depth == 4)
	{
		if(data->parse_sender)
		{
			_twitter_xml_process_user_section(&data->sender, element_name, data->buffer);
		}
		else
		{
			_twitter_xml_process_user_section(&data->receiver, element_name, data->buffer);
		}
	}

	--data->depth;
}

static void
_twitter_xml_direct_message_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLDirectMessageParserData *data = (_TwitterXMLDirectMessageParserData *)user_data;

	if(data->depth >= 2)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_direct_message_parser =
{
	&_twitter_xml_direct_message_parser_start_element,
	&_twitter_xml_direct_message_parser_end_element,
	&_twitter_xml_direct_message_parser_text,
	NULL,
	NULL
};

void
twitter_xml_parse_direct_messages ( const gchar* xml, gint length, TwitterProcessDirectMessageFunc parser_func, gpointer user_data)
{
	GMarkupParseContext *ctx;
	_TwitterXMLDirectMessageParserData data;

	g_return_if_fail(parser_func != NULL);

	_twitter_xml_direct_message_data_init(&data, parser_func, user_data);
	ctx = g_markup_parse_context_new(&_twitter_xml_direct_message_parser, 0, (gpointer)&data, NULL);
	g_markup_parse_context_parse(ctx, xml, length, NULL);
	g_markup_parse_context_free(ctx);
	_twitter_xml_direct_message_data_free(data);
}

/*
 *	Twitter friendship parsing:
 */

/*!
 * \struct _TwitterXMLFriendshipData
 * \brief This structure holds data for parsing friendship information.
 */
typedef struct
{
	/*! Current depth in the XML tree. */
	gint depth;
	/*! Buffer to store characters. */
	GString *buffer;
	/*! Holds the friendship information. */
	TwitterFriendship friendship;
	/*! Indicates if the source or the target should be parsed. */
	gboolean parse_source;
} _TwitterXMLFriendshipData;

static void
_twitter_xml_friendship_data_init(_TwitterXMLFriendshipData *data)
{
	memset(data, 0, sizeof(_TwitterXMLFriendshipData));
}

static void
_twitter_xml_friendship_data_free(_TwitterXMLFriendshipData data)
{
	if(data.buffer)
	{
		g_string_free(data.buffer, TRUE);
	}
}

/* Handles friendship sections */
static void
_twitter_xml_process_friendship_section(TwitterFriendship *friendship, const gchar *element_name, gboolean parse_source, GString *buffer)
{
	if(!g_ascii_strcasecmp(element_name, "id_str"))
	{
		if(parse_source)
		{
			g_strlcpy(friendship->source_guid, buffer->str, 32);
		}
		else
		{
			g_strlcpy(friendship->target_guid, buffer->str, 32);
		}
	}
	else if(!g_ascii_strcasecmp(element_name, "screen_name"))
	{
		if(parse_source)
		{
			g_strlcpy(friendship->source_screen_name, buffer->str, 64);
		}
		else
		{
			g_strlcpy(friendship->target_screen_name, buffer->str, 64);
		}
	}
	else if(!g_ascii_strcasecmp(element_name, "following"))
	{
		if(parse_source)
		{
			friendship->source_following = (!g_ascii_strcasecmp(buffer->str, "true")) ? TRUE : FALSE;
		}
		else
		{
			friendship->target_following = (!g_ascii_strcasecmp(buffer->str, "true")) ? TRUE : FALSE;
		}
	}
	else if(!g_ascii_strcasecmp(element_name, "followed_by"))
	{
		if(parse_source)
		{
			friendship->source_followed_by = (!g_ascii_strcasecmp(buffer->str, "true")) ? TRUE : FALSE;
		}
		else
		{
			friendship->target_followed_by = (!g_ascii_strcasecmp(buffer->str, "true")) ? TRUE : FALSE;
		}
	}
}

/* GMarkupParser callbacks */
static void
_twitter_xml_friendship_parser_start_element(GMarkupParseContext *context,
                                             const gchar *element_name,
                                             const gchar **attribute_names,
                                             const gchar **attribute_values,
                                             gpointer user_data,
                                             GError **error)
{
	_TwitterXMLFriendshipData *data = (_TwitterXMLFriendshipData *)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 1)
	{
		if(data->depth == 2)
		{
			if(!g_ascii_strcasecmp(element_name, "target"))
			{
				data->parse_source = FALSE;
			}
			else if(!g_ascii_strcasecmp(element_name, "source"))
			{
				data->parse_source = TRUE;
			}
		}
	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_friendship_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLFriendshipData *data = (_TwitterXMLFriendshipData *)user_data;

	if(data->depth == 3)
	{
		_twitter_xml_process_friendship_section(&data->friendship, element_name, data->parse_source, data->buffer);
	}

	--data->depth;
}

static void
_twitter_xml_friendship_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLFriendshipData *data = (_TwitterXMLFriendshipData *)user_data;

	if(data->depth >= 2)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_friendship_parser =
{
	&_twitter_xml_friendship_parser_start_element,
	&_twitter_xml_friendship_parser_end_element,
	&_twitter_xml_friendship_parser_text,
	NULL,
	NULL
};

gboolean
twitter_xml_parse_friendship(const gchar *xml, gint length, TwitterFriendship *friendship)
{
	GMarkupParseContext *ctx;
	_TwitterXMLFriendshipData data;
	gboolean result = FALSE;

	/* parse data */
	_twitter_xml_friendship_data_init(&data);
	ctx = g_markup_parse_context_new(&_twitter_xml_friendship_parser, 0, (gpointer)&data, NULL);
	g_markup_parse_context_parse(ctx, xml, length, NULL);
	g_markup_parse_context_free(ctx);
	_twitter_xml_friendship_data_free(data);

	/* test result */
	if(data.friendship.source_guid[0] && data.friendship.source_screen_name[0] && data.friendship.target_guid[0] && data.friendship.target_screen_name[0])
	{
		*friendship = data.friendship;
		result = TRUE;
	}

	return result;
}

/*
 *	Twitter user id  parsing:
 */

/*!
 * \struct _TwitterXMLIdParserData
 * \brief This structure holds data passed to the GMarkupParser functions.
 */
typedef struct
{
	/*! Current depth in the XML tree. */
	gint depth;
	/*! Buffer to store characters. */
	GString *buffer;
	/*! Callback invoked when a list member is found. */
	TwitterProcessIdFunc parser_func;
	/*! User data. */
	gpointer user_data;
	/*! Next cursor. */
	gchar next_cursor[64];
} _TwitterXMLIdParserData;

static void
_twitter_xml_id_data_init(_TwitterXMLIdParserData *data, TwitterProcessIdFunc parser_func, gpointer user_data)
{
	memset(data, 0, sizeof(_TwitterXMLIdParserData));
	data->parser_func = parser_func;
	data->user_data = user_data;
}

static void
_twitter_xml_id_data_free(_TwitterXMLIdParserData data)
{
	if(data.buffer)
	{
		g_string_free(data.buffer, TRUE);
	}
}

/* GMarkupParser callbacks */
static void
_twitter_xml_id_parser_start_element(GMarkupParseContext *context,
                                     const gchar *element_name,
                                     const gchar **attribute_names,
                                     const gchar **attribute_values,
                                     gpointer user_data,
                                     GError **error)
{
	_TwitterXMLIdParserData *data = (_TwitterXMLIdParserData *)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 2)
	{	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_id_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLIdParserData *data = (_TwitterXMLIdParserData *)user_data;

	if(data->depth == 3 && !g_ascii_strcasecmp(element_name, "id"))
	{
		data->parser_func(data->buffer->str, data->user_data);
	}
	else if(data->depth == 2 && !g_ascii_strcasecmp(element_name, "next_cursor"))
	{
		g_strlcpy(data->next_cursor, data->buffer->str, 64);
	}

	--data->depth;
}

static void
_twitter_xml_id_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLIdParserData *data = (_TwitterXMLIdParserData *)user_data;

	if(data->depth >= 2)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_id_parser =
{
	&_twitter_xml_id_parser_start_element,
	&_twitter_xml_id_parser_end_element,
	&_twitter_xml_id_parser_text,
	NULL,
	NULL
};

void
twitter_xml_parse_ids(const gchar *xml, gint length, TwitterProcessIdFunc parser_func, gchar next_cursor[], gint cursor_size, GCancellable *cancellable, gpointer user_data)
{
	GMarkupParseContext *ctx;
	_TwitterXMLIdParserData data;
	gint offset = 0;
	gint size = 64;

	g_return_if_fail(parser_func != NULL);
	g_assert(cursor_size <= 64);

	_twitter_xml_id_data_init(&data, parser_func, user_data);
	ctx = g_markup_parse_context_new(&_twitter_xml_id_parser, 0, (gpointer)&data, NULL);

	while((offset < length) && (!cancellable || !g_cancellable_is_cancelled(cancellable)))
	{
		if(offset + size > length)
		{
			size = length - offset;
		}
	
		g_markup_parse_context_parse(ctx, xml + offset, size, NULL);
		offset += size;
	}

	g_markup_parse_context_free(ctx);
	g_strlcpy(next_cursor, data.next_cursor, cursor_size);
	_twitter_xml_id_data_free(data);
}

/*
 *	single status parsing:
 */

/*!
 * \struct _TwitterXMLStatusParserData
 * \brief This structure holds data passed to the GMarkupParser functions.
 */
typedef struct
{
	/*! Current depth in the XML tree. */
	gint depth;
	/*! Buffer to store characters. */
	GString *buffer;
	/*! Holds the found status information. */
	TwitterStatus status;
	/*! Holds the found user information. */
	TwitterUser user;
} _TwitterXMLStatusParserData;

static void
_twitter_xml_status_data_init(_TwitterXMLStatusParserData *data)
{
	memset(data, 0, sizeof(_TwitterXMLStatusParserData));
}

static void
_twitter_xml_status_data_free(_TwitterXMLStatusParserData data)
{
	if(data.buffer)
	{
		g_string_free(data.buffer, TRUE);
	}
}

/* GMarkupParser callbacks */
static void
_twitter_xml_status_parser_start_element(GMarkupParseContext *context,
                                         const gchar *element_name,
                                         const gchar **attribute_names,
                                         const gchar **attribute_values,
                                         gpointer user_data,
                                         GError **error)
{
	_TwitterXMLStatusParserData *data = (_TwitterXMLStatusParserData *)user_data;

	++data->depth;

	/* reset buffer */
	if(data->depth >= 2)
	{	
		if(!data->buffer)
		{
			data->buffer = g_string_sized_new(280);
		}
		else
		{
			g_string_erase(data->buffer, 0, -1);
		}
	}
}

static void
_twitter_xml_status_parser_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	_TwitterXMLStatusParserData *data = (_TwitterXMLStatusParserData *)user_data;

	if(data->depth == 2)
	{
		_twitter_xml_process_status_section(&data->status, element_name, data->buffer);
	}
	else if(data->depth == 3)
	{
		_twitter_xml_process_user_section(&data->user, element_name, data->buffer);
	}

	--data->depth;
}

static void
_twitter_xml_status_parser_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_TwitterXMLStatusParserData *data = (_TwitterXMLStatusParserData *)user_data;

	if(data->depth >= 2)
	{
		g_string_append(data->buffer, text);
	}

}

static GMarkupParser _twitter_xml_status_parser =
{
	&_twitter_xml_status_parser_start_element,
	&_twitter_xml_status_parser_end_element,
	&_twitter_xml_status_parser_text,
	NULL,
	NULL
};

gboolean
twitter_xml_parse_status(const gchar *xml, gint length, TwitterStatus *status, TwitterUser *user)
{
	GMarkupParseContext *ctx;
	_TwitterXMLStatusParserData data;
	gboolean result = FALSE;

	/* parse data */
	_twitter_xml_status_data_init(&data);
	ctx = g_markup_parse_context_new(&_twitter_xml_status_parser, 0, (gpointer)&data, NULL);
	g_markup_parse_context_parse(ctx, xml, length, NULL);
	g_markup_parse_context_free(ctx);
	_twitter_xml_status_data_free(data);

	/* test result */
	if(data.status.id[0] && data.user.id[0])
	{
		*status = data.status;
		*user = data.user;
		result = TRUE;
	}

	return result;
}

/**
 * @}
 * @}
 */


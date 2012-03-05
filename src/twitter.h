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
* \file twitter.h
* \brief Twitter datatypes and functions.
* \author Sebastian Fedrau <lord-kefir@arcor.de>
* \version 0.1.0
* \date 11. January 2012
*/

#ifndef __TWITTER_H__
#define __TWITTER_H__

#include <glib.h>

#include "urlopener.h"

/**
* @addtogroup Core
* @{
* 	@addtogroup Twitter
* 	@{
*/

/*! Maximum number of tweets to fetch with one API call. */
#define TWITTER_MAX_STATUS_COUNT 200

/**
 * \struct TwitterUser
 * \brief Holds details of a twitter account.
 */
typedef struct
{
	/*! Unique identifier of the user. */
	gchar id[32];
	/*! Username. */
	gchar name[64];
	/*! Screen name. */
	gchar screen_name[64];
	/*! A brief description. */
	gchar description[280];
	/*! The user's website. */
	gchar url[256];
	/*! Url of the user's image. */
	gchar image[256];
	/*! Specifies if the logged on user is following the user. */
	gboolean following;
	/*! Location. */
	gchar location[64];
} TwitterUser;

/**
 * \struct TwitterStatus
 * \brief Holds a twitter status.
 */
typedef struct
{
	/*! Timestamp. */
	gchar created_at[32];
	/*! UNIX timestamp. */
	gint timestamp;
	/*! Unique identifier of the status. */
	gchar id[32];
	/*! The tweet. */
	gchar text[280];
	/*! The previous status.*/
	gchar prev_status[32];
} TwitterStatus;

/**
 * \struct TwitterList
 * \brief Holds information about a twitter list.
 */
typedef struct
{
	/*! Unique identifier of the list. */
	gchar id[32];
	/*! Name of the list. */
	gchar name[64];
	/*! Full name of the list. */
	gchar fullname[64];
	/*! Uri of the list. */
	gchar uri[256];
	/*! Protected. */
	gboolean protected;
	/*! A brief description. */
	gchar description[280];
	/*! Specifies if the logged on user is following the list. */
	gboolean following;
	/*! Number of subscribers. */
	gint subscriber_count;
	/*! Number of members. */
	gint member_count;
} TwitterList;

/**
 * \struct TwitterDirectMessage 
 * \brief Holds a direct message.
 */
typedef struct
{
	/*! Unique identifier of the message. */
	gchar id[32];
	/*! Text of the message. */
	gchar text[280];
	/*! Timestamp. */
	gchar created_at[32];
} TwitterDirectMessage;

/**
 * \struct TwitterFriendship
 * \brief Holds friendship information.
 */
typedef struct
{
	/*! Guid of the target. */
	gchar target_guid[32];
	/*! Screen name of the target. */
	gchar target_screen_name[64];
	/*! TRUE if target is followed by source. */
	gboolean target_followed_by;
	/*! TRUE if target is following source. */
	gboolean target_following;
	/*! Guid of the source. */
	gchar source_guid[32];
	/*! Screen name of the source. */
	gchar source_screen_name[64];
	/*! TRUE if source is followed by target. */
	gboolean source_followed_by;
	/*! TRUE if source is following target. */
	gboolean source_following;
} TwitterFriendship;

/*! Specifies the type of function which is called when a status is parsed. */
typedef void (* TwitterProcessStatusFunc)(TwitterStatus status, TwitterUser user, gpointer user_data);
/*! Specifies the type of function which is called when a list is parsed. */
typedef void (* TwitterProcessListFunc)(TwitterList list, TwitterUser user, gpointer user_data);
/*! Specifies the type of function which is called when a list member is parsed. */
typedef void (* TwitterProcessListMemberFunc)(TwitterUser user, gpointer user_data);
/*! Specifies the type of function which is called when a user is parsed. */
typedef void (* TwitterProcessUserFunc)(TwitterUser user, gpointer user_data);
/*! Specifies the type of function which is called when a direct message is parsed. */
typedef void (* TwitterProcessDirectMessageFunc)(TwitterDirectMessage message, TwitterUser sender, TwitterUser receiver, gpointer user_data);
/*! Specifies the type of function which is called when an id is parsed. */
typedef void (* TwitterProcessIdFunc)(const gchar *id, gpointer user_data);

/**
 * \param urlopener an UrlOpener opener
 * \param request_key location to store the received request key
 * \param request_secret location to store the received request secret
 * \param err location to store error messages
 * \return TRUE on success
 *
 * Gets a request key and secret and opens the authorization url in a browser. 
 */
gboolean twitter_request_authorization(UrlOpener *urlopener, gchar **request_key, gchar **request_secret, GError **err);

/**
 * \param twitter_timestamp a Twitter timestamp (e.g. Mon Dec 27 16:35:55 +0000 2010)
 * \return a UNIX timestamp
 *
 * Converts a Twitter timestamp into a UNIX timestamp.
 */
gint64 twitter_timestamp_to_unix_timestamp(const gchar *twitter_timestamp);

/**
 * @}
 * @}
 */
#endif


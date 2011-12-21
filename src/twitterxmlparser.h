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
 * \file twitterxmlparser.h
 * \brief Parsing XML data from Twitter.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 21. June 2011
 */

#ifndef __TWITTER_XML_PARSER_H__
#define __TWITTER_XML_PARSER_H__

#include <glib.h>

#include "twitter.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param parser_func callback to invoke when a status is found
 * \param user_data user data 
 * \param cancellable a GCancellable to abort the operation
 *
 * Parses a Twitter timeline.
 */
void twitter_xml_parse_timeline(const gchar *xml, gint length, TwitterProcessStatusFunc parser_func, gpointer user_data, GCancellable *cancellable);

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param parser_func callback to invoke when a list is found
 * \param user_data user data 
 *
 * Parses Twitter list data.
 */
void twitter_xml_parse_lists(const gchar *xml, gint length, TwitterProcessListFunc parser_func, gpointer user_data);

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param user TwitterUser structure to store found user information
 * \param list TwitterList structure to store found list information
 * \return TRUE if the data could be parsed successfully 
 *
 * Parses Twitter list data.
 */
gboolean twitter_xml_parse_list(const gchar *xml, gint length, TwitterUser *user, TwitterList *list);

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param parser_func callback to invoke when a list member is found
 * \param next_cursor location to store the next cursor
 * \param cursor_size size of the buffer to store the next cursor
 * \param user_data user data 
 *
 * Parses Twitter list member data.
 */
void twitter_xml_parse_list_members(const gchar *xml, gint length, TwitterProcessListMemberFunc parser_func, gchar next_cursor[], gint cursor_size, gpointer user_data);

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param parser_func callback to invoke when a list member is found
 * \param user_data user data 
 *
 * Parses Twitter user data.
 */
void twitter_xml_parse_user_details(const gchar *xml, gint length, TwitterProcessUserFunc parser_func, gpointer user_data);

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param parser_func callback to invoke when a direct message is found
 * \param user_data user data 
 *
 * Parses direct messages.
 */
void twitter_xml_parse_direct_messages(const gchar *xml, gint length, TwitterProcessDirectMessageFunc parser_func, gpointer user_data);

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param friendship location to store friendship information
 * \return TRUE if data could be parsed.
 *
 * Parses friendship information.
 */
gboolean twitter_xml_parse_friendship(const gchar *xml, gint length, TwitterFriendship *friendship);

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param parser_func callback to invoke when an id is found
 * \param next_cursor location to store the next cursor
 * \param cursor_size size of the buffer to store the next cursor
 * \param cancellable a GCancellable to abort the operation
 * \param user_data user data
 *
 * Parses ids.
 */
void twitter_xml_parse_ids(const gchar *xml, gint length, TwitterProcessIdFunc parser_func, gchar next_cursor[], gint cursor_size, GCancellable *cancellable, gpointer user_data);

/**
 * \param xml XML data
 * \param length length of the XML data
 * \param status location to store status information
 * \param user location to store user information
 * \return TRUE if data could be parsed.
 *
 * Parses status information.
 */
gboolean twitter_xml_parse_status(const gchar *xml, gint length, TwitterStatus *status, TwitterUser *user);

/**
 * @}
 * @}
 */
#endif


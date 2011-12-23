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
 * \file twitterjsonparser.h
 * \brief Parsing JSON data from Twitter.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 23. December 2011
 */

#ifndef __TWITTER_JSON_PARSER_H__
#define __TWITTER_JSON_PARSER_H__

#include <glib.h>
#include <gio/gio.h>

#include "twitter.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/**
 * \param json JSON data
 * \param length length of the XML data
 * \param parser_func callback to invoke when a status is found
 * \param user_data user data 
 * \param cancellable a GCancellable to abort the operation
 *
 * Parses a Twitter search result.
 */
void twitter_json_parse_search_result(const gchar *json, gint length, TwitterProcessStatusFunc parser_func, gpointer user_data, GCancellable *cancellable);

/**
 * @}
 * @}
 */
#endif


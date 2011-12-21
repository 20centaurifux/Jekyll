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
 * \file twittersync.h
 * \brief Twitter synchronization functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 8. May 2011
 */

#ifndef __TWITTERSYNC_H__
#define __TWITTERSYNC_H__

#include <glib.h>
#include <gio/gio.h>

#include "twitterdb.h"
#include "net/twitterwebclient.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/**
 * \param handle database handle
 * \param client a TwitterWebClient instance
 * \param status_count pointer to a location to store the number of new statuses
 * \param sync_members TRUE to synchronize list members
 * \param cancellable a GCancellable
 * \param err a GError structure to store failure messages
 * \return TRUE on success.
 *
 * Updates the lists of an account.
 */
gboolean twittersync_update_lists(TwitterDbHandle *handle, TwitterWebClient *client, gint *status_count, gboolean sync_members, GCancellable *cancellable, GError **err);

/**
 * \param handle database handle
 * \param client a TwitterWebClient instance
 * \param status_count pointer to a location to store the number of new statuses
 * \param cancellable a GCancellable
 * \param err a GError structure to store failure messages
 * \return TRUE on success.
 *
 *
 * Updates the home timeline, user timeline and replies of an user account.
 */
gboolean twittersync_update_timelines(TwitterDbHandle *handle, TwitterWebClient *client, gint *status_count, GCancellable *cancellable, GError **err);

/**
 * \param handle database handle
 * \param client a TwitterWebClient instance
 * \param message_count pointer to a location to store the number of new messages 
 * \param err a GError structure to store failure messages
 * \return TRUE on success.
 *
 * Updates the direct messages of an account.
 */
gboolean twittersync_update_direct_messages(TwitterDbHandle *handle, TwitterWebClient *client, gint *message_count, GError **err);

/**
 * \param handle database handle
 * \param client a TwitterWebClient instance
 * \param cancellable a GCancellable
 * \param err a GError structure to store failure messages
 * \return TRUE on success.
 *
 * Updates friends of an account.
 */
gboolean twittersync_update_friends(TwitterDbHandle *handle, TwitterWebClient *client, GCancellable *cancellable, GError **err);

/**
 * \param handle database handle
 * \param client a TwitterWebClient instance
 * \param cancellable a GCancellable
 * \param err a GError structure to store failure messages
 * \return TRUE on success.
 *
 * Updates followers of an account.
 */
gboolean twittersync_update_followers(TwitterDbHandle *handle, TwitterWebClient *client, GCancellable *cancellable, GError **err);

/**
 * @}
 * @}
 */
#endif


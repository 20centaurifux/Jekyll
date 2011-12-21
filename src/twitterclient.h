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
 * \file twitterclient.h
 * \brief Access to Twitter webservice and data caching.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 23. September 2011
 */

#ifndef __TWITTER_CLIENT_H__
#define __TWITTER_CLIENT_H__

#include <glib-object.h>
#include <glib.h>
#include <gio/gio.h>

#include "configuration.h"
#include "cache.h"
#include "twitter.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/*! Gets GType. */
#define TWITTER_CLIENT_TYPE              twitter_client_get_type()
/*! Cast to TwitterClient. */
#define TWITTER_CLIENT(inst)             (G_TYPE_CHECK_INSTANCE_CAST((inst), TWITTER_CLIENT_TYPE, TwitterClient))
/*! Cast to TwitterClientClass. */
#define TWITTER_CLIENT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST((class), TWITTER_CLIENT_TYPE, TwitterClientClass))
/*! Check if instance is TwitterClient. */
#define IS_TWITTER_CLIENT(inst)          (G_TYPE_CHECK_INSTANCE_TYPE((inst), TWITTER_CLIENTTYPE))
/*! Check if class is TwitterClientClass. */
#define IS_TWITTER_CLIENT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE((class), TWITTER_CLIENTTYPE))
/*! Get TwitterClientClass from TwitterClient. */
#define TWITTER_CLIENT_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS((inst), TWITTER_CLIENT_TYPE, TwitterClientClass))

/*! Default cache lifetime. */
#define TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME 60

/*!A type definition for _TwitterClientPrivate. */
typedef struct _TwitterClientPrivate TwitterClientPrivate;

/*!A type definition for _TwitterClientClass. */
typedef struct _TwitterClientClass TwitterClientClass;

/*!A type definition for _TwitterClient. */
typedef struct _TwitterClient TwitterClient;

/**
 * \struct _TwitterClientClass
 * \brief The _TwitterClient class structure.
 *
 * The _TwitterClient class structure.
 * It has the following properties:
 * - \b cache: Internal cache. (Cache, ro)\n
 * - \b lifetime: Lifetime of stored data. (integer, ro)
 */
struct _TwitterClientClass
{
	/*! The parent class. */
	GObjectClass parent_class;

	/**
	 * \param twitterclient TwitterClient instance
	 * \param username an username
	 * \param acceess_key an access key
	 * \param acceess_secret an access secret
	 *
	 * Registers a Twitter account.
	 */
	void (* add_account)(TwitterClient *client, const gchar * restrict username, const gchar * restrict access_key, const gchar * restrict access_secret);

	/**
	 * \param twitterclient TwitterClient instance
	 *
	 * Removes all registered accounts.
	 */
	void (* clear_accounts)(TwitterClient *client);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param username an username
	 * \param func callback function
	 * \param user_data  user data
	 * \param cancellable a GCancellable to abort the operation
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Gets tweets from a public timeline.
	 */
	gboolean (* process_public_timeline)(TwitterClient *twitterclient, const gchar *username, TwitterProcessStatusFunc func,
	                                     gpointer user_data, GCancellable *cancellable, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param username an username
	 * \param func callback function
	 * \param user_data  user data
	 * \param cancellable a GCancellable to abort the operation
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Gets replies.
	 */
	gboolean (* process_replies)(TwitterClient *twitterclient, const gchar *username, TwitterProcessStatusFunc func,
	                             gpointer user_data, GCancellable *cancellable, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param username an username
	 * \param func callback function
	 * \param user_data  user data
	 * \param cancellable a GCancellable to abort the operation
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 
	 * Gets tweets from an usertimline.
	 */
	gboolean (* process_usertimeline)(TwitterClient *twitterclient, const gchar *username, TwitterProcessStatusFunc func,
	                                  gpointer user_data, GCancellable *cancellable, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param username an username
	 * \param list a list
	 * \param func callback function
	 * \param user_data  user data
	 * \param cancellable a GCancellable to abort the operation
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 
	 * Gets tweets from a list.
	 */
	gboolean (* process_list)(TwitterClient *twitterclient, const gchar * restrict username, const gchar * restrict list, TwitterProcessStatusFunc func,
	                          gpointer user_data, GCancellable *cancellable, GError **err);
	/**
	 * \param twitterclient TwitterClient instance
	 * \param owner list owner
	 * \param list a list
	 * \param username user to add to the list
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Adds an user to a list.
	 */
	gboolean (* add_user_to_list)(TwitterClient *twitterclient, const gchar * restrict owner, const gchar * restrict listname, const gchar *username, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param owner list owner
	 * \param listname a list
	 * \param username user to remove from the list
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Removes an user from a list.
	 */
	gboolean (* remove_user_from_list)(TwitterClient *twitterclient, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param owner list owner
	 * \param listname old listname
	 * \param new_listname new listname
	 * \param description list description
	 * \param protected protected
	 * \param err structure to store failure messages
	 * \return new name of the list or NULL on failure
	 *
	 * Updates a list.
	 */
	gchar *(* update_list)(TwitterClient *twitterclient, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict new_listname, const gchar * restrict description, gboolean protected, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param owner list owner
	 * \param listname listname
	 * \param protected protected
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Creates a list.
	 */
	gboolean (* add_list)(TwitterClient *twitterclient, const gchar * restrict owner, const gchar * restrict listname, gboolean protected, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param owner list owner
	 * \param listname name of the list to remove
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Removes a list.
	 */
	gboolean (* remove_list)(TwitterClient *twitterclient, const gchar * restrict owner, const gchar * restrict listname, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param account name of the Twitter account
	 * \param follower follower to block
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Blocks a follower.
	 */
	gboolean (* block_follower)(TwitterClient *twitterclient, const gchar * restrict account, const gchar * restrict follower, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param account name of the Twitter account
	 * \param friend friend to add
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Adds a friend.
	 */
	gboolean (* add_friend)(TwitterClient *twitterclient, const gchar * restrict account, const gchar * restrict friend, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param account name of the Twitter account
	 * \param friend friend to remove
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Removes a friend.
	 */
	gboolean (* remove_friend)(TwitterClient *twitterclient, const gchar * restrict account, const gchar * restrict friend, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param account name of the Twitter account
	 * \param text text to post
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Posts a tweet.
	 */
	gboolean (* post)(TwitterClient *client, const gchar * restrict account, const gchar * restrict text, GError **err);

	/**
	 * \param twitterclient TwitterClient instance
	 * \param account name of the Twitter account
	 * \param guid guid to retweet
	 * \param err structure to store failure messages
	 * \return TRUE on success
	 *
	 * Retweets a tweet.
	 */
	gboolean (* retweet)(TwitterClient *client, const gchar * restrict account, const gchar * restrict guid, GError **err);
};

/**
 * \struct _TwitterClient
 * \brief Access to Twitter webservice and data caching.
 *
 * The TwitterClient manages twitter data. See _TwitterClientClass for more details.
 */
struct _TwitterClient
{
	/*! The parent instance. */
	GObject parent_instance;

	/*! Private data. */
	TwitterClientPrivate *priv;
};

/*! See _TwitterClientClass::add_account for further information. */
void twitter_client_add_account(TwitterClient *twitter_client, const gchar * restrict username, const gchar * restrict access_key, const gchar * restrict access_secret);
/*! See _TwitterClientClass::clear_accounts for further information. */
void twitter_client_clear_accounts(TwitterClient *client);
/*! See _TwitterClientClass::process_public_timeline for further information. */
gboolean twitter_client_process_public_timeline(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                                                gpointer user_data, GCancellable *cancellable, GError **err);
/*! See _TwitterClientClass::process_replies for further information. */
gboolean twitter_client_process_replies(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                                        gpointer user_data, GCancellable *cancellable, GError **err);
/*! See _TwitterClientClass::process_usertimeline for further information. */
gboolean twitter_client_process_usertimeline(TwitterClient *twitter_client, const gchar *username, TwitterProcessStatusFunc func,
                                             gpointer user_data, GCancellable *cancellable, GError **err);
/*! See _TwitterClientClass::process_list for further information. */
gboolean twitter_client_process_list(TwitterClient *twitter_client, const gchar * restrict username, const gchar * restrict list, TwitterProcessStatusFunc func,
                                     gpointer user_data, GCancellable *cancellable, GError **err);
/*! See _TwitterClientClass::add_user_to_list for further information. */
gboolean twitter_client_add_user_to_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err);
/*! See _TwitterClientClass::remove_user_from_list for further information. */
gboolean twitter_client_remove_user_from_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict username, GError **err);
/*! See _TwitterClientClass::update_list for further information. */
gchar *twitter_client_update_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, const gchar * restrict new_listname, const gchar * restrict description, gboolean protected, GError **err);
/*! See _TwitterClientClass::add_list for further information. */
gboolean twitter_client_add_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, gboolean protected, GError **err);
/*! See _TwitterClientClass::remove_list for further information. */
gboolean twitter_client_remove_list(TwitterClient *twitter_client, const gchar * restrict owner, const gchar * restrict listname, GError **err);
/*! See _TwitterClientClass::block_follower for further information. */
gboolean twitter_client_block_follower(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict follower, GError **err);
/*! See _TwitterClientClass::add_friend for further information. */
gboolean twitter_client_add_friend(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict friend, GError **err);
/*! See _TwitterClientClass::remove_friend for further information. */
gboolean twitter_client_remove_friend(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict friend, GError **err);
/*! See _TwitterClientClass::post for further information. */
gboolean twitter_client_post(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict text, GError **err);
/*! See _TwitterClientClass::retweet for further information. */
gboolean twitter_client_retweet(TwitterClient *twitter_client, const gchar * restrict account, const gchar * restrict guid, GError **err);

/**
 * \return a GType
 *
 * Registers the type in the type system.
 */
GType twitter_client_get_type (void)G_GNUC_CONST;

/**
 * \param cache a cache
 * \param lifetime lifetime of each cache item
 * \return a new TwitterClient instance.
 *
 * Creates a new TwitterClient instance.
 */
TwitterClient *twitter_client_new(Cache *cache, gint lifetime);

/**
 * @}
 * @}
 */
#endif


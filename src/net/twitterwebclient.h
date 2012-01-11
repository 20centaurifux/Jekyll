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
 * \file twitterwebclient.h
 * \brief A Twitter client.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 11. January 2012
 */

#ifndef __TWITTER_WEB_CLIENT_H__
#define __TWITTER_WEB_CLIENT_H__

#include <glib-object.h>

/**
 * @addtogroup Net
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

/*! Gets the GType. */
#define TWITTER_WEB_CLIENT_TYPE            twitter_web_client_get_type()
/*! Cast to TwitterWebClient. */
#define TWITTER_WEB_CLIENT(inst)           (G_TYPE_CHECK_INSTANCE_CAST((inst), TWITTER_WEB_CLIENT_TYPE, TwitterWebClient))
/*! Cast To GtkTwitterStatusClass. */
#define TWITTER_WEB_CLIENT_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST((class), TWITTER_WEB_CLIENT_TYPE, TwitterWebClientClass))
/*! Check if instance is TwitterWebClient. */
#define IS_TWITTER_WEB_CLIENT(inst)        (G_TYPE_CHECK_INSTANCE_TYPE((inst), TWITTER_WEB_CLIENTTYPE))
/*! Check if class is TwitterWebClientClass. */
#define IS_TWITTER_WEB_CLIENT_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE((class), TWITTER_WEB_CLIENTTYPE))
/*! Get TwitterWebClientClass from TwitterWebClient. */
#define TWITTER_WEB_CLIENT_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS((inst), TWITTER_WEB_CLIENT_TYPE, TwitterWebClientClass))

/*! Hostname of the Twitter web API server. */
#define TWITTER_API_HOSTNAME               "api.twitter.com"
/*! Hostname of the Twitter search server. */
#define TWITTER_SEARCH_API_HOSTNAME        "search.twitter.com"

/*!A type definition for _TwitterWebClientPrivate. */
typedef struct _TwitterWebClientPrivate TwitterWebClientPrivate;

/*!A type definition for _TwitterWebClientClass. */
typedef struct _TwitterWebClientClass TwitterWebClientClass;

/*!A type definition for _TwitterWebClient. */
typedef struct _TwitterWebClient TwitterWebClient;

/**
 * \struct _TwitterWebClientClass
 * \brief The _TwitterWebClient class structure.
 *
 * The _TwitterWebClient class structure.
 * It has the following properties:
 * - \b username: Name of the user account. (string, rw)\n
 * - \b password: Password of the user account. (string, rw)\n
 * - \b format: The desired data format. (string, rw)\n
 * - \b status-count: Number of statuses to receive. (integer, rw)
 */
struct _TwitterWebClientClass
{
	/*! The parent class. */
	GObjectClass parent_class;

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \return Returns the last occured error.
	 *
	 * Gets the last error message.
	 */
	const GError *(* get_last_error)(TwitterWebClient *twitterwebclient);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param username username of the account owner
	 *
	 * Sets the username.
	 */
	void (* set_username)(TwitterWebClient *twitterwebclient, const gchar *username);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \return pointer to the username
	 *
	 * Gets the username.
	 */
	const gchar *(*get_username)(TwitterWebClient *twitterwebclient);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param consumer_key oauth consumer key
	 * \param consumer_secret oauth consumer secret
	 * \param access_key the oauth access key
	 * \param access_secret oauth access secret
	 *
	 * Sets the user credentials.
	 */
	void (* set_oauth_authorization)(TwitterWebClient *twitterwebclient, const gchar * restrict consumer_key, const gchar * restrict consumer_secret, const gchar * restrict access_key, const gchar * restrict access_secret);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param format a format supported by the Twitter API
	 *
	 * Sets the format (e.g. "xml", "json" or "atom").
	 */
	void (* set_format)(TwitterWebClient *twitterwebclient, const gchar *format);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets the home timeline.
	 */
	gboolean (* get_home_timeline)(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets mentions.
	 */
	gboolean (* get_mentions)(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets direct messages.
	 */
	gboolean (* get_direct_messages)(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param username an username or guid
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets the user timeline.
	 */
	gboolean (* get_user_timeline)(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param username an username or guid
	 * \param listname name of a list or its guid
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets the timeline from a list. If username is NULL the name of the connected user will be used.
	 */
	gboolean (* get_timeline_from_list)(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict listname, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param username an username or guid
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets all (public) lists from the specified user. If username is NULL the name of the connected user will be used.
	 */
	gboolean (* get_lists)(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param username an username or guid
	 * \param listname name of the list or its guid
	 * \param cursor breaks the result into pages
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets all members of a list.
	 */
	gboolean (* get_users_from_list)(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict listname, const gchar * restrict cursor, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param listname name of the list to create
	 * \param protected protect list
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Creates a new list.
	 */
	gboolean (* add_list)(TwitterWebClient *twitterwebclient, const gchar *listname, gboolean protected, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param listname name of the list to remove or its guid
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Removes the specified list.
	 */
	gboolean (* remove_list)(TwitterWebClient *twitterwebclient, const gchar *listname, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param listname name of the list to update or its guid
	 * \param new_listname new name of the list
	 * \param description list description
	 * \param protected protected
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Updates the specified list.
	 */
	gboolean (* update_list)(TwitterWebClient *twitterwebclient, const gchar *listname, const gchar *new_listname, const gchar *description, gboolean protected, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param listname name of a list or guid
	 * \param username an username
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Adds an user to a list.
	 */
	gboolean (* add_user_to_list)(TwitterWebClient *twitterwebclient, const gchar * restrict listname, const gchar * restrict username, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param listname name of a list or guid
	 * \param username an username
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Removes an user from a list.
	 */
	gboolean (* remove_user_from_list)(TwitterWebClient *twitterwebclient, const gchar * restrict listname, const gchar * restrict username, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param word string to search for
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Searches for the given word.
	 */
	gboolean (* search)(TwitterWebClient *twitterwebclient, const gchar *word, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param username an username or guid
	 * \param cursor breaks the result into pages
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets all followers from the specified user. If username is NULL the name of the connected user will be used.
	 */
	gboolean (* get_followers)(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict cursor, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param username an username or guid
	 * \param cursor breaks the result into pages
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets all friends of the specified user. If username is NULL the name of the connected user will be used.
	 */
	gboolean (* get_friends)(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict cursor, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param friend an username
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Adds an user to the list of friends.
	 */
	gboolean (* add_friend)(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param friend an username
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Removes an user from the list of friends.
	 */
	gboolean (* remove_friend)(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param user an username or guid
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Blocks an user.
	 */
	gboolean (* block_user)(TwitterWebClient *twitterwebclient, const gchar *user, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param username an username or guid
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets details of an user.
	 */
	gboolean (* get_user_details)(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param id guid of an user
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Gets details of an user.
	 */
	gboolean (* get_user_details_by_id)(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param text text of the status to post
	 * \param prev_status the guid of an status that the update is in reply to
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Publishes a tweet.
	 */
	gboolean (* post_tweet)(TwitterWebClient *twitterwebclient, const gchar *text, const gchar *prev_status, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param id guid of the status to remove
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Removes a tweet.
	 */
	gboolean (* remove_tweet)(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length);

	/**
	 * \param twitterwebclient TwitterWebClient instance
	 * \param id guid of the status to retweet
	 * \param buffer a buffer
	 * \param length length of the buffer
	 * \return TRUE on success
	 *
	 * Retweets a status.
	 */
	gboolean (* retweet)(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length);
};

/**
 * \struct _TwitterWebClient
 * \brief A Twitter client.
 *
 * TwitterClient provides access to the Twitter webservice. See _TwitterWebClientClass for more details.
 */
struct _TwitterWebClient
{
	/*! The parent instance. */
	GObject parent_instance;

	/*! Private data. */
	TwitterWebClientPrivate *priv;
};

/*! See _TwitterWebClientClass::get_last_error for further information. */
const GError *twitter_web_client_get_last_error(TwitterWebClient *twitterwebclient);
/*! See _TwitterWebClientClass::set_username for further information. */
void twitter_web_client_set_username(TwitterWebClient *twitterwebclient, const gchar *username);
/*! See _TwitterWebClientClass::get_username for further information. */
const gchar *twitter_web_client_get_username(TwitterWebClient *twitterwebclient);
/*! See _TwitterWebClientClass::set_oauth_authorization for further information. */
void twitter_web_client_set_oauth_authorization(TwitterWebClient *twitterwebclient, const gchar * restrict consumer_key, const gchar * restrict consumer_secret, const gchar * restrict access_key, const gchar * restrict access_secret);
/*! See _TwitterWebClientClass::set_format for further information. */
void twitter_web_client_set_format(TwitterWebClient *twitterwebclient, const gchar *format);
/*! See _TwitterWebClientClass::get_home_timeline for further information. */
gboolean twitter_web_client_get_home_timeline(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_mentions for further information. */
gboolean twitter_web_client_get_mentions(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_direct_messages for further information. */
gboolean twitter_web_client_get_direct_messages(TwitterWebClient *twitterwebclient, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_user_timeline for further information. */
gboolean twitter_web_client_get_user_timeline(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_timeline_from_list for further information. */
gboolean twitter_web_client_get_timeline_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict listname, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_lists for further information. */
gboolean twitter_web_client_get_lists(TwitterWebClient *twitterwebclient, const gchar *username, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_users_from_list for further information. */
gboolean twitter_web_client_get_users_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict listname, const gchar * restrict cursor, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::add_list for further information. */
gboolean twitter_web_client_add_list(TwitterWebClient *twitterwebclient, const gchar *listname, gboolean protected, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::remove_list for further information. */
gboolean twitter_web_client_remove_list(TwitterWebClient *twitterwebclient, const gchar *listname, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::update_list for further information. */
gboolean twitter_web_client_update_list(TwitterWebClient *twitterwebclient, const gchar *listname, const gchar *new_listname, const gchar *description, gboolean protected, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::add_user_to_list for further information. */
gboolean twitter_web_client_add_user_to_list(TwitterWebClient *twitterwebclient, const gchar * restrict listname, const gchar * restrict username, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::remove_user_from_list for further information. */
gboolean twitter_web_client_remove_user_from_list(TwitterWebClient *twitterwebclient, const gchar * restrict listname, const gchar * restrict username, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::search for further information. */
gboolean twitter_web_client_search(TwitterWebClient *twitterwebclient, const gchar *word, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_followers for further information. */
gboolean twitter_web_client_get_followers(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict cursor, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_friends for further information. */
gboolean twitter_web_client_get_friends(TwitterWebClient *twitterwebclient, const gchar * restrict username, const gchar * restrict cursor, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::add_friend for further information. */
gboolean twitter_web_client_add_friend(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::remove_friend for further information. */
gboolean twitter_web_client_remove_friend(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::block_user for further information. */ 
gboolean twitter_web_client_block_user(TwitterWebClient *twitterwebclient, const gchar *user, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_user_details for further information. */
gboolean twitter_web_client_get_user_details(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::get_user_details_by_id for further information. */
gboolean twitter_web_client_get_user_details_by_id(TwitterWebClient *twitterwebclient, const gchar *friend, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::post_tweet for further information. */
gboolean twitter_web_client_post_tweet(TwitterWebClient *twitterwebclient, const gchar *text, const gchar *prev_status, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::remove_tweet for further information. */
gboolean twitter_web_client_remove_tweet(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length);
/*! See _TwitterWebClientClass::retweet for further information. */
gboolean twitter_web_client_retweet(TwitterWebClient *twitterwebclient, const gchar *id, gchar **buffer, gint *length);

/**
 * \return a GType
 *
 * Registers the type in the type system.
 */
GType twitter_web_client_get_type (void)G_GNUC_CONST;

/**
 * \return a new TwitterWebClient instance.
 *
 * Creates a new TwitterWebClient instance.
 */
TwitterWebClient *twitter_web_client_new(void);

/**
 * @}
 * @}
 */
#endif

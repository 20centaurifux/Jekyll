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
 * \file application.h
 * \brief Application information.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 17. August 2012
 */

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

/**
 * @addtogroup Core
 * @{
 */

/*! Name of the application. */
#define APPLICATION_NAME          "Jekyll"
/*! A short description of the application. */
#define APPLICATION_DESCRIPTION   "A crossplatform Twitter client."
/*! The author of the program. */
#define APPLICATION_AUTHOR        "Sebastian Fedrau"
/*! Copyright of the program. */
#define APPLICATION_COPYRIGHT     "Copyright (C) 2010-2012, Sebastian Fedrau"
/*! Project's website. */
#define APPLICATION_WEBSITE       "https://github.com/20centaurifux/jekyll"

/*! Major release number.*/
#define VERSION_MAJOR             0
/*! Minor release number. */
#define VERSION_MINOR             3
/*! Patch level of the release. */
#define VERSION_PATCH_LEVEL       1

/*! Name of the directory to store configuration data. */
#define CONFIG_DIR                ".jekyll"
/*! Name of the configuration file. */
#define CONFIG_FILENAME           "config.xml"

/*! Filename of the license file. */
#define LICENSE_FILE              "LICENSE.txt"

/*! Maximum cache size in bytes. */
#define CACHE_LIMIT               81920
/*! Filename of the swap folder. */
#define CACHE_SWAP_FOLDER         ".swap"

/*! Twitter url to get a request token. */
#define TWITTER_REQUEST_TOKEN_URL "http://twitter.com/oauth/request_token"
/*! Twitter authorization url. */
#define TWITTER_AUTHORIZATION_URL "http://twitter.com/oauth/authorize"
/*! Twitter url to get an access token. */
#define TWITTER_ACCESS_TOKEN_URL  "http://twitter.com/oauth/access_token"

/*! OAuth consumer key of the application. */
#define OAUTH_CONSUMER_KEY        TWITTER_CONSUMER_KEY
/*! OAuth consumer secret of the application. */
#define OAUTH_CONSUMER_SECRET     TWITTER_CONSUMER_SECRET

/*! Directory of the Twitter database file. */
#define TWTTER_DATABASE_FOLDER    ".database"
/*! Twitter database filename. */
#define TWITTER_DATABASE_FILE     "twitterdb.db"
/*! Major version of the database model. */
#define DATABASE_MODEL_MAJOR      0
/*! Minor version of the database model. */
#define DATABASE_MODEL_MINOR      2

/*! Gettext package name. */
#define GETTEXT_PACKAGE_NAME      "jekyll"

/**
 * @}
 */
#endif


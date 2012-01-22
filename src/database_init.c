/***************************************************************************
    begin........: May 2011
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
 * \file database_init.c
 * \brief Startup functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 11. January 2012
 */

/**
 * @addtogroup Core
 * @{
 */

#include "database_init.h"
#include "application.h"

/*
 *	database upgrade:
 */
gboolean
_database_init_upgrade(TwitterDbHandle *handle, gint major, gint minor, GError **err)
{
	gboolean result = FALSE;

	if(major == 0 && minor == 1)
	{
		result = twitterdb_upgrade_0_1_to_0_2(handle, err);
	}

	return result;
}

/*
 *	public:
 */
DatabaseInitResult
database_init(GError **err)
{
	TwitterDbHandle *handle;
	gint major = DATABASE_MODEL_MAJOR;
	gint minor = DATABASE_MODEL_MINOR;
	DatabaseInitResult result = DATABASE_INIT_FAILURE;

	/* connect to database */
	g_debug("Connecting to database");
	if((handle = twitterdb_get_handle(err)))
	{
		/* check if version table does exist */
		g_debug("Searching for version table");
		if(!twitterdb_table_exists(handle, "version", err))
		{
			if(!*err)
			{
				/* version table does not exist => create tables */
				g_debug("Couldn't find version information => creating tables");
				if(twitterdb_init(handle, major, minor, TRUE, err))
				{
					result = DATABASE_INIT_FIRST_INITIALIZATION;
				}
				else
				{
					g_warning("Couldn't create database tables.");
				}
			}
		}
		else
		{
			/* table does exist => get database version */
			g_debug("Getting database version");
			if(twitterdb_get_version(handle, &major, &minor, err))
			{
				g_debug("Found version: %d.%d", major, minor);
				result = DATABASE_INIT_SUCCESS;
			}
			else
			{
				g_warning("Couldn't get database version.");
			}
		}

		if(result == DATABASE_INIT_SUCCESS)
		{
			/* test version */
			g_debug("Testing database version: %d.%d <=> %d.%d", major, minor, DATABASE_MODEL_MAJOR, DATABASE_MODEL_MINOR);
			if(major > DATABASE_MODEL_MAJOR || (major == DATABASE_MODEL_MAJOR && minor > DATABASE_MODEL_MINOR))
			{
				result = DATABASE_INIT_APPLICATION_OUTDATED;
			}
			if(major < DATABASE_MODEL_MAJOR || (major == DATABASE_MODEL_MAJOR && minor < DATABASE_MODEL_MINOR))
			{
				if(!_database_init_upgrade(handle, major, minor, err))
				{
					result = DATABASE_INIT_FAILURE;
				}
			}
			else if(major != DATABASE_MODEL_MAJOR || minor != DATABASE_MODEL_MINOR)
			{
				result = DATABASE_INIT_FAILURE;
			}
		}

		/* close connection */
		twitterdb_close_handle(handle);
	}

	return result;
}

/**
 * @}
 */


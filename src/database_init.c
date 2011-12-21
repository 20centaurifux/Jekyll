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
 * \date 29. May 2011
 */

/**
 * @addtogroup Core
 * @{
 */

#include "database_init.h"
#include "application.h"

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
		/* try to get database version */
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
			/* get database version */
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


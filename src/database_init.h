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
 * \file database_init.h
 * \brief Database initialization functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 28. May 2011
 */

#include "twitterdb.h"

#ifndef __DATABASE_INIT_H__
#define __DATABASE_INIT_H__

/**
 * @addtogroup Core
 * @{
 */

/**
 * \enum DatabaseInitResult
 * \brief Database initialization result codes.
 */
typedef enum
{
	/*! Database initialized successfully. */
	DATABASE_INIT_SUCCESS,
	/*! Database could not be initialized. */
	DATABASE_INIT_FAILURE,
	/*! Database has been initialized for the very first time. */
	DATABASE_INIT_FIRST_INITIALIZATION,
	/*! Application is outdated. */
	DATABASE_INIT_APPLICATION_OUTDATED,
	/*! Database has been upgraded successfully. */
	DATABASE_INIT_DATABASE_UPGRADED
} DatabaseInitResult;

/**
 * \param err structure to store failure messages
 * \return the result
 *
 * Initializes the database.
 */
DatabaseInitResult database_init(GError **err);

/**
 * @}
 */
#endif


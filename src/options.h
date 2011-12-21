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
 * \file options.h
 * \brief Parsing command line options.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 9. June 2010
 */

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <glib.h>

/**
 * @addtogroup Core
 * @{
 */

/**
 * \struct Options
 * \brief Holds command line options.
 */
typedef struct
{
	/*! Show version and exit. */
	gboolean version;
	/*! Alternative configuration file. */
	gchar *config_filename;
	/*! Print a summary of memory usage on exit. */
	gboolean enable_mem_profile;
	/*! Show debug messages. */
	gboolean enable_debug;
	/*! Don't synchronize application. */
	gboolean no_sync;
} Options;

/**
 * \param argc a pointer to the number of command line arguments
 * \param argv a pointer to the array of command line arguments
 * \param options a pointer to a Options structure
 * \return TRUE if the parsing was successful, FALSE if an error occurred
 *
 * Parses command line options.
 */
gboolean options_parse(gint *argc, gchar ***argv, Options *options);

/**
 * @}
 */
#endif


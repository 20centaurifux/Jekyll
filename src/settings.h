/***************************************************************************
    begin........: May 2010
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
 * \file settings.h
 * \brief Loading and saving configuration settings.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 22. March 2011
 */

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <glib.h>

#include "configuration.h"

/**
 * @addtogroup Core
 * @{
 */

/**
 * \return TRUE if the configuration directory can be accessed
 * 
 * Checks if the configuration directory does exist and tries to create it on failure.
 */
gboolean settings_check_directory(void);

/**
 * \return a new allocated string
 *
 * Builds the name of the default configuration file.
 */
gchar *settings_get_default_config_filename(void);

/**
 * \return a new Config instance or NULL on failure
 *
 * Tries to load the default configuration file.
 */
Config *settings_load_config(void);

/**
 * \param filename file to load
 * \return a new Config instance or NULL on failure
 *
 * Loads the configuration from the given filename.
 */
Config *settings_load_config_from_file(const gchar *filename);

/**
 * \param config a Configuration instance
 * \return TRUE if the given configuration does contain at least one account
 *
 * Checks if a configuration does contain at least one account
 */
gboolean settings_config_has_account(Config *config);

/**
 * \param config a Configuration instance
 * \param overwrite TRUE to overwrite existing values
 *
 * Sets the default settings.
 */
void settings_set_default_settings(Config *config, gboolean overwrite);

/**
 * @}
 */
#endif


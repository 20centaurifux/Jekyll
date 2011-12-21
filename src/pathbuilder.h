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
 * \file pathbuilder.h
 * \brief Building filenames.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2010
 */

#ifndef __PATHBUILDER_H__
#define __PATHBUILDER_H__

#include <glib.h>

/**
 * @addtogroup Core
 * @{
 */

/**
 * Initialize the pathbuilder.
 */
void pathbuilder_init(void);

/**
 * Frees resources allocated by the pathbuilder.
 */
void pathbuilder_free(void);

/**
 * \return a string
 *
 * Returns the user's configuration directory.
 */
const gchar *pathbuilder_get_user_application_directory(void);

/**
 * \return a string
 *
 * Returns the path to the default configuration file.
 */
const gchar *pathbuilder_get_config_filename(void);

/**
 * \return a string
 *
 * Returns the path to the shared data.
 */
const gchar *pathbuilder_get_share_directory(void);

/**
 * \return a string
 *
 * Returns the path to the locate directory.
 */
const gchar *pathbuilder_get_locale_directory(void);

/**
 * \param image filename of the image
 * \return a new allocated string
 *
 * Builds the path to the specified image.
 */
gchar *pathbuilder_build_image_path(const gchar *image);

/**
 * \param size size of the icon
 * \param image filename of the image
 * \return a new allocated string
 *
 * Builds the path to the specified icon.
 */
gchar *pathbuilder_build_icon_path(const gchar * restrict size, const gchar * restrict image);

/**
 * \param key key associated with the path
 * \param path path to store
 *
 * Saves a path in the pathbuilder cache.
 */
void pathbuilder_save_path(const gchar * restrict key, const gchar * restrict path);

/**
 * \param key key of the requested path
 * \return a path or NULL
 *
 * Loads a path from the pathbuilder cache.
 */
const gchar *pathbuilder_load_path(const gchar *key);

/**
 * @}
 */
#endif


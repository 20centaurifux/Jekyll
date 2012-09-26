/***************************************************************************
    begin........: September 2012
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
 * \file completion.h
 * \brief Organize strings for autocompletion support.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 26. September 2012
 */

#include <glib.h>

#ifndef __COMPLETION_H__
#define __COMPLETION_H__

/**
 * @addtogroup Core
 * @{
 */

/**
 * \param filename file to load
 * \return a list containing loaded strings
 *
 * Load strings from a file.
 */
GList *completion_load_file(const char *filename);

/**
 * \param filename file containing stored strings
 * \param text text to apppend
 *
 * Add text to specified file.
 */
void completion_append_string(const char *filename, const char *text);

/**
 * @}
 */
#endif


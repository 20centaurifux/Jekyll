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
 * \file uri.h
 * \brief Uri functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 18. May 2010
 */

#ifndef __URI_H__
#define __URI_H__

#include <glib.h>

/**
 * @addtogroup Net
 * @{
 */

/**
 * \param uri uri to parse
 * \param scheme location to store the scheme
 * \param hostname location to store the hostname
 * \param path location to store the path
 * \return TRUE on success
 *
 * Parses an Uri.
 */
gboolean uri_parse(const gchar *uri, gchar ** restrict scheme, gchar ** restrict hostname, gchar ** restrict path);

/**
 * \param scheme a scheme
 * \return a port number or -1 on failure
 *
 * Tries to detect the correct port from the given scheme.
 */
gint uri_get_port_from_scheme(const gchar *scheme);

/**
 * @}
 */
#endif


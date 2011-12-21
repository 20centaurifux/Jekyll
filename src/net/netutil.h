/***************************************************************************
    begin........: March 2010
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
 * \file netutil.h
 * \brief Network utility functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 1. July 2010
 */

#ifndef __NET_UTIL_H__
#define __NET_UTIL_H__

#include <gio/gio.h>

/**
 * @addtogroup Net
 * @{
 */

/*! Default socket timeout in seconds. */
#define NETUTIL_DEFAULT_SOCKET_TIMEOUT 60

/**
 * \param hostname name or ip address of the desired host
 * \param port port of the remote host
 * \param err holds failure messages
 * \return a new GSocket instance or NULL on failure
 *
 * Establishes a TCP connection to a remote host.
 */
GSocket *network_util_create_tcp_socket(const gchar *hostname, guint port, GError **err);

/**
 * @}
 */
#endif


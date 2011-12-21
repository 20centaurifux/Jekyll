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
 * \file netutil.c
 * \brief Network utility functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 5. July 2011
 */

#include "netutil.h"

/**
 * @addtogroup Net
 * @{
 */

GSocket *
network_util_create_tcp_socket(const gchar *hostname, guint port, GError **err)
{
	GSocketConnectable *addr;
	GSocketAddressEnumerator *enumerator;
	GSocketAddress *sockaddr;
	GSocket *socket = NULL;
	GError *failure = NULL;

	g_debug("Resolving hostnane: %s", hostname);
	if((addr = g_network_address_parse(hostname, port, err)))
	{
		enumerator = g_socket_connectable_enumerate(addr);
		while(!socket && (sockaddr = g_socket_address_enumerator_next(enumerator, NULL, err)))
		{
			g_debug("Creating socket");
			socket = g_socket_new(g_socket_address_get_family(sockaddr), G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, NULL);

			g_socket_set_keepalive(socket, TRUE);

			#if GLIB_MAJOR_VERSION >= 2 && GLIB_MINOR_VERSION >= 26 && GLIB_MICRO_VERSION >= 1
			g_socket_set_timeout(socket, NETUTIL_DEFAULT_SOCKET_TIMEOUT);
			#endif

			if(!g_socket_connect(socket, sockaddr, NULL, &failure))
			{
				if(failure)
				{
					g_debug("Couldn't connect to server: %s", failure->message);
					g_error_free(failure);
				}
				else
				{
					g_debug("Couldn't connect to server: %s", failure->message);
				}

				g_object_unref(socket);
				socket = NULL;
			}
			else
			{
				g_debug("Connection established successfully");
			}
		}

		g_object_unref(enumerator);
		g_object_unref(addr);
	}

	return socket;
}

/**
 * @}
 */


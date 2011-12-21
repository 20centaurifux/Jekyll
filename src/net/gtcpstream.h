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
 * \file gtcpstream.h
 * \brief TCP read and write streaming operations.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. April 2010
 */

#ifndef __G_TCP_STREAM_H__
#define __G_TCP_STREAM_H__

#include <gio/gio.h>

/**
 * @addtogroup Net
 * @{
 */

/*! Get GType. */
#define G_TYPE_TCP_STREAM            (g_tcp_stream_get_type())
/*! Cast to GTcpStream. */
#define G_TCP_STREAM(inst)           (G_TYPE_CHECK_INSTANCE_CAST((inst), G_TYPE_TCP_STREAM, GTcpStream))
/*! Cast to GTcpStreamClass. */
#define G_TCP_STREAM_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST((class), G_TYPE_TCP_STREAM, GTcpStreamClass))
/*! Check if instance is GTcpStream. */
#define G_IS_TCP_STREAM(inst)        (G_TYPE_CHECK_INSTANCE_TYPE((inst), G_TYPE_TCP_STREAM))
/*! Check if class is GTcpStreamClass. */
#define G_IS_TCP_STREAM_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE((class), G_TYPE_TCP_STREAM))
/*! Get GTcpStreamClass from GTcpStream. */
#define G_TCP_STREAM_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS((inst), G_TYPE_TCP_STREAM, GTcpStreamClass))

/*! A type definition for _GTcpStreamPrivate. */
typedef struct _GTcpStreamPrivate    GTcpStreamPrivate;

/*!A type definition for _GTcpStreamClass. */
typedef struct _GTcpStreamClass      GTcpStreamClass;

/*!A type definition for _GTcpStream. */
typedef struct _GTcpStream           GTcpStream;

/**
 * \struct _GTcpStreamClass
 * \brief The _GTcpStream class structure.
 *
 * _GTcpStream provides io streams that both read and write to the same TCP socket. It has
 * the following properties:
 * - \b socket: the underlying socket (GSocket, rw)\n
 * - \b ssl-enabled: activates/deactivates SSL (boolean, rw)\n
 */
struct _GTcpStreamClass
{
	/*! The parent class. */
	GIOStreamClass parent_class;

	/**
	 * \param stream a GTcpStream instance
	 * \return the underlying socket
	 *
	 * Gets the underlying socket.
	 */
	GSocket *(*get_socket)(GTcpStream *stream);

	/**
	 * \param stream a GTcpStream instance
	 * \param err holds failure messages
	 * \return TRUE on success
	 *
	 * Lets the stream work in SSL client mode.
	 */
	gboolean (* ssl_client_init)(GTcpStream *stream, GError **err);

	/**
	 * \param stream a GTcpStream instance
	 * \param err holds failure messages
	 * \return TRUE on success
	 *
	 * Perfoms a SSL handshake.
	 */
	gboolean (* ssl_handshake)(GTcpStream *stream, GError **err);
};

/**
 * \struct _GTcpStream
 * \brief TCP read and write streaming operations.
 *
 * _GTcpStream is a subclass of GIOStream. It provides io streams that both read
 * and write to the same TCP connection. _GTcpStream supports also SSL based
 * client connections. See _GTcpStreamClass for further information.
 */
struct _GTcpStream
{
	/*! The parent instance. */
	GIOStream parent_instance;
	/*! Private data. */
	GTcpStreamPrivate *priv;
};

/*! See _GTcpStreamClass::get_socket() for further information. */
GSocket *g_tcp_stream_get_socket(GTcpStream *stream);
/*! See _GTcpStreamClass::ssl_client_init() for further information. */
gboolean g_tcp_stream_ssl_client_init(GTcpStream *stream, GError **err);
/*! See _GTcpStreamClass::ssl_handshake() for further information. */
gboolean g_tcp_stream_ssl_handshake(GTcpStream *stream, GError **err);

/**
 * \return a GType
 *
 * Registers the type in the type system.
 */
GType g_tcp_stream_get_type (void)G_GNUC_CONST;

/**
 * \param socket a GSocket instance
 * \return a new GTcpStream instance
 *
 * Creates a new GTcpStream instance.
 */
GTcpStream *g_tcp_stream_new(GSocket *socket);

/**
 * \param socket a GSocket instance
 * \return a new GTcpStream instance
 *
 * Creates a new GTcpStream instance and enables SSL.
 */
GTcpStream *g_tcp_stream_new_ssl(GSocket *socket);

/**
 * @}
 */
#endif


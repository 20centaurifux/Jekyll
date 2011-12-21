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
 * \file gsslinputstream.h
 * \brief SSL streaming input operations.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. April 2010
 */

#ifndef __G_SSL_INPUT_STREAM_H__
#define __G_SSL_INPUT_STREAM_H__

#include <gio/gio.h>

#include "openssl.h"

/**
 * @addtogroup Net
 * @{
 */

/*! Get GType. */
#define G_TYPE_SSL_INPUT_STREAM            (g_ssl_input_stream_get_type())
/*! Cast to GSSLInputStream. */
#define G_SSL_INPUT_STREAM(inst)           (G_TYPE_CHECK_INSTANCE_CAST((inst), G_TYPE_SSL_INPUT_STREAM, GSSLInputStream))
/*! Cast to GSSLInputStreamClass. */
#define G_SSL_INPUT_STREAM_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST((class), G_TYPE_SSL_INPUT_STREAM, GSSLInputStreamClass))
/*! Check if instance is GSSLInputStream. */
#define G_IS_SSL_INPUT_STREAM(inst)        (G_TYPE_CHECK_INSTANCE_TYPE((inst), G_TYPE_SSL_INPUT_STREAM))
/*! Check if class is GSSLInputStreamClass. */
#define G_IS_SSL_INPUT_STREAM_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE((class), G_TYPE_SSL_INPUT_STREAM))
/*! Get GSSLInputStreamClass from GSSLInputStream. */
#define G_SSL_INPUT_STREAM_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS((inst), G_TYPE_SSL_INPUT_STREAM, GSSLInputStreamClass))

/*! A type definition for _GSSLInputStreamPrivate. */
typedef struct _GSSLInputStreamPrivate GSSLInputStreamPrivate;

/*! A type definition for _GSSLInputStreamClass. */
typedef struct _GSSLInputStreamClass GSSLInputStreamClass;

/*! A type definition for _GSSLInputStream. */
typedef struct _GSSLInputStream GSSLInputStream;

/**
 * \struct _GSSLInputStreamClass
 * \brief The _GSSLInputStream class structure.
 *
 * The _GSSLInputStream class structure.
 * It has the following properties:
 * - \b ssl: a SSL handle (SSL, rw)\n
 */
struct _GSSLInputStreamClass
{
	/*! The parent class. */
	GInputStreamClass parent_class;
};

/**
 * \struct _GSSLInputStream
 * \brief SSL streaming input operations.
 *
 * _GSSLInputStream is a subclass of GIOInputStream. It provides input streams that
 * take their content from SSL connections. See _GSSLInputStreamClass for more
 * details.
 */
struct _GSSLInputStream
{
	/*! The parent instance. */
	GInputStream parent_instance;
	/*! Private data. */
	GSSLInputStreamPrivate *priv;
};

/**
 * \return a GType
 *
 * Registers the type in the type system.
 */
GType ssl_input_stream_get_type (void)G_GNUC_CONST;

/**
 * \param ssl a SSL handle
 * \return a new GSSLInputStream instance.
 *
 * Creates a new GSSLInputStream instance.
 */
GSSLInputStream *g_ssl_input_stream_new(SSL *ssl);

/**
 * @}
 */
#endif


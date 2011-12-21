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
 * \file gssloutputstream.h
 * \brief SSL streaming output operations.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. April 2010
 */

#ifndef __G_SSL_OUTPUT_STREAM_H__
#define __G_SSL_OUTPUT_STREAM_H__

#include <gio/gio.h>

#include "openssl.h"

/**
 * @addtogroup Net
 * @{
 */

/*! Get GType. */
#define G_TYPE_SSL_OUTPUT_STREAM            (g_ssl_output_stream_get_type())
/*! Cast to GSSLOutputStream. */
#define G_SSL_OUTPUT_STREAM(inst)           (G_TYPE_CHECK_INSTANCE_CAST((inst), G_TYPE_SSL_OUTPUT_STREAM, GSSLOutputStream))
/*! Cast to GSSLOutputStreamClass. */
#define G_SSL_OUTPUT_STREAM_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST((class), G_TYPE_SSL_OUTPUT_STREAM, GSSLOutputStreamClass))
/*! Check if instance is GSSLOutputStream. */
#define G_IS_SSL_OUTPUT_STREAM(inst)        (G_TYPE_CHECK_INSTANCE_TYPE((inst), G_TYPE_SSL_OUTPUT_STREAM))
/*! Check if class is GSSLOutputStreamClass. */
#define G_IS_SSL_OUTPUT_STREAM_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE((class), G_TYPE_SSL_OUTPUT_STREAM))
/*! Get GSSLOutputStreamClass from GSSLOutputStream. */
#define G_SSL_OUTPUT_STREAM_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS((inst), G_TYPE_SSL_OUTPUT_STREAM, GSSLOutputStreamClass))

/*! A type definition for _GSSLOUtputStreamPrivate. */
typedef struct _GSSLOutputStreamPrivate GSSLOutputStreamPrivate;

/*! A type definition for _GSSLOutputStreamClass. */
typedef struct _GSSLOutputStreamClass GSSLOutputStreamClass;

/*! A type definition for _GSSLOutputStream. */
typedef struct _GSSLOutputStream GSSLOutputStream;

/**
 * \struct _GSSLOutputStreamClass
 * \brief The _GSSLOutputStream class structure.
 *
 * The _GSSLOutputStream instance structure implements a GIOOutputStream for SSL connections.
 * It has the following properties:
 * - \b ssl: a SSL handle (SSL, rw)\n
 */
struct _GSSLOutputStreamClass
{
	/*! The parent instance. */
	GOutputStreamClass parent_class;
};

/**
 * \struct _GSSLOutputStream
 * \brief SSL streaming output operations.
 *
 * _GSSLOutputStream is a subclass of GIOOutputStream. It provides output streams that
 * write their content to a SSL connections. See _GSSLOutputStreamClass for more
 * details.
 */
struct _GSSLOutputStream
{
	/*! The parent instance. */
	GOutputStream parent_instance;
	/*! Private data. */
	GSSLOutputStreamPrivate *priv;
};

/**
 * \return a GType
 *
 * Registers the type in the type system.
 */
GType g_ssl_output_stream_get_type (void)G_GNUC_CONST;

/**
 * \param ssl a SSL handle
 * \return a new GSSLOutputStream instance.
 *
 * Creates a new GSSLOutputStream instance.
 */
GSSLOutputStream *g_ssl_output_stream_new(SSL *ssl);

/**
 * @}
 */
#endif


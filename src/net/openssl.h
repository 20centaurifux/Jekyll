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
 * \file openssl.h
 * \brief OpenSSL wrapper functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. April 2010
 */

#ifndef __OPENSSL_H__
#define __OPENSSL_H__

#include <glib.h>
#include <gio/gio.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/**
 * @addtogroup Net
 * @{
 */

/**
 * Initializes the OpenSSL library.
 */
gboolean openssl_init(void);

/**
 * \param ssl an OpenSSL handle
 * \param buffer data to write
 * \param count length of the data to write
 * \param cancellable a GCancellable
 * \param err holds failure messages
 * \return number of bytes that have been written
 *
 * Sends bytes through a SSL connection.
 */
gsize openssl_write(SSL *ssl, const gchar *buffer, gsize count, GCancellable *cancellable, GError **err);

/**
 * \param ssl an OpenSSL handle
 * \param buffer a buffer to store the received bytes
 * \param count number of bytes to read
 * \param cancellable a GCancellable
 * \param err holds failure messages
 * \return number of bytes that have been read
 *
 * Reads bytes through a SSL connection.
 */
gsize openssl_read(SSL *ssl, gchar **buffer, gsize count, GCancellable *cancellable, GError **err);

/**
 * @}
 */
#endif


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
 * \file openssl.c
 * \brief OpenSSL wrapper functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. April 2010
 */

#include "openssl.h"

/**
 * @addtogroup Net
 * @{
 */

static gboolean openssl_initialized = FALSE;

gboolean
openssl_init(void)
{
	if(!openssl_initialized)
	{
		g_debug("Initializing OpenSSL");

		SSL_library_init();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
		openssl_initialized = TRUE;
	}

	return TRUE;
}

gsize
openssl_write(SSL *ssl, const gchar *buffer, gsize count, GCancellable *cancellable, GError **err)
{
	gint errno;
	const gchar *errstr;
	gint ret;

	if((ret = SSL_write(ssl, buffer, count)) <= 0)
	{
		errno = SSL_get_error(ssl, ret);
		if(errno == SSL_ERROR_WANT_READ || errno == SSL_ERROR_WANT_WRITE)
		{
			g_debug("%s: SSL_ERROR_WANT_READ || SSL_ERROR_WANT_WRITE", __func__);
			if(cancellable && g_cancellable_is_cancelled(cancellable))
			{
				g_debug("write operation cancelled");
				return -1;
			}
			return openssl_write(ssl, buffer, count, cancellable, err);
		}
		else
		{
			if(!(errstr = ERR_reason_error_string(ERR_get_error())))
			{
				errstr = strerror(errno);
			}

			if(errstr)
			{
				g_set_error(err, 0, errno, "%s", errstr);
			}
			else
			{
				g_set_error(err, 0, errno, "SSL_write failed: unexpected error");
			}
		}
	}

	return ret;
}

gsize
openssl_read(SSL *ssl, gchar **buffer, gsize count, GCancellable *cancellable, GError **err)
{
	gint errno;
	const gchar *errstr;
	gint ret;

	if((ret = SSL_read(ssl, buffer, count)) <= 0)
	{
		errno = SSL_get_error(ssl, ret);
		if(errno == SSL_ERROR_WANT_READ || errno == SSL_ERROR_WANT_WRITE)
		{
			g_debug("%s: SSL_ERROR_WANT_READ || SSL_ERROR_WANT_WRITE", __func__);
			if(cancellable && g_cancellable_is_cancelled(cancellable))
			{
				g_debug("write operation cancelled");
				return -1;
			}
			return openssl_read(ssl, buffer, count, cancellable, err);
		}
		else
		{
			if(!(errstr = ERR_reason_error_string(ERR_get_error())))
			{
				errstr = strerror(errno);
			}

			if(errstr)
			{
				g_set_error(err, 0, errno, "%s", errstr);
			}
			else
			{
				g_set_error(err, 0, errno, "SSL_read failed: unexpected error");
			}
		}
	}

	return ret;
}

/**
 * @}
 */


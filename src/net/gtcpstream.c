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
 * \file gtcpstream.c
 * \brief TCP read and write streaming operations.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. April 2010
 */

#include "gtcpstream.h"
#include "gsslinputstream.h"
#include "gssloutputstream.h"
#include "openssl.h"

/**
 * @addtogroup Net
 * @{
 */

/*! Define GTcpStream. */
G_DEFINE_TYPE(GTcpStream, g_tcp_stream, G_TYPE_IO_STREAM);

enum
{
	PROP_NONE,
	PROP_SOCKET,
	PROP_SSL_ENABLED
};

/**
 * \struct _GTcpStreamPrivate
 * \brief Private data.
 */
struct _GTcpStreamPrivate
{
	/*! The underlying socket. */
	GSocket *socket;
	/*! A helper class for non-ssl connections. */
	GSocketConnection *connection;
	/*! The provided input stream. */
	GInputStream *input_stream;
	/*! The provided output stream. */
	GOutputStream *output_stream;
	/*! Specifies if SSL is enabled. */
	gboolean ssl_enabled;
	/*! An OpenSSL handle. */
	SSL *ssl;
	/*! An OpenSSL context. */
	SSL_CTX *ctx;
	/*! Indicates if the handshake has been performed successfully. */
	gboolean handshake;
};

/*
 *	implementation:
 */
static GInputStream *
_g_tcp_stream_get_input_stream(GIOStream *io_stream)
{
	GTcpStream *stream = G_TCP_STREAM(io_stream);

	if(!stream->priv->input_stream)
	{
		if(stream->priv->ssl_enabled)
		{
			g_return_val_if_fail(stream->priv->handshake == TRUE, FALSE);
			stream->priv->input_stream = (GInputStream *)g_ssl_input_stream_new(stream->priv->ssl);
		}
		else
		{
			if(!stream->priv->connection)
			{
				stream->priv->connection = g_socket_connection_factory_create_connection(stream->priv->socket);
			}
			stream->priv->input_stream = (GInputStream *)g_io_stream_get_input_stream(G_IO_STREAM(stream->priv->connection));
		}
	}

	return stream->priv->input_stream;
}

static GOutputStream *
_g_tcp_stream_get_output_stream(GIOStream *io_stream)
{
	GTcpStream *stream = G_TCP_STREAM(io_stream);

	if(!stream->priv->output_stream)
	{
		if(stream->priv->ssl_enabled)
		{
			g_return_val_if_fail(stream->priv->handshake == TRUE, FALSE);
			stream->priv->output_stream = (GOutputStream *)g_ssl_output_stream_new(stream->priv->ssl);
		}
		else
		{
			if(!stream->priv->connection)
			{
				stream->priv->connection = g_socket_connection_factory_create_connection(stream->priv->socket);
			}
			stream->priv->output_stream = (GOutputStream *)g_io_stream_get_output_stream(G_IO_STREAM(stream->priv->connection));
		}
	}

	return stream->priv->output_stream;
}


static gboolean
_g_tcp_stream_close(GIOStream *io_stream, GCancellable *cancellable, GError **error)
{
	GTcpStream *stream = G_TCP_STREAM(io_stream);

	if(stream->priv->output_stream)
	{
	    g_output_stream_close(stream->priv->output_stream, cancellable, NULL);
	}

	if(stream->priv->input_stream)
	{
		g_input_stream_close(stream->priv->input_stream, cancellable, NULL);
	}

	return g_socket_close(stream->priv->socket, error);
}

static void
_g_tcp_stream_close_async(GIOStream *io_stream, int io_priority, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *res;
	GIOStreamClass *class;
	GError *error;

	class = G_IO_STREAM_GET_CLASS(io_stream);
	error = NULL;

	if(class->close_fn && !class->close_fn(io_stream, cancellable, &error))
	{
		g_simple_async_report_gerror_in_idle(G_OBJECT(io_stream), callback, user_data, error);
		g_error_free(error);
		return;
	}

	res = g_simple_async_result_new(G_OBJECT(io_stream), callback, user_data, _g_tcp_stream_close_async); g_simple_async_result_complete_in_idle(res);
	g_object_unref(res);
}

static gboolean
_g_tcp_stream_close_finish(GIOStream *io_stream, GAsyncResult *result, GError**error)
{
	return TRUE;
}

static void
_g_tcp_stream_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GTcpStream *stream = G_TCP_STREAM(object);

	switch(prop_id)
	{
		case PROP_SOCKET:
			g_value_set_object(value, stream->priv->socket);
			break;

		case PROP_SSL_ENABLED:
			g_value_set_boolean(value, stream->priv->ssl_enabled);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
_g_tcp_stream_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GTcpStream *stream = G_TCP_STREAM(object);

	switch(prop_id)
	{
		case PROP_SOCKET:
			stream->priv->socket = G_SOCKET(g_value_dup_object(value));
			break;

		case PROP_SSL_ENABLED:
			stream->priv->ssl_enabled = g_value_get_boolean(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static gboolean
_g_tcp_stream_ssl_client_init(GTcpStream *stream, GError **err)
{
	SSL *ssl;
	SSL_CTX *ctx;

	g_debug("Initializing SSL client");

	g_return_val_if_fail(stream->priv->ssl == NULL, FALSE);
	g_return_val_if_fail(stream->priv->ctx == NULL, FALSE);
	g_return_val_if_fail(stream->priv->handshake == FALSE, FALSE);

	g_socket_set_blocking(stream->priv->socket, TRUE);

	if(!(ctx = SSL_CTX_new(SSLv23_client_method())))
	{
		g_set_error(err, 0, 0, "Couldn't allocate memory for SSL context");
		return FALSE;
	}

	if(!(ssl = SSL_new(ctx)))
	{
		g_set_error(err, 0, 0, "Failed to allocate SSL structure");
		SSL_CTX_free(ctx);
		return FALSE;
	}

	if(!SSL_set_fd(ssl, g_socket_get_fd(stream->priv->socket)))
	{
		g_set_error(err, 0, 0, "Failed to associate socket to SSL stream");
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		return FALSE;
	}

	SSL_set_connect_state(ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	stream->priv->ssl = ssl;
	stream->priv->ctx = ctx;

	return TRUE;
}

static gboolean
_g_tcp_stream_ssl_handshake(GTcpStream *stream, GError **err)
{
	gint result;
	gint errno;
	const gchar *errstr;

	g_return_val_if_fail(stream->priv->ssl != NULL, FALSE);
	g_return_val_if_fail(stream->priv->ctx != NULL, FALSE);
	g_return_val_if_fail(stream->priv->handshake == FALSE, FALSE);

	g_debug("Performing SSL handshake");
	while(1)
	{
		/* establish connection */
		if((result = SSL_do_handshake(stream->priv->ssl)) < 0)
		{
			/* check error code */
			errno = SSL_get_error(stream->priv->ssl, result);
			if(errno != SSL_ERROR_WANT_READ && errno != SSL_ERROR_WANT_WRITE)
			{
				/* handle error code */
				switch(errno)
				{
					case SSL_ERROR_ZERO_RETURN:
						g_set_error(err, 0, errno, "SSL handshake failed: server closed connection");
						break;
					
					case SSL_ERROR_SYSCALL:
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
							g_set_error(err, 0, errno, "SSL handshake failed: server closed connection unexpectedly");
						}
						break;
					
					default:
						if((errstr = ERR_reason_error_string(ERR_get_error())))
						{
							g_set_error(err, 0, errno, "%s", errstr);
						}
						else
						{
							g_set_error(err, 0, errno, "SSL handshake failed: unknown SSL error");
						}
	
				}

				/* leave loop on failure */
				break;
			}
			else
			{
				g_debug("%s: SSL_ERROR_WANT_READ || SSL_ERROR_WANT_WRITE", __func__);
			}
		}
		else
		{
			stream->priv->handshake = TRUE;
			return TRUE;
		}
	}

	return FALSE;
}

static GSocket *
_g_tcp_stream_get_socket(GTcpStream *stream)
{
	g_return_val_if_fail(G_IS_TCP_STREAM(stream), NULL);

	return stream->priv->socket;
}

/*
 *	public:
 */
GSocket
*g_tcp_stream_get_socket(GTcpStream *stream)
{
	return G_TCP_STREAM_GET_CLASS(stream)->get_socket(stream);
}

gboolean
g_tcp_stream_ssl_client_init(GTcpStream *stream, GError **err)
{
	return G_TCP_STREAM_GET_CLASS(stream)->ssl_client_init(stream, err);
}

gboolean
g_tcp_stream_ssl_handshake(GTcpStream *stream, GError **err)
{
	return G_TCP_STREAM_GET_CLASS(stream)->ssl_handshake(stream, err);
}

GTcpStream *
g_tcp_stream_new(GSocket *socket)
{
	return g_object_new(G_TYPE_TCP_STREAM, "socket", socket, "ssl-enabled", FALSE, NULL);
}

GTcpStream *
g_tcp_stream_new_ssl(GSocket *socket)
{
	return g_object_new(G_TYPE_TCP_STREAM, "socket", socket, "ssl-enabled", TRUE, NULL);
}

/*
 *	initialization/finalization:
 */
static void
g_tcp_stream_constructed(GObject *object)
{
	GTcpStream *stream = G_TCP_STREAM(object);

	g_assert(stream->priv->socket != NULL);

	/* initialize openssl */
	if(stream->priv->ssl_enabled)
	{
		g_assert(openssl_init() == TRUE);
	}

}

static void
g_tcp_stream_finalize(GObject *object)
{
	GTcpStream *stream = G_TCP_STREAM(object);

	if(stream->priv->connection)
	{
		/* destroys also input and output stream */
		g_object_unref(stream->priv->connection);
	}
	else
	{
		if(stream->priv->input_stream)
		{
			g_object_unref(stream->priv->input_stream);
		}

		if(stream->priv->output_stream)
		{
			g_object_unref(stream->priv->output_stream);
		}
	}

	/* free allocated SSL resources */
	if(stream->priv->ssl_enabled)
	{
		if(stream->priv->ssl)
		{
			SSL_shutdown(stream->priv->ssl);
			SSL_free(stream->priv->ssl);
		}

		if(stream->priv->ctx)
		{
			SSL_CTX_free(stream->priv->ctx);
		}
	}

	g_object_unref(stream->priv->socket);
	G_OBJECT_CLASS(g_tcp_stream_parent_class)->finalize(object);
}

static void
g_tcp_stream_class_init(GTcpStreamClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GIOStreamClass *stream_class = G_IO_STREAM_CLASS(klass);

	g_type_class_add_private(klass, sizeof(GTcpStreamPrivate));

	gobject_class->set_property = _g_tcp_stream_set_property;
	gobject_class->get_property = _g_tcp_stream_get_property;
	gobject_class->constructed = g_tcp_stream_constructed;
	gobject_class->finalize = g_tcp_stream_finalize;

	stream_class->get_input_stream = _g_tcp_stream_get_input_stream;
	stream_class->get_output_stream = _g_tcp_stream_get_output_stream;
	stream_class->close_fn = _g_tcp_stream_close;
	stream_class->close_async = _g_tcp_stream_close_async;
	stream_class->close_finish = _g_tcp_stream_close_finish;

	klass->get_socket = _g_tcp_stream_get_socket;
	klass->ssl_client_init = _g_tcp_stream_ssl_client_init;
	klass->ssl_handshake = _g_tcp_stream_ssl_handshake;

	g_object_class_install_property(gobject_class, PROP_SOCKET,
	                                g_param_spec_object("socket", NULL, NULL, G_TYPE_SOCKET, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_SSL_ENABLED,
	                                g_param_spec_boolean("ssl-enabled", NULL, NULL, FALSE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
g_tcp_stream_init(GTcpStream *stream)
{
	stream->priv = G_TYPE_INSTANCE_GET_PRIVATE(stream, G_TYPE_TCP_STREAM, GTcpStreamPrivate);
}

/**
 * @}
 */


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
 * \file gsslinputstream.c
 * \brief SSL streaming input operations.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. April 2010
 */

#include "gsslinputstream.h"

/**
 * @addtogroup Net
 * @{
 */

/*! Define GSSLInputStream. */
G_DEFINE_TYPE(GSSLInputStream, g_ssl_input_stream, G_TYPE_INPUT_STREAM);

enum
{
	PROP_0,
	PROP_SSL
};

/**
 * \struct _GSSLInputStreamPrivate
 * \brief Private data.
 */
struct _GSSLInputStreamPrivate
{
	/*! The underlying SSL handle. */
	SSL *ssl;
	/*! A GSimpleAsyncResult.  */
	GSimpleAsyncResult *result;
	/*! A GCancellable. */
	GCancellable *cancellable;
	/*! Points to a string buffer during asynchronous operations. */
	gpointer buffer;
	/*! Holds the buffer size during asynchronous operations. */
	gsize count;
};

/*
 *	implementation:
 */
static void
_g_ssl_input_stream_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GSSLInputStream *stream = G_SSL_INPUT_STREAM(object);

	switch(prop_id)
	{
		case PROP_SSL:
			g_value_set_pointer(value, stream->priv->ssl);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
_g_ssl_input_stream_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GSSLInputStream *stream = G_SSL_INPUT_STREAM(object);

	switch(prop_id)
	{
		case PROP_SSL:
			stream->priv->ssl = g_value_get_pointer(value);
			break;

		default:
			 G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static gssize
_g_ssl_input_stream_read(GInputStream *stream, void *buffer, gsize count, GCancellable *cancellable, GError **err)
{
	return openssl_read((G_SSL_INPUT_STREAM(stream))->priv->ssl, buffer, count, cancellable, err);
}

static gboolean
_g_ssl_input_stream_read_ready(gpointer data)
{
	GSSLInputStream *stream = (GSSLInputStream *)data;
	GSimpleAsyncResult *simple;
	GError *error = NULL;
	gssize result;

	g_assert(stream != NULL);
	g_assert(stream->priv->result != NULL);

	if(!g_source_is_destroyed(g_main_current_source()))
	{
		simple = stream->priv->result;
		stream->priv->result = NULL;

		if((result = openssl_read(stream->priv->ssl, stream->priv->buffer, stream->priv->count, stream->priv->cancellable, &error)) > 0)
		{
			g_simple_async_result_set_op_res_gssize(simple, result);
		}

		if(error)
		{
			g_simple_async_result_set_from_error(simple, error);
			g_error_free(error);
		}

		if(stream->priv->cancellable)
		{
			g_object_unref(stream->priv->cancellable);
		}

		g_simple_async_result_complete(simple);
		g_object_unref(simple);
	}

	return FALSE;
}

static void
_g_ssl_input_stream_read_async(GInputStream *stream, void *buffer, gsize count, gint io_priority,
                              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSSLInputStream *input_stream = G_SSL_INPUT_STREAM(stream);
	GSource *source;

	g_assert(input_stream->priv->result == NULL);

	input_stream->priv->result = g_simple_async_result_new(G_OBJECT(stream), callback, user_data, _g_ssl_input_stream_read_async);

	if(cancellable)
	{
		g_object_ref(cancellable);
	}

	input_stream->priv->cancellable = cancellable;
	input_stream->priv->buffer = buffer;
	input_stream->priv->count = count;

	source = g_timeout_source_new(0);
	g_source_set_callback(source, (GSourceFunc)_g_ssl_input_stream_read_ready, g_object_ref(input_stream), g_object_unref);
	g_source_attach(source, g_main_context_get_thread_default());
	g_source_unref(source);
}

static gssize
_g_ssl_input_stream_read_finish(GInputStream  *stream, GAsyncResult *result, GError **error)
{
	GSimpleAsyncResult *simple;
	gssize count;

	g_return_val_if_fail(G_IS_SSL_INPUT_STREAM(stream), -1);

	simple = G_SIMPLE_ASYNC_RESULT(result);
	g_warn_if_fail(g_simple_async_result_get_source_tag(simple) == _g_ssl_input_stream_read_async);
	count = g_simple_async_result_get_op_res_gssize(simple);

	return count;
}

/*
 *	public:
 */
GSSLInputStream *
g_ssl_input_stream_new(SSL *ssl)
{
	return G_SSL_INPUT_STREAM(g_object_new(G_TYPE_SSL_INPUT_STREAM, "ssl", ssl, NULL));
}

/*
 *	initialization/finalization:
 */
static void
_g_ssl_input_stream_finalize(GObject *object)
{
	if(G_OBJECT_CLASS(g_ssl_input_stream_parent_class)->finalize)
	{
		(*G_OBJECT_CLASS(g_ssl_input_stream_parent_class)->finalize)(object);
	}
}

static void
g_ssl_input_stream_class_init(GSSLInputStreamClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GInputStreamClass *ginputstream_class = G_INPUT_STREAM_CLASS(klass);

	g_type_class_add_private(klass, sizeof(GSSLInputStreamPrivate));

	gobject_class->finalize = _g_ssl_input_stream_finalize;
	gobject_class->get_property = _g_ssl_input_stream_get_property;
	gobject_class->set_property = _g_ssl_input_stream_set_property;

	ginputstream_class->read_fn = _g_ssl_input_stream_read;
	ginputstream_class->read_async = _g_ssl_input_stream_read_async;
	ginputstream_class->read_finish = _g_ssl_input_stream_read_finish;

	g_object_class_install_property(gobject_class, PROP_SSL,
	                                g_param_spec_pointer("ssl", NULL, NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
g_ssl_input_stream_init(GSSLInputStream *stream)
{
	stream->priv = G_TYPE_INSTANCE_GET_PRIVATE(stream, G_TYPE_SSL_INPUT_STREAM, GSSLInputStreamPrivate);
}

/**
 * @}
 */


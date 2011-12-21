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
 * \file httpclient.c
 * \brief An easy to use HTTP client.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 5. December 2010
 */

#include <ctype.h>

#include "httpclient.h"
#include "gtcpstream.h"
#include "netutil.h"

/**
 * @addtogroup Net
 * @{
 * 	@addtogroup Http
 * 	@{
 */

/*! Defines HttpClient. */
G_DEFINE_TYPE(HttpClient, http_client, G_TYPE_OBJECT);

enum
{
	PROP_0,
	PROP_HOSTNAME,
	PROP_PORT,
	PROP_SSL_ENABLED,
	PROP_HEADER_USER_AGENT,
	PROP_HEADER_ACCEPT,
	PROP_HEADER_AUTHORIZATION,
	PROP_HEADERS,
	PROP_STATUS,
	PROP_AUTO_ESCAPE
};

/**
 * \struct _HttpClientPrivate
 * \brief Private _HttpClient data.
 */
struct _HttpClientPrivate
{
	/*! Name of the remote host. */
	gchar *hostname;
	/*! Port of the remote host. */
	gint port;
	/*! Specifies if SSL is enabled. */
	gboolean ssl_enabled;
	/*! User-Agent header. */
	gchar *header_user_agent;
	/*! Authorization header. */
	gchar *header_authorization;
	/*! Accept header. */
	gchar *header_accept;
	/*! The HTTP status code of the response. */
	gint status;
	/*! A TCP stream for reading and writing. */
	GTcpStream *stream;
	/*! The response from the server. */
	gchar *response;
	/*! The length of the response. */
	gint response_length;
	/*! A dictionary holding headers of the response. */
	GHashTable *headers;
	/*! The position of the first byte of the content. */
	gint header_offset;
	/*! Escape post values automatically. */
	gboolean auto_escape;
};

/*
 *	helpers:
 */

/**
 * \param client an HttpClient instance
 *
 * Resets the internal response buffer.
 */
static void
_http_client_free_buffer(HttpClient *client)
{
	if(client->priv->response)
	{
		g_free(client->priv->response);
		client->priv->response = NULL;
	}
	client->priv->response_length = 0;
}

/**
 * \param client an HttpClient instance
 *
 * Frees any memory allocated for stored HTTP headers.
 */
static void
_http_client_free_headers(HttpClient *client)
{
	if(client->priv->headers)
	{
		g_hash_table_destroy(client->priv->headers);
		client->priv->headers = NULL;
	}
	client->priv->header_offset = -1;
}

/**
 * \param client an HttpClient instance
 *
 * Resets internal data. This function is called before sending a request to the remote host.
 */
static void
_http_client_reset(HttpClient *client)
{
	client->priv->status = HTTP_NONE;
	_http_client_free_buffer(client);
	_http_client_free_headers(client);
}

/**
 * \param client an HttpClient instance
 * \param err holds failure messages
 * \return TRUE on success
 *
 * Tries to establish a connection to the specified remote host.
 */
static gboolean
_http_client_connect(HttpClient *client, GError **err)
{
	GSocket *socket;
	gboolean success = FALSE;

	g_return_val_if_fail(client->priv->status == HTTP_NONE, FALSE);
	g_return_val_if_fail(client->priv->stream == NULL, FALSE);
	g_return_val_if_fail(client->priv->hostname != NULL, FALSE);

	/* create socket */
	g_debug("Connecting to remote host: %s", client->priv->hostname);
	if(!(socket = network_util_create_tcp_socket(client->priv->hostname, client->priv->port, err)))
	{
		return FALSE;
	}

	/* create new stream */
	if(client->priv->ssl_enabled)
	{
		if((client->priv->stream = g_tcp_stream_new_ssl(socket)))
		{
			if(g_tcp_stream_ssl_client_init(client->priv->stream, err))
			{
				if(g_tcp_stream_ssl_handshake(client->priv->stream, err))
				{
					success = TRUE;
				}
			}

			if(!success)
			{
				g_object_unref(client->priv->stream);
				client->priv->stream = NULL;
			}
		}

	}
	else
	{
		if((client->priv->stream = g_tcp_stream_new(socket)))
		{
			success = TRUE;
		}
	}

	/* unref socket */
	g_object_unref(socket);

	return success;
}

/**
 * \param client an HttpClient instance
 *
 * Closes an established connection.
 */
static void
_http_client_close(HttpClient *client)
{
	g_return_if_fail(client->priv->stream != NULL);

	g_debug("Closing connection");
	g_io_stream_close(G_IO_STREAM(client->priv->stream), NULL, NULL);
	g_object_unref(client->priv->stream);
	client->priv->stream = NULL;
}

/**
 * \param client an HttpClient instance
 * \return the HTTP status code of the response or HTTP_NONE on failure
 *
 * Extracts the HTTP status code from the response.
 */
static gint
_http_client_get_status_code(HttpClient *client)
{
	gchar * restrict code;
	gint size = 9;
	gint ret;

	g_return_val_if_fail(client->priv->response != NULL, HTTP_NONE);
	g_return_val_if_fail(client->priv->response_length > 9, HTTP_NONE);
	g_return_val_if_fail(g_str_has_prefix(client->priv->response, "HTTP/1.0") == TRUE ||
	                     g_str_has_prefix(client->priv->response, "HTTP/1.1") == TRUE, HTTP_NONE);

	while(size < client->priv->response_length && (g_ascii_isdigit(client->priv->response[size]) ||
	                                               g_ascii_isspace(client->priv->response[size])))
	{
		++size;
	}

	size -= 8;
	code = (gchar *)g_alloca(size);
	g_strlcpy(code, client->priv->response + 9, size - 1);
	ret = atoi(code);

	return (ret == 0) ? HTTP_NONE : ret;
}

/**
 * \param client an HttpClient instance
 *
 * Stores all headers found in the response in a dictionary.
 */
static void
_http_client_parse_headers(HttpClient *client)
{
	gint x = 0;
	gint y = 0;
	GString *line = g_string_new_len(NULL, 64);
	gboolean finished = FALSE;
	GRegex *regex;
	GMatchInfo *match_info;

	g_return_if_fail(client->priv->response != NULL);
	g_return_if_fail(client->priv->response_length > 0);
	g_return_if_fail(client->priv->headers == NULL);
	
	g_debug("Parsing HTTP headers");

	/* compile regex */
	regex = g_regex_new("(?<name>.*):\\s*?(?<value>.*)$", G_REGEX_UNGREEDY, 0, NULL);

	/* create hashtable */
	client->priv->headers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	/* read headers */
	while(!finished)
	{
		if(y <= client->priv->response_length)
		{
			/* detect linebreak */
			if(client->priv->response[y] == '\n')
			{
				/* copy found line into string buffer */
				line = g_string_erase(line, 0, -1);
				while(x < y - 1)
				{
					line = g_string_append_c(line, client->priv->response[x]);
					++x;
				}

				/* parse line */
				if(!g_str_has_prefix(line->str, "HTTP/1"))
				{
					if(g_regex_match(regex, line->str, 0, &match_info))
					{
						if(g_match_info_matches(match_info))
						{
							g_hash_table_insert(client->priv->headers,
							                    g_match_info_fetch_named(match_info, "name"),
							                    g_match_info_fetch_named(match_info, "value"));
						}
						else
						{
							finished = TRUE;
						}
					}

					else
					{
						finished = TRUE;
					}

					/* free match info */
					if(match_info)
					{
						g_match_info_free(match_info);
					}
				}

				x = y + 1;
			}

			++y;
		}
		else
		{
			finished = TRUE;
		}
	}

	client->priv->header_offset = x;

	/* cleanup */
	g_string_free(line, TRUE);
	g_regex_unref(regex);
}

/**
 * \param buffer text to parse
 * \param length length of the buffer
 * \param offset position to start from
 * \return position of the found CRLF or -1
 *
 * Searches for the next line feed in the given buffer.
 */
static gint
_http_client_find_next_crlf(const gchar *buffer, gint length, gint offset)
{
	gint i = offset;

	g_debug("Detecting next CRLF");
	while(i < length + 1)
	{
		if(buffer[i] == 0x0D && buffer[i + 1] == 0x0A)
		{
			g_debug("Offset: %d", i);
			return i;
		}

		++i;
	}


	return -1;
}

/**
 * \param buffer text to parse
 * \param x start position of the header
 * \param y end position of the header
 * \return the converted header
 *
 * Extracts the size of the chunk block from its related header.
 */
static gint
_http_client_parse_chunk_header(const gchar *buffer, gint x, gint y)
{
	gint i = x;
	gchar str[y - x + 1];

	g_debug("Parsing chunk header (x=%d, y=%d)", x, y);
	while(i < y)
	{
		str[i - x] = buffer[i];
		if(!isxdigit(buffer[i]))
		{
			break;
		}

		++i;
	}
	str[i - x] = '\0';
	g_debug("block size=0x%s", str);

	return strtol(str, NULL, 16);
}

/**
 * \param response response of the HTTP server
 * \param length length of the response
 * \param buffer pointer to a string which will hold the merged data
 * \return length of the buffer
 *
 * Merges memory chunks.
 */
static gint
_http_client_handle_chunked_transfer_encoding(const gchar *response, gint length, gchar **buffer)
{
	gint x = 0;
	gint y;
	gint block_size;
	GString *content = g_string_new_len(NULL, 4096);
	gint bytes = 0;
	gboolean finished = FALSE;

	*buffer = NULL;

	while(!finished)
	{
		/* find next line feed */
		if((y = _http_client_find_next_crlf(response, length, x)) != -1)
		{
			/* read chunk header */
			if((block_size = _http_client_parse_chunk_header(response, x, y)) > 0)
			{
				/* block size > 0 => copy data into content buffer */
				x = y + 2;
				content = g_string_append_len(content, response + x, block_size);
				x += block_size + 2;
			}
			else
			{
				finished = TRUE;
			}
		}
		else
		{
			g_warning("Couldn't merge memory blocks");
			finished = TRUE;
		}
	}

	bytes = content->len;
	*buffer = g_string_free(content, FALSE);

	return bytes;
}

/**
 * \param client an HttpClient instance
 * \param request an HTTP request
 * \param err holds failure messages
 * \return TRUE on success
 *
 * Sends an HTTP request to the specified remote host.
 */
static gboolean
_http_client_send_request(HttpClient *client, const gchar *request, GError **err)
{
	GInputStream *in;
	GOutputStream *out;
	gsize bytes;
	gchar buffer[8192];
	GString *response = g_string_new_len(NULL, 8192);
	gboolean ret = FALSE;

	g_return_val_if_fail(client->priv->status == HTTP_NONE, FALSE);
	g_return_val_if_fail(client->priv->stream != NULL, FALSE);

	if((out = g_io_stream_get_output_stream(G_IO_STREAM(client->priv->stream))))
	{
		g_debug("Sending request:\n%s", request);
		if(g_output_stream_write_all(out, request, strlen(request), &bytes, NULL, NULL))
		{
			g_debug("OK, sent %d bytes to host", (gint)bytes);
			g_output_stream_flush(out, NULL, NULL);

			/* read response */
			if((in = g_io_stream_get_input_stream(G_IO_STREAM(client->priv->stream))))
			{
				while((bytes = g_input_stream_read(in, buffer, 8192, NULL, NULL)) > 0)
				{
					client->priv->response_length += bytes;
					g_string_append_len(response, buffer, bytes);
				}

				g_debug("Received %d bytes from host", client->priv->response_length);
				client->priv->response = response->str;
				g_string_free(response, FALSE);
				ret = TRUE;
			}
		}
	}

	return ret;
}

/**
 * \param client an HttpClient instance
 * \param err holds failure messages
 *
 * Parses the reponse buffer.
 */
static void
_http_client_handle_response(HttpClient *client, GError **err)
{
	g_return_if_fail(client->priv->response != NULL);
	g_return_if_fail(client->priv->response_length > 0);

	if((client->priv->status = _http_client_get_status_code(client)) != HTTP_NONE)
	{
		g_debug("HTTP status code: %d", client->priv->status);

		/* read HTTP headers */
		_http_client_parse_headers(client);
	}
	else
	{
		g_set_error(err, 0, 0, "Couldn't fetch HTTP status code");
	}
}


/*
 *	implementation:
 */
static gint
_http_client_get(HttpClient *client, const gchar *path, GError **err)
{
	GString *request = g_string_sized_new(128);

	g_return_val_if_fail(client->priv->hostname != NULL, HTTP_NONE);
	g_return_val_if_fail(client->priv->header_user_agent != NULL, HTTP_NONE);
	g_return_val_if_fail(client->priv->header_accept != NULL, HTTP_NONE);

	/* initialize internal data */
	_http_client_reset(client);

	/* connect to server */
	if(_http_client_connect(client, err))
	{
		/* build request */
		g_string_printf(request, "GET %s HTTP/1.1\n"
		                         "User-Agent: %s\n"
		                         "Host: %s\n"
		                         "Accept: %s\n\n",
		                         path,
		                         client->priv->header_user_agent,
		                         client->priv->hostname,
		                         client->priv->header_accept);

		if(client->priv->header_authorization)
		{
			g_string_append_printf(request, "Authorization: %s\n", client->priv->header_authorization);
		}

		g_string_append_printf(request, "Connection : close\n\n");

		/* handle request */
		if(_http_client_send_request(client, request->str, err))
		{
			/* handle response */
			_http_client_handle_response(client, err);
		}
		
		/* close connection */
		_http_client_close(client);
	}

	/* free memory */
	g_string_free(request, TRUE);
	
	return client->priv->status;
}

static gint
_http_client_post(HttpClient *client, const gchar *path, const gchar * restrict keys[], const gchar * restrict values[], gint argc, GError **err)
{
	GString *params = g_string_sized_new(128);
	GString *request = g_string_sized_new(128);

	g_return_val_if_fail(client->priv->hostname != NULL, HTTP_NONE);
	g_return_val_if_fail(client->priv->header_user_agent != NULL, HTTP_NONE);
	g_return_val_if_fail(client->priv->header_accept != NULL, HTTP_NONE);

	/* initialize internal data */
	_http_client_reset(client);

	/* parse parameters */
	if(argc > 0)
	{
		for(gint i = 0; i < argc; ++i)
		{
			if(i > 0)
			{
				params = g_string_append_c(params, '&');
			}

			if(client->priv->auto_escape)
			{
				params = g_string_append_uri_escaped(params, keys[i], NULL, TRUE);
				params = g_string_append_c(params, '=');
				params = g_string_append_uri_escaped(params, values[i], NULL, TRUE);
			}
			else
			{
				g_string_append_printf(params, "%s=%s", keys[i], values[i]);
			}
		}
	}

	/* connect to server */
	if(_http_client_connect(client, err))
	{
		/* build request */
		g_string_printf(request, "POST %s HTTP/1.1\n"
		                         "User-Agent: %s\n"
		                         "Content-Type: application/x-www-form-urlencoded\n"
		                         "Content-Length: %d\n"
		                         "Host: %s\n"
		                         "Accept: %s\n",
		                         path,
		                         client->priv->header_user_agent,
		                         (gint)params->len,
		                         client->priv->hostname,
		                         client->priv->header_accept);

		if(client->priv->header_authorization)
		{
			g_string_append_printf(request, "Authorization: %s\n", client->priv->header_authorization);
		}

		g_string_append_printf(request, "Connection : close\n\n%s\n", params->str);


//printf("%s", request->str);exit(0);

		/* handle request */
		if(_http_client_send_request(client, request->str, err))
		{
			/* handle response */
			_http_client_handle_response(client, err);
		}

		/* cleanup */
		_http_client_close(client);
	}

	/* free memory allocated for parameters */
	g_string_free(params, TRUE);
	g_string_free(request, TRUE);

	return client->priv->status;
}

static void
_http_client_set_basic_authorization(HttpClient *client, const gchar *username, const gchar *password)
{
	gchar * restrict plain;
	gchar * restrict base64;
	gchar * restrict header;

	g_return_if_fail(username != NULL);
	g_return_if_fail(password != NULL);

	plain = g_strdup_printf("%s:%s", username, password);
	base64 = g_base64_encode((const guchar *)plain, strlen(plain));
	header = g_strdup_printf("Basic %s", base64);

	g_object_set(G_OBJECT(client), "header-authorization", header, NULL);

	g_free(plain);
	g_free(base64);
	g_free(header);
}

static gint
_http_client_get_status(HttpClient *client)
{
	return client->priv->status;
}

static gboolean
_http_client_has_header(HttpClient *client, const gchar *name)
{
	g_return_val_if_fail(client->priv->status != HTTP_NONE, FALSE);

	return (g_hash_table_lookup(client->priv->headers, name) == NULL) ? FALSE : TRUE;
}

static const gchar *
_http_client_lookup_header(HttpClient *client, const gchar *name)
{
	g_return_val_if_fail(client->priv->status != HTTP_NONE, FALSE);
	g_return_val_if_fail(client->priv->headers != NULL, FALSE);

	return (const gchar *)g_hash_table_lookup(client->priv->headers, name);
}

static gint
_http_client_lookup_header_int(HttpClient *client, const gchar *name)
{
	const gchar *value = http_client_lookup_header(client, name);

	if(value)
	{
		return atoi(http_client_lookup_header(client, name));
	}

	return 0;

}

static gint
_http_client_get_content_length(HttpClient *client)
{
	gint length;

	g_return_val_if_fail(client->priv->status != HTTP_NONE, -1);
	g_return_val_if_fail(client->priv->response_length > 0, -1);
	g_return_val_if_fail(client->priv->header_offset != -1, -1);
	g_return_val_if_fail(client->priv->header_offset <= client->priv->response_length, -1);

	if((length = http_client_lookup_header_int(client, "Content-Length")) == 0)
	{
		length = client->priv->response_length - client->priv->header_offset;
	}

	return length;
}

static void
_http_client_read_content(HttpClient *client, gchar **buffer, gint *length)
{
	const gchar * restrict encoding;
	gboolean chunked = FALSE;

	*length = -1;

	g_return_if_fail(client->priv->status != HTTP_NONE);
	g_return_if_fail(client->priv->response != NULL);
	g_return_if_fail(client->priv->response_length > 0);
	g_return_if_fail(client->priv->header_offset != -1);
	g_return_if_fail(client->priv->header_offset <= client->priv->response_length);

	/* check transfer encoding */
	g_debug("Testing HTTP transfer encoding");
	if((encoding = http_client_lookup_header(client, "Transfer-Encoding")))
	{
		g_debug("Transfer-Encoding: %s", encoding);
		if(!g_ascii_strncasecmp(encoding, "chunked", 8))
		{
			chunked = TRUE;
		}
	}

	if(chunked)
	{
		/* merge chunks */
		g_debug("Parsing chunked data");
		*length = _http_client_handle_chunked_transfer_encoding(client->priv->response + client->priv->header_offset,
		                                                        client->priv->response_length - client->priv->header_offset,
		                                                        buffer);
	}
	else
	{
		/* copy data */
		*length = http_client_get_content_length(client);
		*buffer = (gchar *)g_malloc(*length);
		memcpy(*buffer, client->priv->response + client->priv->header_offset, *length);
	}
}

static gboolean
_http_client_dump(HttpClient *client, const gchar *filename, GError **err)
{
	GIOChannel *out;
	gboolean ret = FALSE;

	g_return_val_if_fail(client->priv->status != HTTP_NONE, FALSE);
	g_return_val_if_fail(client->priv->response != NULL, FALSE);
	g_return_val_if_fail(client->priv->response_length > 0, FALSE);
	g_return_val_if_fail(client->priv->header_offset != -1, FALSE);

	if((out = g_io_channel_new_file(filename, "w", err)))
	{
		g_io_channel_set_encoding(out, NULL, NULL);
		if(g_io_channel_write_chars(out, client->priv->response, client->priv->response_length, NULL, err) == G_IO_STATUS_NORMAL)
		{
			ret = TRUE;
		}
		g_io_channel_shutdown(out, TRUE, NULL);
		g_io_channel_unref(out);
	}

	return ret;
}

static void
_http_client_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	HttpClient *client = HTTP_CLIENT(object);

	switch(prop_id)
	{
		case PROP_HOSTNAME:
			g_value_set_string(value, client->priv->hostname);
			break;

		case PROP_PORT:
			g_value_set_int(value, client->priv->port);
			break;

		case PROP_SSL_ENABLED:
			g_value_set_boolean(value, client->priv->ssl_enabled);
			break;

		case PROP_HEADER_USER_AGENT:
			g_value_set_string(value, client->priv->header_user_agent);
			break;

		case PROP_HEADER_ACCEPT:
			g_value_set_string(value, client->priv->header_accept);
			break;

		case PROP_HEADER_AUTHORIZATION:
			g_value_set_string(value, client->priv->header_authorization);
			break;

		case PROP_HEADERS:
			g_value_set_pointer(value, client->priv->headers);
			break;

		case PROP_STATUS:
			g_value_set_int(value, client->priv->status);
			break;

		case PROP_AUTO_ESCAPE:
			g_value_set_boolean(value, client->priv->auto_escape);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
_http_client_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	HttpClient *client = HTTP_CLIENT(object);

	switch(prop_id)
	{
		case PROP_HOSTNAME:
			if(client->priv->hostname)
			{
				g_free(client->priv->hostname);
			}
			client->priv->hostname = g_value_dup_string(value);
			break;

		case PROP_PORT:
			client->priv->port = g_value_get_int(value);
			break;

		case PROP_SSL_ENABLED:
			client->priv->ssl_enabled = g_value_get_boolean(value);
			break;

		case PROP_HEADER_USER_AGENT:
			if(client->priv->header_user_agent)
			{
				g_free(client->priv->header_user_agent);
			}
			client->priv->header_user_agent = g_value_dup_string(value);
			break;

		case PROP_HEADER_ACCEPT:
			if(client->priv->header_accept)
			{
				g_free(client->priv->header_accept);
			}
			client->priv->header_accept = g_value_dup_string(value);
			break;

		case PROP_HEADER_AUTHORIZATION:
			if(client->priv->header_authorization)
			{
				g_free(client->priv->header_authorization);
			}
			client->priv->header_authorization = g_value_dup_string(value);
			break;

		case PROP_AUTO_ESCAPE:
			client->priv->auto_escape = g_value_get_boolean(value);
			break;

		default:
			 G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/*
 *	public:
 */
gint
http_client_get(HttpClient *client, const gchar *path, GError **err)
{
	return HTTP_CLIENT_GET_CLASS(client)->get(client, path, err);
}

gint
http_client_post(HttpClient *client, const gchar *path, const gchar * restrict keys[], const gchar * restrict values[], gint argc, GError **err)
{
	return HTTP_CLIENT_GET_CLASS(client)->post(client, path, keys, values, argc, err);
}

void
http_client_set_basic_authorization(HttpClient *client, const gchar * restrict username, const gchar * restrict password)
{
	return HTTP_CLIENT_GET_CLASS(client)->set_basic_authorization(client, username, password);
}

gint
http_client_get_status(HttpClient *client)
{
	return HTTP_CLIENT_GET_CLASS(client)->get_status(client);
}

gboolean
http_client_has_header(HttpClient *client, const gchar *name)
{
	return HTTP_CLIENT_GET_CLASS(client)->has_header(client, name);
}

const gchar *
http_client_lookup_header(HttpClient *client, const gchar *name)
{
	return HTTP_CLIENT_GET_CLASS(client)->lookup_header(client, name);
}

gint
http_client_lookup_header_int(HttpClient *client, const gchar *name)
{
	return HTTP_CLIENT_GET_CLASS(client)->lookup_header_int(client, name);
}

gint
http_client_get_content_length(HttpClient *client)
{
	return HTTP_CLIENT_GET_CLASS(client)->get_content_length(client);
}

void
http_client_read_content(HttpClient *client, gchar **buffer, gint *length)
{
	HTTP_CLIENT_GET_CLASS(client)->read_content(client, buffer, length);
}

gboolean
http_client_dump(HttpClient *client, const gchar *filename, GError **err)
{
	return HTTP_CLIENT_GET_CLASS(client)->dump(client, filename, err);
}

HttpClient *
http_client_new(const gchar *first_property_name, ...)
{
	va_list ap;
	HttpClient *client;

	va_start(ap, first_property_name);
	client = (HttpClient *)g_object_new_valist(HTTP_CLIENT_TYPE, first_property_name, ap);
	va_end(ap);

	return client;
}

/*
 *	initialization/finalization:
 */
static void
_http_client_finalize(GObject *object)
{
	HttpClient *client = HTTP_CLIENT(object);

	if(client->priv->hostname)
	{
		g_free(client->priv->hostname);
	}

	if(client->priv->header_user_agent)
	{
		g_free(client->priv->header_user_agent);
	}

	if(client->priv->header_accept)
	{
		g_free(client->priv->header_accept);
	}

	if(client->priv->header_authorization)
	{
		g_free(client->priv->header_authorization);
	}

	_http_client_free_buffer(client);
	_http_client_free_headers(client);

	if(G_OBJECT_CLASS(http_client_parent_class)->finalize)
	{
		(*G_OBJECT_CLASS(http_client_parent_class)->finalize)(object);
	}
}

static void
http_client_class_init(HttpClientClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(HttpClientClass));

	gobject_class->finalize = _http_client_finalize;
	gobject_class->get_property = _http_client_get_property;
	gobject_class->set_property = _http_client_set_property;

	klass->get = _http_client_get;
	klass->post = _http_client_post;
	klass->set_basic_authorization = _http_client_set_basic_authorization;
	klass->get_status = _http_client_get_status;
	klass->has_header = _http_client_has_header;
	klass->lookup_header = _http_client_lookup_header;
	klass->lookup_header_int = _http_client_lookup_header_int;
	klass->get_content_length = _http_client_get_content_length;
	klass->read_content = _http_client_read_content;
	klass->dump = _http_client_dump;

	g_object_class_install_property(gobject_class, PROP_HOSTNAME,
	                                g_param_spec_string("hostname", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_PORT,
	                                g_param_spec_int("port", NULL, NULL, 1, 65536, 80, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_SSL_ENABLED,
	                                g_param_spec_boolean("ssl-enabled", NULL, NULL, FALSE, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_HEADER_USER_AGENT,
	                                g_param_spec_string("header-user-agent", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_HEADER_ACCEPT,
	                                g_param_spec_string("header-accept", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_HEADER_AUTHORIZATION,
	                                g_param_spec_string("header-authorization", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_HEADERS,
	                                g_param_spec_pointer("headers", NULL, NULL, G_PARAM_READABLE));
	g_object_class_install_property(gobject_class, PROP_STATUS,
	                                g_param_spec_int("status", NULL, NULL, -1, 600, HTTP_NONE, G_PARAM_READABLE));
	g_object_class_install_property(gobject_class, PROP_AUTO_ESCAPE,
	                                g_param_spec_boolean("auto-escape", NULL, NULL, TRUE, G_PARAM_READWRITE));
}

static void
http_client_init(HttpClient *client)
{
	/* register private data */
	client->priv = G_TYPE_INSTANCE_GET_PRIVATE(client, HTTP_CLIENT_TYPE, HttpClientPrivate);

	/* initialize default HTTP header */
	client->priv->header_user_agent = g_strdup(HTTP_CLIENT_DEFAULT_HEADER_USER_AGENT);
	client->priv->header_accept = g_strdup(HTTP_CLIENT_DEFAULT_HEADER_ACCEPT);
	client->priv->auto_escape = TRUE;

	/* reset internal data */
	_http_client_reset(client);
}

/**
 * @}
 * @}
 */


/***************************************************************************
    begin........: June 2010
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
 * \file twitter_oauth.c
 * \brief Twitter OAuth functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 25. May 2010
 */

#include "twitter_oauth.h"
#include "oauth.h"
#include "../application.h"
#include "../net/httpclient.h"
#include "../net/uri.h"

/**
 * @addtogroup OAuth
 * @{
 */

static HttpClient *
_twitter_oauth_create_http_client(const gchar * restrict scheme, const gchar * restrict hostname)
{
	gint port;
	HttpClient *client = NULL;
	
	port = uri_get_port_from_scheme(scheme);
	if(port == HTTP_DEFAULT_PORT || port == HTTP_DEFAULT_SSL_PORT)
	{
		client = http_client_new("hostname", hostname, "port", port, NULL);
	
		if(port == HTTP_DEFAULT_SSL_PORT)
		{
			g_object_set(G_OBJECT(client), "enable-ssl", NULL);
		}
	}
	else
	{
		g_warning("%s: invalid port for HTTP(S): %d", __func__, port);
	}

	return client;
}

static gint
_twitter_oauth_http_get(const gchar * restrict uri, gchar ** restrict response)
{
	HttpClient *client;
	gchar *scheme = NULL;
	gchar *hostname = NULL;
	gchar *path = NULL;
	GError *err = NULL;
	gint length;
	gint status = HTTP_NONE;

	/* parse uri */
	if(uri_parse(uri, &scheme, &hostname, &path))
	{
		/* send HTTP request */
		if((client = _twitter_oauth_create_http_client(scheme, hostname)))
		{
			if((status = http_client_get(client, path, &err)) == HTTP_OK)
			{
				http_client_read_content(client, response, &length);
			}
			else
			{
				g_warning("Couldn't get response: %d", status);
				if(err)
				{
					g_warning("%s", err->message);
					g_error_free(err);
				}
			}

			/* destroy HttpClient instance */
			g_object_unref(client);
		}
		else
		{
			g_warning("Couldn't create HttpClient instance");
		}
	}
	else
	{
		g_warning("Couldn't parse URI: \"%s\"", uri);
	}

	/* free memory */
	if(scheme)
	{
		g_free(scheme);
	}

	if(hostname)
	{
		g_free(hostname);
	}

	if(path)
	{
		g_free(path);
	}

	return status;
}

static gboolean
_twitter_oauth_parse_reply(const gchar * restrict reply, gchar ** restrict key, gchar ** restrict secret)
{
	gint rc;
	gint i;
	gboolean ret = FALSE;
	gchar **rv = NULL;

	g_return_val_if_fail(key != NULL, FALSE);
	g_return_val_if_fail(secret != NULL, FALSE);

	*key = NULL;
	*secret = NULL;

	rc = oauth_split_url_parameters(reply, &rv);

	if(rc >= 2)
	{
		for(i = 0; i < rc; ++i)
		{
			if(!strncmp(rv[i], "oauth_token=", 12))
			{
				*key = g_strdup(&(rv[i][12]));
				g_debug("%s: found key: \"%s\"", __func__, *key);
			}
			else if(!strncmp(rv[i], "oauth_token_secret=", 19))
			{
				*secret = g_strdup(&(rv[i][19]));
				g_debug("%s: found secret: \"%s\"", __func__, *secret);
			}

			if(*key && *secret)
			{
				ret = TRUE;
				break;
			}

		}
	}
 
 	/* free memory */
	if(rv)
	{
		for(i = 0; i < rc; ++i)
		{
			free(rv[i]);
		}
		free(rv);
	}

	if(!ret)
	{
		if(*key)
		{
			g_free(*key);
		}

		if(*secret)
		{
			g_free(*secret);
		}
	}

	return ret;
}

gboolean
twitter_oauth_request_token(gchar ** restrict request_key, gchar ** restrict request_secret)
{
	gchar *url;
	gchar *response = NULL;
	gboolean ret = FALSE;

	g_return_val_if_fail(request_key != NULL, FALSE);
	g_return_val_if_fail(request_secret != NULL, FALSE);

	url = oauth_sign_url2(TWITTER_REQUEST_TOKEN_URL, NULL, OA_HMAC, NULL, OAUTH_CONSUMER_KEY, OAUTH_CONSUMER_SECRET, NULL, NULL);

	if(_twitter_oauth_http_get(url, &response) == HTTP_OK)
	{
		ret = _twitter_oauth_parse_reply(response, request_key, request_secret);
	}

	/* free memory */
	if(response)
	{
		g_free(response);
	}

	free(url);

	return ret;
}

gchar *
twitter_oauth_authorization_url(const gchar * restrict request_key, const gchar * restrict request_secret)
{
	gchar *ptr;
	gchar *url = NULL;

	if((ptr = oauth_sign_url2(TWITTER_AUTHORIZATION_URL, NULL, OA_HMAC, NULL, OAUTH_CONSUMER_KEY, OAUTH_CONSUMER_SECRET, request_key, request_secret)))
	{
		url = g_strdup(ptr);
		free(ptr);
	}

	return url;
}

gboolean
twitter_oauth_access_token(const gchar * restrict request_key, const gchar * restrict request_secret, const gchar * restrict pin,
                           gchar ** restrict access_key, gchar ** restrict access_secret)
{
	gchar *url = NULL;
	gchar *url2;
	gchar *response = NULL;
	gboolean ret = FALSE;

	url = g_strdup_printf("%s?a=b&oauth_verifier=%s", TWITTER_ACCESS_TOKEN_URL, pin);
	url2 = oauth_sign_url2(url, NULL, OA_HMAC, NULL, OAUTH_CONSUMER_KEY, OAUTH_CONSUMER_SECRET, request_key, request_secret);

	if(_twitter_oauth_http_get(url2, &response) == HTTP_OK)
	{
		ret = _twitter_oauth_parse_reply(response, access_key, access_secret);
	}

	/* free memory */
	if(response)
	{
		g_free(response);
	}

	g_free(url);
	free(url2);

	return ret;
}

/**
 * @}
 */


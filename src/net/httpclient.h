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
 * \file httpclient.h
 * \brief An easy to use HTTP client.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 18. Fedrau 2011
 */

#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include <gio/gio.h>
#include <glib-object.h>

#include "openssl.h"
#include "http.h"

/**
 * @addtogroup Net
 * @{
 * 	@addtogroup Http
 * 	@{
 */

/*! Get the GType. */
#define HTTP_CLIENT_TYPE            (http_client_get_type())
/*! Cast to HttpClient. */
#define HTTP_CLIENT(inst)           (G_TYPE_CHECK_INSTANCE_CAST((inst), HTTP_CLIENT_TYPE, HttpClient))
/*! Cast to HttpClientClass. */
#define HTTP_CLIENT_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST((class), HTTP_CLIENT_TYPE, HttpClientClass))
/*! Checks if instance is HttpClient. */
#define IS_HTTP_CLIENT(inst)        (G_TYPE_CHECK_CLASS_TYPE((inst), HTTP_CLIENT_TYPE))
/*! Get HttpClientClass from HttpClient. */
#define HTTP_CLIENT_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS((inst), HTTP_CLIENT_TYPE, HttpClientClass))

/*! A type definition for _HttpClientPrivate. */
typedef struct _HttpClientPrivate             HttpClientPrivate;

/*! A type definition for _HttpClientClass. */
typedef struct _HttpClientClass               HttpClientClass;

/*! A type definition for _HttpClient. */
typedef struct _HttpClient                    HttpClient;

/*! The default User-Agent header. */
#define HTTP_CLIENT_DEFAULT_HEADER_USER_AGENT "Mozilla/5.0"
/*! The default Accept header. */
#define HTTP_CLIENT_DEFAULT_HEADER_ACCEPT     "text/html, image/jpeg, image/png, image/gif, image/bmp, text/css text/javascript application/rss"

/**
 * \struct _HttpClientClass
 * \brief The _HttpClient class structure.
 *
 * This class provides an easy to use HTTP client. It has the following properties:
 * - \b hostname: name of the remote host (string, rw)\n
 * - \b port: port of the remote host (integer, rw)\n
 * - \b ssl-enabled: activates/deactivate SSL (boolean, rw)\n
 * - \b header-user-agent: the User-Agent header sent in the request (string, rw)\n
 * - \b header-accept: the Accept header sent in the request (string, rw)\n
 * - \b headers: an hashtable containing all headers found in the response (GHashTable, ro)\n
 * - \b status: HTTP status code of the response (integer, ro)
 * - \b auto-escape: escape post values automatically (boolean, rw)
 */
struct _HttpClientClass
{
	/*! The parent class. */
	GObjectClass parent_class;

	/**
	 * \param client HttpClient instance
	 * \param path a path to the content you are interested in
	 * \param err holds failure messages
	 * \return the HTTP status code of the response or HTTP_NONE on failure
	 *
	 * Sends a GET request to a remote host.
	 */
	gint (* get)(HttpClient *client, const gchar *path, GError **err);

	/**
	 * \param client HttpClient instance
	 * \param path a path to the content you are interested in
	 * \param keys an array containing keys
	 * \param values an array containing values assigned to the specified keys
	 * \param argc the number of keys/values
	 * \param err holds failure messages
	 * \return the HTTP status code of the response or HTTP_NONE on failure
	 *
	 * Sends a POST request to a remote host.
	 */
	gint (* post)(HttpClient *client, const gchar *path, const gchar * restrict keys[], const gchar * restrict values[], gint argc, GError **err);

	/**
	 * \param client HttpClient instance
	 * \param username an username
	 * \param password a password related to the specified username
	 *
	 * Sets the Authorization header.
	 */
	void (* set_basic_authorization)(HttpClient *client, const gchar * restrict username, const gchar * restrict password);

	/**
	 * \param client HttpClient instance
	 * \return the status code of the response
	 *
	 * Gets the HTTP status code of the response.
	 */
	gboolean (* get_status)(HttpClient *client);

	/**
	 * \param client HttpClient instance
	 * \param name name of the header to check
	 * \return TRUE if the header does exist
	 *
	 * Checks if the response contains a header with the given name.
	 */
	gboolean (* has_header)(HttpClient *client, const gchar *name);

	/**
	 * \param client HttpClient instance
	 * \param name name of the header to lookup
	 * \return the value of the header with the given name of NULL on failure
	 *
	 * Reads the header with the given name.
	 */
	const gchar *(* lookup_header)(HttpClient *client, const gchar *name);

	/**
	 * \param client HttpClient instance
	 * \param name name of the header to lookup
	 * \return the value of the header with the given name as integer
	 *
	 * Reads the header with the given name and tries to convert it into an integer.
	 */
	gint (* lookup_header_int) (HttpClient *client, const gchar *name);

	/**
	 * \param client HttpClient instance
	 * \return the content length of the response
	 *
	 * Detects the length of the content part of the response.
	 */
	gint (* get_content_length)(HttpClient *client);
	
	/**
	 * \param client HttpClient instance
	 * \param buffer a buffer to store the read content
	 * \param length an integer to store the length of the read content
	 *
	 * Reads the content of the response and converts it into plaintext.
	 */
	void (* read_content)(HttpClient *client, gchar **buffer, gint *length);

	/**
	 * \param client HttpClient instance
	 * \param filename a filename
	 * \param err holds failure messages
	 * \return TRUE on success
	 *
	 * Writes the response (including HTTP headers) to a file.
	 */
	gboolean (* dump)(HttpClient *client, const gchar *filename, GError **err);
};

/**
 * \struct _HttpClient
 * \brief An HTTP client.
 *
 * An easy to use HTTP client. See _HttpClientClass for further information.
 */
struct _HttpClient
{
	/*! The parent instance. */
	GObject parent_instance;
	/*! Private data. */
	HttpClientPrivate *priv;
};

/*! See _HttpClientClass::get() for further information. */
gint http_client_get(HttpClient *client, const gchar *path, GError **err);
/*! See _HttpClientClass::post() for further information. */
gint http_client_post(HttpClient *client, const gchar *path, const gchar * restrict keys[], const gchar * restrict values[], gint argc, GError **err);
/*! See _HttpClientClass::set_basic_authorization() for further information. */
void http_client_set_basic_authorization(HttpClient *client, const gchar * restrict username, const gchar * restrict password);
/*! See _HttpClientClass::get_status() for further information. */
gint http_client_get_status(HttpClient *client);
/*! See _HttpClientClass::has_header() for further information. */
gboolean http_client_has_header(HttpClient *client, const gchar *name);
/*! See _HttpClientClass::lookup_header() for further information. */
const gchar *http_client_lookup_header(HttpClient *client, const gchar *name);
/*! See _HttpClientClass::lookup_header_int() for further information. */
gint http_client_lookup_header_int(HttpClient *client, const gchar *name);
/*! See _HttpClientClass::get_content_length() for further information. */
gint http_client_get_content_length(HttpClient *client);
/*! See _HttpClientClass::read_content() for further information. */
void http_client_read_content(HttpClient *client, gchar **buffer, gint *length);
/*! See _HttpClientClass::dump() for further information. */
gboolean http_client_dump(HttpClient *client, const gchar *filename, GError **err);

/*!
 * \param first_property_name name of the first property to set
 * \param ... property names and values
 * \return a new HttpClient instance or NULL on failure
 *
 * Creates a new HttpClient instance.
 */
HttpClient *http_client_new(const gchar *first_property_name, ...);

/**
 * @}
 * @}
 */
#endif


/***************************************************************************
    begin........: May 2010
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
 * \file uri.c
 * \brief Uri functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 18. May 2010
 */

#include <string.h>

#include "uri.h"
#include "http.h"

/**
 * @addtogroup Net
 * @{
 */

gboolean
uri_parse(const gchar *uri, gchar ** restrict scheme, gchar ** restrict hostname, gchar ** restrict path)
{
	const gchar *ptr;
	gint offset;
	gint length;
	gboolean ret = FALSE;

	g_return_val_if_fail(scheme != NULL, FALSE);
	g_return_val_if_fail(hostname != NULL, FALSE);
	g_return_val_if_fail(path != NULL, FALSE);

	*scheme = NULL;
	*hostname = NULL;
	*path = NULL;

	/* get scheme  */
	if((*scheme = g_uri_parse_scheme(uri)))
	{
		/* extract hostname & path */
		ptr = uri + strlen(*scheme) + 1;
		while(*ptr && *ptr == '/')
		{
			++ptr;
		}

		length = strlen(uri);
		offset = 0;
		while(offset < length && (!(*hostname)))
		{
			if(ptr[offset] == '/')
			{
				*hostname = (gchar *)g_malloc0(offset + 2);
				memcpy(*hostname, ptr, offset);
				*path = g_strdup(ptr + offset);
				ret = TRUE;
				break;
			}
			else
			{
				++offset;
			}
		}

		if(!(*hostname))
		{
			*hostname = (gchar *)g_malloc0(offset + 2);
			memcpy(*hostname, ptr, offset);
			*path = g_strdup("/");
			ret = TRUE;
		}
	}

	/* free memory on failure */
	if(!ret)
	{
		if(*scheme)
		{
			g_free(*scheme);

			if(*hostname)
			{
				g_free(*hostname);
			}

			if(*path)
			{
				g_free(*path);
			}
		}
	}

	return ret;
}

gint
uri_get_port_from_scheme(const gchar *scheme)
{
	if(!g_ascii_strcasecmp(scheme, "https"))
	{
		return HTTP_DEFAULT_SSL_PORT;
	}
	else if(!g_ascii_strcasecmp(scheme, "http"))
	{
		return HTTP_DEFAULT_PORT;
	}

	return -1;
}

/**
 * @}
 */


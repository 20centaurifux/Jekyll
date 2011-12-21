/***************************************************************************
begin........: October 2010
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
* \file twitter.c
* \brief Twitter datatypes and functions.
* \author Sebastian Fedrau <lord-kefir@arcor.de>
* \version 0.1.0
* \date 30. September 2011
*/

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>

#include "twitter.h"
#include "oauth/twitter_oauth.h"

#ifdef G_OS_WIN32
#include <windows.h>
#else
/*! Initialize timezone data. */
void tzset(void);
/*! The local timezone. */
extern long timezone;
#endif

/**
* @addtogroup Core
* @{
* 	@addtogroup Twitter
* 	@{
*/

gboolean
twitter_request_authorization(UrlOpener *urlopener, gchar **request_key, gchar **request_secret, GError **err)
{
	gchar *key = NULL;
	gchar *secret = NULL;
	gchar *authorization_url;
	gboolean success = FALSE;

	g_assert(request_secret != NULL);
	g_assert(request_key != NULL);
	g_assert(err != NULL);

	if(urlopener)
	{
		if(twitter_oauth_request_token(&key, &secret))
		{
			authorization_url = twitter_oauth_authorization_url(key, secret);

			if((success = url_opener_launch(urlopener, authorization_url)))
			{
				*request_key = key;
				*request_secret = secret;
			}
			else
			{
				g_set_error(err, 0, 0, _("Couldn't launch browser, please check your preferences."));
			}

			g_free(authorization_url);
		}
		else
		{
			g_set_error(err, 0, 0, _("Couldn't connect to Twitter service. Please try again later."));
		}

		if(!success)
		{
			if(key)
			{
				g_free(key);
			}

			if(secret)
			{
				g_free(secret);
			}
		}
	}
	else
	{
		g_set_error(err, 0, 0, _("Couldn't launch browser, please check your preferences."));
	}

	return success;
}

gint64
twitter_timestamp_to_unix_timestamp(const gchar *twitter_timestamp)
{
	gchar **arr0 = NULL;
	gchar **arr1 = NULL;
	static gchar *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	gint month = 0;
	struct tm tm;
	gint64 timestamp = 0;
	static gboolean tz_init = FALSE;
	static gint offset = 0;

	/* initialize time structure */
	memset(&tm, 0, sizeof(struct tm));

	/* get timezone offset */
	if(!tz_init)
	{
		g_debug("Reading timezone");
		tz_init = TRUE;

		#ifdef G_OS_WIN32
		TIME_ZONE_INFORMATION tzinfo;

		GetTimeZoneInformation(&tzinfo);
		offset = -(tzinfo.Bias * 60);
		#else
		tzset();
		offset = -timezone;
		#endif
		g_debug("timezone offset: %d", offset);
	}
	
	g_debug("Parsing Twitter timestamp: \"%s\"", twitter_timestamp);

	/* split timestamp */
	if((arr0 = g_strsplit(twitter_timestamp, " ", 6)))
	{
		/* check vector length and length of month string */
		if(g_strv_length(arr0) == 6 && strlen(arr0[1]) == 3)
		{
			/* find month in month array */
			while(g_strcmp0(months[month], arr0[1]) && month < 12)
			{
				++month;
			}
			tm.tm_mon = month;

			/* convert day and year */
			tm.tm_mday = atoi(arr0[2]);
			tm.tm_year = atoi(arr0[5]) - 1900;
			g_debug("Found year, month and year: %d/%d/%d", tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
	
			/* split specified time */
			if((arr1 = g_strsplit(arr0[3], ":", 3)) && g_strv_length(arr1) == 3)
			{
				tm.tm_hour = atoi(arr1[0]);
				tm.tm_min = atoi(arr1[1]);
				tm.tm_sec = atoi(arr1[2]);
				g_debug("Found time: %d:%d:%d", tm.tm_hour, tm.tm_min, tm.tm_sec);

				/* convert time to timestamp */
				timestamp = (gint64)(mktime(&tm) + offset);
				#ifdef G_OS_WIN32
				g_debug("Converted timestamp: %I64d", timestamp);
				#else
				g_debug("Converted timestamp: %lld", timestamp);
				#endif
			}
		}	
	}

	/* free memory */
	if(arr0)
	{
		g_strfreev(arr0);
	}

	if(arr1)
	{
		g_strfreev(arr1);
	}

	if(!timestamp)
	{
		g_warning("Couldn't parse Twitter timestamp: \"%s\"", twitter_timestamp);
	}

	return timestamp;
}

/**
 * @}
 * @}
 */


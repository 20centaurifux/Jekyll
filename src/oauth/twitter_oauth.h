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
 * \file twitter_oauth.h
 * \brief Twitter OAuth functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 25. May 2010
 */

#ifndef __TWITTER_OAUTH_H__
#define __TWITTER_OAUTH_H__

#include <glib.h>

/**
 * @addtogroup OAuth
 * @{
 */

/**
 * \param request_key buffer to store the received key
 * \param request_secret buffer to store the received secret
 * \return TRUE on success
 *
 * Gets a request key and secret.
 */
gboolean twitter_oauth_request_token(gchar ** restrict request_key, gchar ** restrict request_secret);

/**
 * \return a new allocated string
 *
 * Builds a twitter authorization url.
 */
gchar *twitter_oauth_authorization_url(const gchar * restrict request_key, const gchar * restrict request_secret);

/**
 * \param request_key a request key
 * \param request_secret a request secret
 * \param pin a generated pin
 * \param access_key buffer to store the received key
 * \param access_secret buffer to store the received secret
 * \return TRUE on success
 *
 * Gets an access key and secret.
 */
gboolean twitter_oauth_access_token(const gchar * restrict request_key, const gchar * restrict request_secret,
                                    const gchar * restrict pin, gchar ** restrict access_key, gchar ** restrict access_secret);

/**
 * @}
 */
#endif


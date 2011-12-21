/***************************************************************************
    begin........: August 2010
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
 * \file twitterclient_factory.h
 * \brief A TwitterClient factory.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 2. September 2011
 */
#ifndef __TWITTERCLIENT_FACTORY_H__
#define __TWITTERCLIENT_FACTORY_H__

#include "twitterclient.h"
#include "configuration.h"

/**
 * \param config a Configuration instance
 * \param cache a cache instance
 * \param lifetime cache lifetime
 * \return a new TwitterClient instance
 *
 * Creates a TwitterClient instance from configuration data.
 */
TwitterClient *twitterclient_new_from_config(Config *config, Cache *cache, gint lifetime);

#endif


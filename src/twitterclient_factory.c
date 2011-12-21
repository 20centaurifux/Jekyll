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
 * \file twitterclient_factory.c
 * \brief A TwitterClient factory.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 2. September 2011
 */

#include "twitterclient_factory.h"

/*
 *	helpers:
 */
static Section *
_twitterclient_factory_get_accounts_section(Config *config, gint *count)
{
	Section *section;
	Section *accounts = NULL;

	/* get root section from configuration */
	section = config_get_root(config);

	/* find "Accounts" section */
	if((section = section_find_first_child(section, "Twitter")) && (section = section_find_first_child(section, "Accounts")))
	{
		*count = section_n_children(section);
		accounts = section;
	}

	return accounts;
}
static void
_twitterclient_factory_add_accounts(TwitterClient *client, Config *config)
{
	Section *section;
	Section *child;
	Value *value;
	const gchar *username;
	const gchar *access_key;
	const gchar *access_secret;
	gint count;

	if((section = _twitterclient_factory_get_accounts_section(config, &count)))
	{
		for(gint i = 0; i < count; ++i)
		{
			child = section_nth_child(section, i);
			username = NULL;
			access_key = NULL;
			access_secret = NULL;

			if(!g_ascii_strcasecmp(section_get_name(child), "Account"))
			{
				if((value = section_find_first_value(child, "username")) && VALUE_IS_STRING(value))
				{
					username = value_get_string(value);
				}

				if((value = section_find_first_value(child, "oauth_access_key")) && VALUE_IS_STRING(value))
				{
					access_key = value_get_string(value);
				}

				if((value = section_find_first_value(child, "oauth_access_secret")) && VALUE_IS_STRING(value))
				{
					access_secret = value_get_string(value);
				}
			}

			if(username && access_key && access_secret)
			{
				g_debug("Adding account \"%s\" to TwitterClient instance", username);
				twitter_client_add_account(client, username, access_key, access_secret);
			}
			else
			{
				g_warning("%s: incomplete account data, please check your configuration", __func__);
			}
		}
	}
}

/*
 *	public:
 */
TwitterClient *
twitterclient_new_from_config(Config *config, Cache *cache, gint lifetime)
{
	TwitterClient *client;

	client = twitter_client_new(cache, lifetime);
	_twitterclient_factory_add_accounts(client, config);

	return client;
}


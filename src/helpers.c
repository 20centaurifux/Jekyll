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
 * \file helpers.c
 * \brief Helper functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 26. October 2010
 */

#include "urlopener.h"
#include "configuration.h"

/**
 * @addtogroup Core 
 * @{
 */

UrlOpener *
helpers_create_urlopener_from_config(Config *config)
{
	Section *section;
	Value *value;
	const gchar *filename;
	UrlOpener *urlopener = NULL;

	g_debug("Creating UrlOpener instance from configuration data");

	if((section = section_find_first_child(config_get_root(config), "Browser")))
	{
		if((value = section_find_first_value(section, "custom")) && VALUE_IS_BOOLEAN(value) && value_get_bool(value))
		{
			g_debug("Using custom browser, searching for executable");

			if((value = section_find_first_value(section, "executable")) && VALUE_IS_STRING(value))
			{
				if((filename = value_get_string(value)))
				{
					if(g_file_test(filename, G_FILE_TEST_IS_EXECUTABLE))
					{
						urlopener = url_opener_new_executable(filename, NULL);
					}
					else
					{
						g_warning("%s: specified file is not executable", __func__);
					}
				}
				else
				{
					g_warning("%s: \"Browser::executable\" is empty", __func__);
				}
			}
			else
			{
				g_warning("%s: couldn't find value \"Browser::executable\"", __func__);
			}
		}
	}
	else
	{
		g_debug("%s: couldn't find browser section", __func__);
	}

	if(!urlopener)
	{
		g_debug("Creating default urlopener");

		if(!(urlopener = url_opener_new_default()))
		{
			g_warning("%s: couldn't create default UrlOpener instance", __func__);
		}
	}

	return urlopener;
}

/**
 * @}
 */


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
 * \file settings.c
 * \brief Loading and saving configuration settings.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 14. October 2011
 */

#include "settings.h"
#include "pathbuilder.h"

/**
 * @addtogroup Core
 * @{
 */

/*
 *	helpers:
 */
static void
_settings_set_default_value(Section *section, const gchar *name, gpointer value, ValueDatatype type, gboolean overwrite)
{
	Value *val;
	gboolean write = FALSE;

	if(!(val = section_find_first_value(section, name)))
	{
		val = section_append_value(section, name, type);
		write = TRUE;
	}

	if(write || overwrite)
	{
		switch(type)
		{
			case VALUE_DATATYPE_BOOLEAN:
				value_set_bool(val, GPOINTER_TO_INT(value));
				break;

			case VALUE_DATATYPE_INT32:
				value_set_int32(val, GPOINTER_TO_INT(value));
				break;

			default:
				g_warning("%s: invalid value type", __func__);
		}
	}
}

static void
_settings_set_default_bool(Section *section, const gchar *name, gboolean value, gboolean overwrite)
{
	_settings_set_default_value(section, name, GINT_TO_POINTER(value), VALUE_DATATYPE_BOOLEAN, overwrite);
}

static void
_settings_set_default_int32(Section *section, const gchar *name, gint32 value, gboolean overwrite)
{
	_settings_set_default_value(section, name, GINT_TO_POINTER(value), VALUE_DATATYPE_INT32, overwrite);
}

/*
 *	public:
 */
gboolean
settings_check_directory(void)
{
	const gchar *path;
	gboolean result = FALSE;

	path = pathbuilder_get_user_application_directory();
	g_debug("Testing \"%s\"", path);
	if(!(result = g_file_test(path, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS)))
	{
		g_debug("Configuration file doesn't exist, will try to create it");
		if(!g_mkdir_with_parents(path, 500))
		{
			result = TRUE;
		}
		else
		{
			g_warning("Couldn't create directory: %s", path);
		}
	}

	return result;
}

Config *
settings_load_config(void)
{
	const gchar *filename;
	Config *config = NULL;

	filename = pathbuilder_get_config_filename();
	g_debug("Testing \"%s\"", filename);
	if(!g_file_test(filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
	{
		g_debug("Configuration file doesn't exist");
	}
	else
	{
		config = settings_load_config_from_file(filename);
	}

	return config;
}

Config *
settings_load_config_from_file(const gchar *filename)
{
	GError *err = NULL;
	Config *config;

	config = config_new();

	if(!config_load(config, filename, &err))
	{
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
		else
		{
			g_warning("Couldn't load configuration file, an unknown error occured");
		}

		config_unref(config);
		config = NULL;
	}

	return config;
}

gboolean
settings_config_has_account(Config *config)
{
	Section * section;
	gboolean has_account = FALSE;

	g_return_val_if_fail(config != NULL, FALSE);

	if((section = section_find_first_child(config_get_root(config), "Twitter")))
	{
		if((section = section_find_first_child(section, "Accounts")))
		{
			if(section_find_first_child(section, "Account"))
			{
				has_account = TRUE;
			}
		}
	}

	return has_account;
}

void
settings_set_default_settings(Config *config, gboolean overwrite)
{
	Section *root;
	Section *section;

	root = config_get_root(config);

	/* account initialization */
	if(!(section = section_find_first_child(root, "Global")))
	{
		section = section_append_child(root, "Global");
	}

	_settings_set_default_bool(section, "first-account-initialized", FALSE, overwrite);

	/* window preferences */
	if(!(section = section_find_first_child(root, "Window")))
	{
		section = section_append_child(root, "Window");
	}

	_settings_set_default_bool(section, "enable-systray", TRUE, overwrite);
	_settings_set_default_bool(section, "start-minimized", FALSE, overwrite);
	_settings_set_default_bool(section, "restore-window", TRUE, overwrite);


	/* view preferences */
	if(!(section = section_find_first_child(root, "View")))
	{
		section = section_append_child(root, "View");
	}

	_settings_set_default_bool(section, "show-notification-area", TRUE, overwrite);
	_settings_set_default_int32(section, "notification-area-level", 1, overwrite);
}

/**
 * @}
 */


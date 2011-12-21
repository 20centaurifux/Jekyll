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
 * \file gui.c
 * \brief GUI initialization.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 16. September 2011
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gui.h"
#include "wizard.h"
#include "mainwindow.h"
#include "first_sync_dialog.h"
#include "../settings.h"
#include "../pathbuilder.h"
#include "../database_init.h"
#include "../application.h"

/**
 * @addtogroup Gui
 * @{
 */

/*
 *	helpers:
 */
static gboolean
_gui_get_first_account(Config *config, gchar **username, gchar **key, gchar **secret)
{
	Section *section;
	Section *child;
	Value *value;
	gint count;
	gint result = 0;
	gboolean success = FALSE;

	g_debug("Getting account data from configuration...");

	*username = NULL;
	*key = NULL;
	*secret = NULL;

	/* get root section from configuration */
	section = config_get_root(config);

	/* find "Accounts" section */
	if((section = section_find_first_child(section, "Twitter")) && (section = section_find_first_child(section, "Accounts")))
	{
		count = section_n_children(section);

		/* set account data */
		for(gint i = 0; i < count && !success; ++i)
		{
			child = section_nth_child(section, i);

			if(!g_ascii_strcasecmp(section_get_name(child), "Account"))
			{
				++result;
			
				if((value = section_find_first_value(child, "username")) && VALUE_IS_STRING(value))
				{
					*username = g_strdup(value_get_string(value));
					g_debug("Found username: \"%s\"", *username);
				}

				if((value = section_find_first_value(child, "oauth_access_key")) && VALUE_IS_STRING(value))
				{
					*key = g_strdup(value_get_string(value));
					g_debug("Found username: \"%s\"", *key);
				}

				if((value = section_find_first_value(child, "oauth_access_secret")) && VALUE_IS_STRING(value))
				{
					*secret = g_strdup(value_get_string(value));
					g_debug("Found username: \"%s\"", *secret);
				}
			}

			if(*username && *key && *secret)
			{
				success = TRUE;
			}
		}
	}

	if(!success)
	{
		g_free(*username);
		g_free(*key);
		g_free(*secret);
	}

	return success;
}

static gboolean
_gui_initialize_account(Config *config)
{
	gchar *username = NULL;
	gchar *access_key = NULL;
	gchar *access_secret = NULL;
	gboolean success = FALSE;

	if(_gui_get_first_account(config, &username, &access_key, &access_secret))
	{
		success = first_sync_dialog_run(NULL, username, access_key, access_secret);
	}
	else
	{
		g_error("Couldn't get initial user account.");
	}

	g_free(username);
	g_free(access_key);
	g_free(access_secret);

	return success;
}

static gboolean
_gui_check_account_initialization(Config *config)
{
	Section *root;
	Section *section;
	Value *value;
	gboolean result = FALSE;

	root = config_get_root(config);

	if((section = section_find_first_child(root, "Global")))
	{
		if((value  = section_find_first_value(section, "first-account-initialized")) && VALUE_IS_BOOLEAN(value))
		{
			result = value_get_bool(value);
		}
	}

	return result;
}

static void
_gui_enable_account_initialization(Config *config)
{
	Section *root;
	Section *section;
	Value *value;

	root = config_get_root(config);

	if(!(section = section_find_first_child(root, "Global")))
	{
		section = section_append_child(root, "Global");
	}

	if(!(value = section_find_first_value(section, "first-account-initialized")))
	{
		value = section_append_value(section, "first-account-initialization", VALUE_DATATYPE_BOOLEAN);
	}

	value_set_bool(value, TRUE);
}

static void
_gui_save_config(Config *config)
{
	GError *err = NULL;

	if(config_get_filename(config))
	{
		g_debug("Saving configuration");
		config_save(config, &err);
	}
	else
	{
		g_debug("Saving configuration: \"%s\"", pathbuilder_get_config_filename());
		config_save_as(config, pathbuilder_get_config_filename(), &err);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
		err = NULL;
	}
}

/*
 *	public:
 */
void
gui_start(Config *config, Cache *cache)
{
	gboolean start_app = FALSE;
	gboolean database_is_valid = FALSE;
	gboolean account_is_initialized = FALSE;
	GtkWidget *dialog = NULL;
	GError *err = NULL;

	g_assert(config != NULL);
	g_assert(cache != NULL);

	g_debug("Starting GUI...");

	#ifdef G_OS_WIN32
	gchar *path;

	/* load windows theme */
	g_debug("Parsing MS-Windows theme file");
	path = g_build_filename(pathbuilder_get_share_directory(),
                                G_DIR_SEPARATOR_S,
                                "themes",
                                G_DIR_SEPARATOR_S,
                                "MS-Windows",
                                G_DIR_SEPARATOR_S,
                                "gtk-2.0",
                                G_DIR_SEPARATOR_S,
                                "gtkrc",
                                NULL);

	g_debug("filename: %s", path);
	gtk_rc_parse(path);

	g_free(path);
	#endif

	/* check if configuration is empty */
	if(!(start_app = settings_config_has_account(config)))
	{
		/* configuration is empty => start wizard */
		g_debug("Configuration doesn't contain at least one account, starting wizard");
		if((start_app = wizard_start(config)))
		{
			_gui_save_config(config);
		}
	}

	if(start_app)
	{
		/* initialize database */
		g_debug("Initializing database...");
		switch(database_init(&err))
		{
			case DATABASE_INIT_FAILURE:
				dialog = gtk_message_dialog_new(NULL,
		                                        GTK_DIALOG_MODAL,
		                                        GTK_MESSAGE_WARNING,
		                                        GTK_BUTTONS_OK,
		                                        _("Couldn't initialize %s, database seems to be damaged."), APPLICATION_NAME);
				break;

			case DATABASE_INIT_APPLICATION_OUTDATED:
				dialog = gtk_message_dialog_new(NULL,
		                                        GTK_DIALOG_MODAL,
		                                        GTK_MESSAGE_WARNING,
		                                        GTK_BUTTONS_OK,
		                                        _("Couldn't initialize database, please upgrade %s."), APPLICATION_NAME);
				break;

			case DATABASE_INIT_FIRST_INITIALIZATION:
			case DATABASE_INIT_SUCCESS:
			case DATABASE_INIT_DATABASE_UPGRADED:
				database_is_valid = TRUE;
				break;

			default:
				g_warning("Invalid database result code.");
				break;
		}

		/* display failure messages */
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
			err = NULL;
		}

		if(database_is_valid)
		{
			/* initialize first account (if neccessary) */
			g_debug("Testing account initialization status...");
			if(!(account_is_initialized = _gui_check_account_initialization(config)))
			{
				g_debug("Starting account initialization");
				if((account_is_initialized = _gui_initialize_account(config)))
				{
					g_debug("account has been initialized successfully => updating configuration");
					_gui_enable_account_initialization(config);
					_gui_save_config(config);
				}
				else
				{
					dialog = gtk_message_dialog_new(NULL,
		                                                        GTK_DIALOG_MODAL,
		                                                        GTK_MESSAGE_WARNING,
		                                                        GTK_BUTTONS_OK,
		                                                        _("Account initialization failed, please try again later."));
				}
			}

			/* open mainwindow */
			if(account_is_initialized)
			{
				g_debug("Opening mainwindow");
				mainwindow_start(config, cache);
			}
		}

		/* show warning dialog */
		if(dialog)
		{
			gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		}
	}
}

/**
 * @}
 */


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
 * \file main.c
 * \brief Startup.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2011
 */

#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "options.h"
#include "application.h"
#include "settings.h"
#include "pathbuilder.h"
#include "cache.h"
#include "listener.h"
#include "gui/gui.h"

/**
 * @addtogroup Core 
 * @{
 */

/**
 * \enum InitResult
 * \brief Describes an initialization status.
 */
typedef enum
{
	/*! Abort startup and return a failure code. */
	INIT_ABORT,
	/*! Continue startup. */
	INIT_CONTINUE,
	/*! Abort startup. */
	INIT_QUIT
} InitResult;

static void
_g_log_consume_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	return;
}

static void
_print_version()
{
	g_print("%s - %s version %d.%d.%d\n", APPLICATION_NAME, APPLICATION_DESCRIPTION, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH_LEVEL);
}

static InitResult
_init_handle_options(int *argc, char **argv[], Options *options)
{

	if(!options_parse(argc, argv, options))
	{
		return INIT_ABORT;
	}

	/* show version and quit */
	if(options->version)
	{
		_print_version();
		return INIT_QUIT;
	}

	/* enable memory profiling */
	if(options->enable_mem_profile)
	{
		g_mem_set_vtable(glib_mem_profiler_table);
	}

	/* don't show debug messages */
	if(!options->enable_debug)
	{
		g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, _g_log_consume_handler, NULL);
	}

	return INIT_CONTINUE;
}

static void
_init_store_startup_options(Config *config, Options options)
{
	Section *root;
	Section *section;
	Value *value;

	root = config_get_root(config);

	if(!(section = section_find_first_child(root, "Startup")))
	{
		section = section_append_child(root, "Startup");
	}

	if(!(value = section_find_first_value(section, "enable-debug")))
	{
		value = section_append_value(section, "enable-debug", VALUE_DATATYPE_BOOLEAN);
	}

	value_set_bool(value, options.enable_debug ? TRUE : FALSE);

	if(!(value = section_find_first_value(section, "no-sync")))
	{
		value = section_append_value(section, "no-sync", VALUE_DATATYPE_BOOLEAN);
	}

	value_set_bool(value, options.no_sync ? TRUE : FALSE);
}

static InitResult
_init_initialize_libs(int *argc, char **argv[])
{
	/* initialize GThread */
	g_debug("Initializing GThread");
	g_thread_init(NULL);

	if(!g_thread_supported())
	{
		g_critical("GThread is not supported");
		return INIT_ABORT;
	}

	gdk_threads_init();

	/* initialize GTK */
	g_debug("Initializing GTK %d.%d.%d", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
	if(!gtk_init_check(argc, argv))
	{
		g_critical("Couldn't initialize GTK");
		return INIT_ABORT;
	}

	return INIT_CONTINUE;
}

static void
_init_gettext(void)
{
	g_debug("Initializing GNU gettext");
	setlocale(LC_ALL, NULL);
	bindtextdomain(GETTEXT_PACKAGE_NAME, pathbuilder_get_locale_directory());
	bind_textdomain_codeset(GETTEXT_PACKAGE_NAME, "UTF-8");

	#ifdef G_OS_WIN32
	bindtextdomain("gtk20", pathbuilder_get_locale_directory());
	bind_textdomain_codeset("gtk20", "UTF-8");
	#endif

	textdomain(GETTEXT_PACKAGE_NAME);
}

static Cache *
_init_create_cache(void)
{
	Cache *cache;
	gchar *path;

	g_debug("Creating cache");
	path = g_build_filename(pathbuilder_get_user_application_directory(), G_DIR_SEPARATOR_S, CACHE_SWAP_FOLDER, NULL);
	cache = cache_new(CACHE_LIMIT, TRUE, path);
	g_free(path);

	return cache;
}

/**
 * \param argc number of arguments
 * \param argv specified arguments
 *
 * Starts the application.
 */
int
main(int argc, char *argv[])
{
	InitResult result;
	Options options;
	Config *config = NULL;
	Cache *cache = NULL;
	GError *err = NULL;

	/*
	 *	INITIALIZATION:
	 */
	/* handle command line options */
	if((result = _init_handle_options(&argc, &argv, &options)) != INIT_CONTINUE)
	{
		return (result == INIT_QUIT) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	/* initialize libraries */
	if(_init_initialize_libs(&argc, &argv) != INIT_CONTINUE)
	{
		return EXIT_FAILURE;
	}

	/* initialize pathbuilder */
	pathbuilder_init();

	/* initialize gettext */
	_init_gettext();

	/* check if configuration directory can be accessed */
	if(settings_check_directory())
	{
		/* start listener */
		g_debug("Initializing listener");
		switch(listener_init())
		{
			case LISTENER_RESULT_OK:
				g_debug("Listener created successfully");
				break;

			case LISTENER_RESULT_ABORT:
				return EXIT_FAILURE;

			case LISTENER_RESULT_FOUND_INSTANCE:
				g_debug("Found existing instance");
				g_debug("Sending message to instance and shutting down");
				listener_send_message(LISTENER_MESSAGE_ACTIVATE_INSTANCE, NULL);
		
				return EXIT_SUCCESS;

			default:
				g_error("Invalid listener result");
		}

		/* create cache */
		cache = _init_create_cache();

		/* load configuration */
		if(options.config_filename)
		{
			/* load configuration from file specified in command line option */
			g_debug("Loading configuration from \"%s\"", options.config_filename);
			if((config = settings_load_config_from_file(options.config_filename)))
			{
				/* check if configuration contains at least one account */
				if(!settings_config_has_account(config))
				{
					g_debug("Couldn't find at least one account in the specified configuration file");
					config_unref(config);
					config = NULL;
				}
			}
		}
		else
		{
			/* try to load configuration from file */
			g_debug("Loading configuration");
			if(!(config = settings_load_config()))
			{
				/* create empty configuration */
				g_debug("Couldn't read configuration, creating an empty one");
				config = config_new();
			}
		}

		if(config)
		{
			/*
			 *	START GUI:
			 */
			/* store startup options */
			_init_store_startup_options(config, options);

			/* start GUI */
			settings_set_default_settings(config, FALSE);
			gui_start(config, cache);

			g_debug("GUI closed, shutting down...");

			/*
			 *	SHUTDOWN:
			 */
			/* save configuration & destroy it */
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
			}

			config_unref(config);
		}
		else
		{
			g_critical("Couldn't find a valid configuration, aborting...");
		}

		/* destroy cache & shutdown listener*/
		g_object_unref(G_OBJECT(cache));
		listener_shutdown();
	}
	else
	{
		g_warning("Couldn't access configuration directory.");
	}

	/* free memory allocated by pathbuilder */
	pathbuilder_free();

	g_debug("Shutdown completed successfully");

	/* print memory usage */
	if(options.enable_mem_profile)
	{
		g_mem_profile();
	}

	return EXIT_SUCCESS;
}

/**
 * @}
 */


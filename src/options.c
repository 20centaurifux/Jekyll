/***************************************************************************
    begin........: April 2010
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
 * \file options.c
 * \brief Parsing command line options.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 17. June 2010
 */

#include <stdio.h>
#include <string.h>

#include "options.h"
#include "application.h"

/**
 * @addtogroup Core 
 * @{
 */

static Options options_args;

static GOptionEntry entries[] = 
{
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &options_args.version, "Shows the version number and quits", NULL },
	{ "config", 0, 0, G_OPTION_ARG_FILENAME, &options_args.config_filename, "Configuration file", "filename" },
	{ "no-sync", 0, 0, G_OPTION_ARG_NONE, &options_args.no_sync, "Do not synchronize Jekyll (useful when debugging)", NULL },
	{ "enable-debug", 0, 0, G_OPTION_ARG_NONE, &options_args.enable_debug, "Show debug messages", NULL },
	{ "enable-mem-profile", 0, 0, G_OPTION_ARG_NONE, &options_args.enable_mem_profile, "Outputs a summary of memory usage on exit", NULL },
	{ NULL }
};

gboolean
options_parse(gint *argc, gchar ***argv, Options *options)
{
	gchar *title;
	GError *error = NULL;
	GOptionContext *context;

	title = (gchar *)g_alloca(strlen(APPLICATION_NAME) + 3);
	sprintf(title, "- %s", APPLICATION_NAME);

	context = g_option_context_new(title);
	g_option_context_add_main_entries(context, entries, NULL);

	if(!g_option_context_parse(context, argc, argv, &error))
	{
		g_print("option parsing failed: %s\n", error->message);
		return FALSE;
	}

	*options = options_args;

	return TRUE;
}

/**
 * @}
 */


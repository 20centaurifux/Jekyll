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
 * \file pathbuilder.c
 * \brief Building filenames.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 18. December 2011
 */

#include "pathbuilder.h"
#include "application.h"

/*
 * @addtogroup Core
 * @{
 */

static gboolean initialized = FALSE;
static GHashTable *pathbuilder_table;
static GMutex *pathbuilder_mutex;

static void
_pathbuilder_free_path(gpointer data)
{
	g_debug("Removing path from cache: \"%s\"", (gchar *)data);
	g_free(data);
}

void
pathbuilder_init(void)
{
	g_return_if_fail(initialized == FALSE);

	g_debug("Initializing pathbuilder");
	initialized = TRUE;
	pathbuilder_mutex = g_mutex_new();
	pathbuilder_table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, _pathbuilder_free_path);

	#ifdef G_OS_WIN32
	gchar *install_dir;

	/* get installation directory */
	install_dir = g_win32_get_package_installation_directory_of_module(NULL);

	/* create table */
	g_hash_table_insert(pathbuilder_table,
	                   (gpointer)"user_app_dir",
                           (gpointer)g_build_filename(g_get_user_config_dir(), G_DIR_SEPARATOR_S, CONFIG_DIR, NULL));

	g_hash_table_insert(pathbuilder_table,
	                   (gpointer)"config_filename",
                           (gpointer)g_build_filename(g_get_user_config_dir(), G_DIR_SEPARATOR_S, CONFIG_DIR, G_DIR_SEPARATOR_S, CONFIG_FILENAME, NULL));

	g_hash_table_insert(pathbuilder_table,
	                   (gpointer)"share_dir",
                           (gpointer)g_build_filename(install_dir, G_DIR_SEPARATOR_S, "share", NULL));

	g_hash_table_insert(pathbuilder_table,
	                   (gpointer)"locale_dir",
                           (gpointer)g_build_filename(install_dir, G_DIR_SEPARATOR_S, "locale", NULL));

	/* free memory  */
	g_free(install_dir);
	#else
	g_hash_table_insert(pathbuilder_table,
	                   (gpointer)"user_app_dir",
                           (gpointer)g_build_filename(g_get_home_dir(), G_DIR_SEPARATOR_S, CONFIG_DIR, NULL));

	g_hash_table_insert(pathbuilder_table,
	                   (gpointer)"config_filename",
                           (gpointer)g_build_filename(g_get_home_dir(), G_DIR_SEPARATOR_S, CONFIG_DIR, G_DIR_SEPARATOR_S, CONFIG_FILENAME, NULL));

	g_hash_table_insert(pathbuilder_table,
	                   (gpointer)"share_dir",
                           (gpointer)g_build_filename(DATAROOTDIR, G_DIR_SEPARATOR_S, "jekyll", NULL));

	g_hash_table_insert(pathbuilder_table,
	                   (gpointer)"locale_dir",
                           (gpointer)g_strdup(LOCALEDIR));
	#endif
}

void
pathbuilder_free(void)
{
	g_return_if_fail(initialized == TRUE);

	g_debug("Freeing memory allocated by pathbuilder");
	initialized = FALSE;
	g_hash_table_destroy(pathbuilder_table);
	g_mutex_free(pathbuilder_mutex);
}

const gchar *
pathbuilder_get_user_application_directory(void)
{
	g_return_val_if_fail(initialized == TRUE, NULL);

	return pathbuilder_load_path("user_app_dir");
}

const gchar *
pathbuilder_get_config_filename(void)
{
	g_return_val_if_fail(initialized == TRUE, NULL);

	return pathbuilder_load_path("config_filename");
}

const gchar *
pathbuilder_get_share_directory(void)
{
	g_return_val_if_fail(initialized == TRUE, NULL);

	return pathbuilder_load_path("share_dir");
}

const gchar *
pathbuilder_get_locale_directory(void)
{
	g_return_val_if_fail(initialized == TRUE, NULL);

	return pathbuilder_load_path("locale_dir");
}

gchar *
pathbuilder_build_image_path(const gchar *image)
{
	g_return_val_if_fail(initialized == TRUE, NULL);

	return g_build_filename(pathbuilder_get_share_directory(), G_DIR_SEPARATOR_S, "images", G_DIR_SEPARATOR_S, image, NULL);
}

gchar *
pathbuilder_build_icon_path(const gchar * restrict size, const gchar * restrict image)
{
	g_return_val_if_fail(initialized == TRUE, NULL);

	return g_build_filename(pathbuilder_get_share_directory(),
	                        G_DIR_SEPARATOR_S,
	                        "icons",
	                        G_DIR_SEPARATOR_S,
	                        size,
	                        G_DIR_SEPARATOR_S,
	                        image,
	                        NULL);
}

void
pathbuilder_save_path(const gchar * restrict key, const gchar * restrict path)
{
	g_return_if_fail(initialized == TRUE);
	g_return_if_fail(g_ascii_strcasecmp(key, "user_app_dir") != 0);
	g_return_if_fail(g_ascii_strcasecmp(key, "config_filename") != 0);
	g_return_if_fail(g_ascii_strcasecmp(key, "share_dir") != 0);

	g_mutex_lock(pathbuilder_mutex);
	if(g_hash_table_lookup(pathbuilder_table, (gpointer)key))
	{
		g_warning("Couldn't save path: the related key does already exist");
	}
	else
	{
		g_debug("Caching path: \"%s\"", path);
		g_hash_table_insert(pathbuilder_table, (gpointer)key, (gpointer)path);
	}
	g_mutex_unlock(pathbuilder_mutex);
}

const gchar *
pathbuilder_load_path(const gchar *key)
{
	const gchar *path;

	g_return_val_if_fail(initialized == TRUE, NULL);

	g_mutex_lock(pathbuilder_mutex);
	path = (const gchar *)g_hash_table_lookup(pathbuilder_table, (gpointer)key);
	g_mutex_unlock(pathbuilder_mutex);

	return path;	
}


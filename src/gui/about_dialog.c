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
 * \file about_dialog.c
 * \brief An about dialog.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 2. January 2012
 */

#include <string.h>

#include "about_dialog.h"
#include "mainwindow.h"
#include "../application.h"
#include "../pathbuilder.h"
#include "../helpers.h"

/**
 * @addtogroup Gui
 * @{
 */

static gchar *
_about_dialog_load_license(void)
{
	gchar *path;
	gchar *license = NULL;
	GError *err = NULL;

	g_debug("Loading license agreement");

	/* load license */
	path = g_build_filename(pathbuilder_get_share_directory(), G_DIR_SEPARATOR_S, "plain", G_DIR_SEPARATOR_S, "LICENSE.txt", NULL);
	g_debug("license file: \"%s\"", path);

	if(!g_file_get_contents(path, &license, NULL, &err))
	{
		g_warning("Couldn't load license agreement.");

		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	g_free(path);

	return license;
}

static GdkPixbuf *
_about_dialog_load_logo(void)
{
	gchar *path;
	GdkPixbuf *pixbuf;
	GError *err = NULL;

	g_debug("Loading logo");

	/* load logo */
	path = pathbuilder_build_image_path("logo.png");
	g_debug("logo file: \"%s\"", path);

	if(!(pixbuf = gdk_pixbuf_new_from_file(path, &err)))
	{
		g_warning("Couldn't load logo.");

		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	g_free(path);

	return pixbuf;

}

static void
_about_dialog_activate_link(GtkAboutDialog *dialog, const gchar *link, gpointer user_data)
{
	Config *config;
	UrlOpener *urlopener;
	gboolean success = FALSE;
	GtkWidget *failure_dialog;

	g_debug("Opening url: \"%s\"", link);

	config = mainwindow_lock_config((GtkWidget *)user_data);
	urlopener = helpers_create_urlopener_from_config(config);
	mainwindow_unlock_config((GtkWidget *)user_data);

	if(urlopener)
	{
		success = url_opener_launch(urlopener, link);
		url_opener_free(urlopener);
	}

	if(!success)
	{
		failure_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog),
		                                        GTK_DIALOG_MODAL,
		                                        GTK_MESSAGE_ERROR,
		                                        GTK_BUTTONS_OK,
		                                        "Couldn't launch browser, please check your preferences.");
		gtk_dialog_run(GTK_DIALOG(failure_dialog));
		gtk_widget_destroy(failure_dialog);
	}
}

GtkWidget *
about_dialog_create(GtkWidget *parent)
{
	GtkWidget *dialog;
	gchar version[32];
	gchar *license;
	GdkPixbuf *pixbuf;

	g_assert(GTK_IS_WINDOW(parent));

	dialog = gtk_about_dialog_new();

	/* set window properties */
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
	gtk_window_set_position(GTK_WINDOW(dialog), gtk_widget_get_visible(parent) ? GTK_WIN_POS_CENTER_ON_PARENT : GTK_WIN_POS_CENTER);

	/* build version string */
	sprintf(version, "version %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH_LEVEL);

	/* load license */
	license = _about_dialog_load_license();

	/* load logo */
	pixbuf = _about_dialog_load_logo();

	/* create dialog */
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), APPLICATION_NAME);
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), APPLICATION_NAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), version);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), APPLICATION_COPYRIGHT);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), APPLICATION_WEBSITE);
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog), APPLICATION_WEBSITE);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), APPLICATION_DESCRIPTION);

	if(pixbuf)
	{
		gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
		gdk_pixbuf_unref(pixbuf);
	}

	if(license)
	{
		gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), license);
		g_free(license);
	}

	/*
	 *	url hook:
	 */
	if(GTK_CHECK_VERSION(2, 24, 0))
	{
		g_object_connect(G_OBJECT(dialog), "activate-link", _about_dialog_activate_link, parent);
	}
	else
	{
		gtk_about_dialog_set_url_hook(_about_dialog_activate_link, parent, NULL);
	}

	/*
	 *	show dialog:
	 */
	gtk_widget_show_all(dialog);

	return dialog;
}

/**
 * @}
 */


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
 * \file systray.c
 * \brief Systray functionality.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 19. October 2010
 */

#include <glib/gi18n.h>

#include "systray.h"
#include "mainwindow.h"
#include "about_dialog.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/* The systray icon instance. */
static GtkStatusIcon *systray_icon_instance = NULL;

/* The systray icon popup menu instance. */
static GtkWidget *systray_menu_instance = NULL;

/* Creates a single instance of the systray icon popup menu. */
static GtkWidget *_systray_menu_get_instance(GtkWidget *parent);

/*
 *	helpers:
 */
static void
_systray_show_mainwindow(GtkWidget *widget)
{
	mainwindow_show(widget);
}

static void
_systray_hide_mainwindow(GtkWidget *widget)
{
	mainwindow_hide(widget);
}

/*
 *	events:
 */
static void
_systray_activate(GtkWidget *widget, gpointer user_data)
{
	_systray_show_mainwindow((GtkWidget *)user_data);
}

static void
_systray_show_menu(GtkStatusIcon *icon, guint button, guint activate_time, gpointer user_data)
{
	gtk_menu_popup(GTK_MENU(_systray_menu_get_instance((GtkWidget *)user_data)), NULL, NULL, NULL, NULL, button, activate_time);
}

static void
_systray_menu_item_show_window_clicked(GtkWidget *widget, GtkWidget *window)
{
	_systray_show_mainwindow(window);
}

static void
_systray_menu_item_hide_window_clicked(GtkWidget *widget, GtkWidget *window)
{
	_systray_hide_mainwindow(window);
}

static void
_systray_menu_item_about_clicked(GtkWidget *widget, GtkWidget *window)
{
	GtkWidget *dialog;

	dialog = about_dialog_create(window);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void
_systray_menu_quit_clicked(GtkWidget *widget, GtkWidget *window)
{
	mainwindow_quit(window);
}

/*
 *	menu:
 */
static GtkWidget *
_systray_menu_get_instance(GtkWidget *parent)
{
	GtkWidget *item;
	GList *children;
	GList *iter;

	/* check if systray icon menu has been created */
	if(systray_menu_instance)
	{
		/* menu has been created => remove menu items */
		g_debug("Clearing systray icon menu");
		children = gtk_container_get_children(GTK_CONTAINER(systray_menu_instance));
		iter = children;

		while(iter)
		{
			gtk_container_remove(GTK_CONTAINER(systray_menu_instance), iter->data);
			iter = g_list_next(iter);
		}

		g_list_free(children);
	}
	else
	{
		/* create menu instance */
		g_debug("Creating systray icon menu instance");
		systray_menu_instance = gtk_menu_new();
	}

	/* insert menu items */
	g_debug("Building systray icon menu");
	if(gtk_widget_get_visible(parent))
	{
		item = gtk_menu_item_new_with_label(_("Hide window"));
		gtk_menu_shell_append(GTK_MENU_SHELL(systray_menu_instance), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_systray_menu_item_hide_window_clicked), parent);
	}
	else
	{
		item = gtk_menu_item_new_with_label(_("Show window"));
		gtk_menu_shell_append(GTK_MENU_SHELL(systray_menu_instance), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_systray_menu_item_show_window_clicked), parent);
	}

	item = gtk_menu_item_new_with_label(_("About"));
	gtk_menu_shell_append(GTK_MENU_SHELL(systray_menu_instance), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_systray_menu_item_about_clicked), parent);

	item = gtk_menu_item_new_with_label(_("Quit program"));
	gtk_menu_shell_append(GTK_MENU_SHELL(systray_menu_instance), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(_systray_menu_quit_clicked), parent);

	gtk_widget_show_all(systray_menu_instance);

	return systray_menu_instance;
}

/*
 *	public:
 */
GtkStatusIcon *
systray_get_instance(GtkWidget *parent)
{
	gchar *filename;

	g_debug("Creating systray");

	if(!systray_icon_instance)
	{
		/* build icon filename */
		filename = pathbuilder_build_image_path("systray.png");
		g_debug("filename: \"%s\"", filename);

		/* create icon */
		systray_icon_instance = gtk_status_icon_new_from_file(filename);

		/* free memory */
		g_free(filename);

		/* signals */
		g_signal_connect(G_OBJECT(systray_icon_instance), "activate", G_CALLBACK(_systray_activate), parent);
		g_signal_connect(G_OBJECT(systray_icon_instance), "popup-menu", G_CALLBACK(_systray_show_menu), parent);
	}

	return systray_icon_instance;
}

void
systray_destroy(GtkStatusIcon *icon)
{
	if(G_IS_OBJECT(icon))
	{
		g_object_unref(icon);
		systray_icon_instance = NULL;
	}

	if(GTK_IS_WIDGET(systray_menu_instance))
	{
		gtk_widget_destroy(systray_menu_instance);
		systray_menu_instance = NULL;
	}
}

void
systray_set_visible(GtkStatusIcon *icon, gboolean show)
{
	gtk_status_icon_set_visible(icon, show);
}

/**
 * @}
 */


/***************************************************************************
    begin........: September 2010
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
 * \file statusbar.c
 * \brief Displays status information.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. October 2010
 */

#include "statusbar.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _StatusBar
 * \brief Status bar widgets.
 */
typedef struct
{
	/*! A spinner. */
	GtkWidget *spinner;
	/*! A text label. */
	GtkWidget *label;
} _StatusBar;

GtkWidget *
statusbar_create(void)
{
	_StatusBar *bar;
	GtkWidget *widget;

	g_debug("Creating statusbar");

	widget = gtk_hbox_new(FALSE, 5);
	bar = (_StatusBar *)g_malloc(sizeof(_StatusBar));

	#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 20
	bar->spinner = gtk_spinner_new();
	gtk_box_pack_start(GTK_BOX(widget), bar->spinner, FALSE, FALSE, 1);
	#endif

	bar->label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(widget), bar->label, FALSE, FALSE, 0);

	g_object_set_data(G_OBJECT(widget), "bar", bar);

	gtk_widget_show_all(widget);
	statusbar_show(widget, FALSE);

	return widget;
}

void
statusbar_destroy(GtkWidget *statusbar)
{
	g_assert(GTK_IS_HBOX(statusbar));

	g_free(g_object_get_data(G_OBJECT(statusbar), "bar"));
}

void
statusbar_show(GtkWidget *statusbar, gboolean show)
{
	g_assert(GTK_IS_HBOX(statusbar));

	gtk_widget_set_visible(statusbar, show);

	#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 20
	_StatusBar *bar = (_StatusBar *)g_object_get_data(G_OBJECT(statusbar), "bar");

	if(show)
	{
		gtk_spinner_start(GTK_SPINNER(bar->spinner));
	}
	else
	{
		gtk_spinner_stop(GTK_SPINNER(bar->spinner));
	}
	#endif
}

void
statusbar_set_text(GtkWidget *statusbar, const gchar *text)
{
	g_assert(GTK_IS_HBOX(statusbar));

	_StatusBar *bar = (_StatusBar *)g_object_get_data(G_OBJECT(statusbar), "bar");

	gtk_label_set_text(GTK_LABEL(bar->label), text);
}

/**
 * @}
 */


/***************************************************************************
    begin........: January 2012
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
 * \file pixbuf_helpers.c
 * \brief PixbufLoader helper functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 17. January 2012
 */

#ifndef __PIXBUF_HELPERS_H__
#define __PIXBUF_HELPERS_H__

#include <gtk/gtk.h>
#include "gtktwitterstatus.h"
#include "pixbuf_helpers.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _GtkWidgetPixbufArg
 * \brief Holds a GtkWidget and a GdkPixbuf to set.
 */
typedef struct
{
	/*! A GtkTwitterStatus. */
	GtkWidget *widget;
	/*! A GdkPixbuf to set. */
	GdkPixbuf *pixbuf;
}
_GtkWidgetPixbufArg;

static gboolean 
_pixbuf_helpers_set_gtktwitterstatus_worker(_GtkWidgetPixbufArg *arg)
{
	gdk_threads_enter();
	g_object_set(G_OBJECT(arg->widget), "pixbuf", arg->pixbuf, NULL);
	gdk_pixbuf_unref(arg->pixbuf);
	g_slice_free1(sizeof(_GtkWidgetPixbufArg), arg);
	gdk_threads_leave();

	return FALSE;
}

void
pixbuf_helpers_set_gtktwitterstatus_callback(GdkPixbuf *pixbuf, GtkTwitterStatus *status)
{
	_GtkWidgetPixbufArg *arg;

	g_assert(pixbuf != NULL);

	arg = (_GtkWidgetPixbufArg *)g_slice_alloc(sizeof(_GtkWidgetPixbufArg));
	arg->widget = GTK_WIDGET(status);
	arg->pixbuf = pixbuf;
	gdk_pixbuf_ref(pixbuf);
	g_idle_add((GSourceFunc)_pixbuf_helpers_set_gtktwitterstatus_worker, arg);
}

/**
 * @}
 */
#endif



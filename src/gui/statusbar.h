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
 * \file statusbar.h
 * \brief Displays status information.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 21. September 2010
 */

#ifndef __STATUSBAR_H___
#define __STATUSBAR_H___

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \return a new widget
 *
 * Creates the statusbar.
 */
GtkWidget *statusbar_create(void);

/**
 * \param statusbar the statusbar
 *
 * Destroys the statusbar.
 */
void statusbar_destroy(GtkWidget *statusbar);

/**
 * \param statusbar the statusbar
 * \param show TRUE to show the statusbar
 *
 * Shows/hides the statusbar.
 */
void statusbar_show(GtkWidget *statusbar, gboolean show);

/**
 * \param statusbar the statusbar
 * \param text the text to display
 *
 * Updates the text in the statusbar.
 */
void statusbar_set_text(GtkWidget *statusbar, const gchar *text);

/**
 * @}
 */
#endif


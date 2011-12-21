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
 * \file systray.h
 * \brief Systray functionality.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 12. October 2010
 */

#ifndef __SYSTRAY_H__
#define __SYSTRAY_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent pointer to the mainwindow
 * \return a systray icon
 *
 * Creates the systray icon.
 */
GtkStatusIcon *systray_get_instance(GtkWidget *parent);

/**
 * \param icon the systray icon
 *
 * Destroys the systray icon.
 */
void systray_destroy(GtkStatusIcon *icon);

/**
 * \param icon the systray icon
 * \param show specifies if the icon should be visible
 *
 * Shows/hides the systray icon.
 */
void systray_set_visible(GtkStatusIcon *icon, gboolean show);

/**
 * @}
 */
#endif


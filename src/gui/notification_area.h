/***************************************************************************
    begin........: March 2011
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
 * \file notification_area.h
 * \brief Displays notifications.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 18. March 2011
 */

#ifndef __NOTIFICATION_AREA_H__
#define __NOTIFICATION_AREA_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \enum NotificationLevel
 * \brief Notification levels.
 */
typedef enum
{
	/*! Debug message. */
	NOTIFICATION_LEVEL_DEBUG,
	/*! Info message. */
	NOTIFICATION_LEVEL_INFO,
	/*! Warning message. */
	NOTIFICATION_LEVEL_WARNING
} NotificationLevel;

/**
 * \return a new widget
 *
 * Creates the notification area.
 */
GtkWidget *notification_area_create(void);

/**
 * \param area the notification area
 *
 * Destroys the notification area.
 */
void notification_area_destroy(GtkWidget *area);

/**
 * \param area the notification area
 * \param level message level
 * \param text text to display
 *
 * Shows a notification message.
 */
void notification_area_notify(GtkWidget *area, NotificationLevel level, const gchar *text);

/**
 * @}
 */
#endif


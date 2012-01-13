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
 * \file status_dialog.h
 * \brief A dialog displaying tweets.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 13. January 2012
 */

#include <gtk/gtk.h>
#include "../twitter.h"

#ifndef __STATUS_DIALOG_H__
#define __STATUS_DIALOG_H__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent parent window
 * \param title window title
 * \return a GTK dialog
 *
 * Creates the status dialog.
 */
GtkWidget *status_dialog_create(GtkWidget *parent, const gchar *title);

/**
 * \param widget the status dialog
 * \param user author of the status
 * \param status status to append
 *
 * Adds a status to the dialog.
 */
void status_dialog_add_status(GtkWidget *widget, TwitterUser *user, TwitterStatus *status);

/**
 * @}
 */
#endif


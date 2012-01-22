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
 * \file select_account_dialog.h
 * \brief A dialog for selecting an account.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 19. January 2012
 */

#include <gtk/gtk.h>

#include "gtkuserlistdialog.h"


#ifndef __SELECT_ACCOUNT_DIALOG_H__
#define __SELECT_ACCOUNT_DIALOG_H__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent the mainwindow
 * \param accounts account list
 * \param count length of account list
 * \param title window title
 * \param text dialog text
 * \return a GTK dialog
 *
 * Creates the dialog.
 */
GtkWidget *select_account_dialog_create(GtkWidget *parent, gchar **accounts, gint length, const gchar *title, const gchar *text);

/**
 * \param dialog the dialog
 * \return response code
 *
 * Runs the "select account" dialog.
 */
gint select_account_dialog_run(GtkWidget *dialog);

/**
 * \param dialog the dialog
 * \return a new allocated string
 *
 * Gets the selected user account.
 */
gchar *select_account_dialog_get_account(GtkWidget *dialog);

/**
 * \param dialog the dialog
 * \param button_text button text or stock id
 * \param response_id reponse id of the button
 * \return the created button
 *
 * Adds a button to the dialog.
 */
GtkWidget *select_account_dialog_add_button(GtkWidget *dialog, const gchar *button_text, gint response_id);

/**
 * \param dialog the dialog
 * \return an action area
 *
 * Gets the action area of the dialog.
 */
GtkWidget *select_account_dialog_get_action_area(GtkWidget *dialog);

/**
 * @}
 */
#endif


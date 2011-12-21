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
 * \file add_account_dialog.h
 * \brief A dialog for adding an account.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 14. October 2010
 */

#ifndef __ACCOUNT_ADD_H__
#define __ACCOUNT_ADD_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent parent window
 * \param existing_accounts a list containing existing accounts
 * \return a GTK dialog
 *
 * Creates a dialog for adding an account.
 */
GtkWidget *add_account_dialog_create(GtkWidget *parent, GList *existing_accounts);

/**
 * \param dialog the dialog
 * \return a new allocated string
 *
 * Gets the entered username.
 */
gchar *add_account_dialog_get_username(GtkWidget *dialog);

/**
 * @}
 */
#endif


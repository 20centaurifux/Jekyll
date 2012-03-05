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
 * \file edit_members_dialog.h
 * \brief A dialog for editing (list) members.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 29. May 2011
 */

#include <gtk/gtk.h>

#include "gtkuserlistdialog.h"
#include "../twitter.h"


#ifndef __EDIT_MEMBERS_DIALOG__
#define __EDIT_MEMBERS_DIALOG__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent the mainwindow
 * \param title window title
 * \return a GTK dialog
 *
 * Creates the dialog.
 */
GtkWidget *edit_members_dialog_create(GtkWidget *parent, const gchar *title);

/**
 * \param dialog the dialog
 *
 * Destroys the dialog.
 */
void edit_members_dialog_destroy(GtkWidget *dialog);

/**
 * \param dialog the dialog
 * \param user user to add
 * \param checked state of the checkbox
 *
 * Adds a user to the dialog.
 */
void edit_members_dialog_add_user(GtkWidget *dialog, TwitterUser user, gboolean checked);

/**
 * @}
 */
#endif


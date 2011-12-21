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
 * \file edit_list_membership_dialog.h
 * \brief A dialog for editing the list membership of a Twitter user.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 11. March 2011
 */

#include <gtk/gtk.h>

#include "gtkdeletabledialog.h"

#ifndef __EDIT_LIST_MEMBERSHIP__
#define __EDIT_LIST_MEMBERSHIP__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent the mainwindow
 * \param username name of a Twitter user
 * \return a GTK dialog
 *
 * Creates the dialog.
 */
GtkWidget *edit_list_membership_dialog_create(GtkWidget *parent, const gchar *username);

/**
 * @}
 */
#endif


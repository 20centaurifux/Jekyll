/***************************************************************************
    begin........: April 2011
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
 * \file list_preferences_dialog.h
 * \brief Edit general list properties.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 14. April 2011
 */

#include <gtk/gtk.h>
#include "gtkdeletabledialog.h"


#ifndef __LIST_PREFERENCES_DIALOG__
#define __LIST_PREFERENCES_DIALOG__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent the mainwindow
 * \param title window title
 * \param owner list owner
 * \param listname name of the list to edit
 * \return a GTK dialog
 *
 * Creates the dialog.
 */
GtkWidget *list_preferences_dialog_create(GtkWidget *parent, const gchar *title, const gchar *owner, const gchar *listname);

/**
 * \param dialog the dialog
 * \return pointer to the listname
 *
 * Returns the listname.
 */
const gchar *preferences_dialog_get_listname(GtkWidget *dialog);

/**
 * @}
 */
#endif


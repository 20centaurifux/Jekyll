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
 * \file add_list_dialog.h
 * \brief Remove lists.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 29. April 2011
 */

#ifndef __ADD_LIST_DIALOG_H__
#define __ADD_LIST_DIALOG_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent the mainwindow
 * \param owner list owner
 * \return a GTK dialog
 *
 * Creates the dialog.
 */
GtkWidget *add_list_dialog_create(GtkWidget *parent, const gchar *owner);

/**
 * @}
 */
#endif


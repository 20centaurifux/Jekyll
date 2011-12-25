/***************************************************************************
    begin........: December 2011
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
 * \file search_dialog.h
 * \brief A dialog for entering search queries.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 25. December 2011
 */

#ifndef __SEARCH_DIALOG_H__
#define __SEARCH_DIALOG_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent parent window
 * \return a GTK dialog
 *
 * Creates a dialog for searching.
 */
GtkWidget *search_dialog_create(GtkWidget *parent);

/**
 * \param dialog the dialog
 * \return a new allocated string
 *
 * Gets the entered search query.
 */
gchar *search_dialog_get_query(GtkWidget *dialog);

/**
 * @}
 */
#endif



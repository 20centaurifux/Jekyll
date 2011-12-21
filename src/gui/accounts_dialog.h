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
 * \file accounts_dialog.h
 * \brief A dialog for editing accounts.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 10. October 2010
 */

#include <gtk/gtk.h>

#ifndef __ACCOUNTS_H__
#define __ACCOUNTS_H__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent parent window
 * \return a GTK dialog
 *
 * Creates the accounts dialog.
 */
GtkWidget *accounts_dialog_create(GtkWidget *parent);

/**
 * @}
 */
#endif


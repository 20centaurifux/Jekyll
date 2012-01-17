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
 * \file replies_dialog.h
 * \brief A dialog displaying tweets.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 17. January 2012
 */

#include <gtk/gtk.h>
#include "../twitter.h"

#ifndef __REPLIES_DIALOG_H__
#define __REPLIES_DIALOG_H__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent parent window
 * \param account name of an user account
 * \param first_status first status to append
 * \return a GTK dialog
 *
 * Creates the status dialog.
 */
GtkWidget *replies_dialog_create(GtkWidget *parent, const gchar * restrict account, const gchar * restrict first_status);

/**
 * \param widget status dialog
 *
 * Runs the status dialog.
 */
void replies_dialog_run(GtkWidget *widget);

/**
 * @}
 */
#endif


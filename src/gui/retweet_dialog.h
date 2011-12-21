/***************************************************************************
    begin........: July 2011
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
 * \file retweet_dialog.h
 * \brief Retweet a status.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 5. July 2011
 */

#ifndef __RETWEET_DIALOG_H__
#define __RETWEET_DIALOG_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent the mainwindow
 * \param user name of the user who wants to retweet a status
 * \param guid status to retweet
 * \return a GTK dialog
 *
 * Creates the dialog.
 */
GtkWidget *retweet_dialog_create(GtkWidget *parent, const gchar *user, const gchar *guid);

/**
 * @}
 */
#endif


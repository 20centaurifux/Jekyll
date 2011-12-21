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
 * \file authorize_account_dialog.h
 * \brief A dialog for account authorization.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 25. October 2010
 */

#ifndef __AUTHORIZE_ACCOUNT_H__
#define __AUTHORIZE_ACCOUNT_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param mainwindow the mainwindow
 * \param parent parent window
 * \param username name of the user to authorize
 * \return a GTK dialog
 *
 * Creates a dialog for Twitter OAuth authorization.
 */
GtkWidget *authorize_account_dialog_create(GtkWidget *mainwindow, GtkWidget *parent, const gchar *username);

/**
 * \param dialog the "authorize account" dialog
 * \param access_key location to store the access key
 * \param access_secret location to store the access secret
 * \return TRUE on access
 *
 * Gets the received access token and secret.
 */
gboolean authorize_account_dialog_get_token_and_secret(GtkWidget *dialog, gchar **access_key, gchar **access_secret);

/**
 * @}
 */
#endif


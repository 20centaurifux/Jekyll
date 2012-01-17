/***************************************************************************
    begin........: June 2011
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
 * \file composer_dialog.h
 * \brief A dialog for composing messages.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 13. January 2012
 */

#include <gtk/gtk.h>

#ifndef __COMPOSER_DIALOG_H__
#define __COMPOSER_DIALOG_H__

/**
 * @addtogroup Gui
 * @{
 */

/*! ComposerDialog apply callback function. */
typedef gboolean (* ComposerApplyCallback)(const gchar *user, const gchar *text, const gchar *prev_status, gpointer user_data);

/**
 * \param parent parent window
 * \param users list of users shown in a combobox
 * \param users_count number of users
 * \param selected_user name of the selected user
 * \param prev_status the previous
 * \param title window title
 * \return a GTK dialog
 *
 * Creates the dialog.
 */
GtkWidget *composer_dialog_create(GtkWidget *parent, gchar **users, gint users_count, const gchar *selected_user, const gchar *prev_status, const gchar *title);

/**
 * \param dialog composer dialog
 * \return a new allocated string
 *
 * Returns the entered text.
 */
gchar *composer_dialog_get_text(GtkWidget *dialog);

/**
 * \param dialog composer dialog
 * \param text text to set
 *
 * Sets a text.
 */
void composer_dialog_set_text(GtkWidget *dialog, const gchar *text);


/**
 * \param dialog composer dialog
 * \return a new allocated string
 *
 * Returns the selected user.
 */
gchar *composer_dialog_get_user(GtkWidget *dialog);

/**
 * \param dialog composer dialog
 * \param callback "apply" callback function
 * \param user_data user data
 *
 * Registers the "apply" callback function.
 */
void composer_dialog_set_apply_callback(GtkWidget *dialog, ComposerApplyCallback callback, gpointer user_data);

/**
 * @}
 */
#endif


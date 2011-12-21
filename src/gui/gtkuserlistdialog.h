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
 * \file gtkuserlistdialog.h
 * \brief A dialog containing an user list.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 9. June 2011
 */

#ifndef __GTK_USER_LIST_DIALOG_H__
#define __GTK_USER_LIST_DIALOG_H__

#include <gtk/gtk.h>

#include "gtkdeletabledialog.h"

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Widgets
 * 	@{
 */

/*! Get GType. */
#define GTK_USER_LIST_DIALOG_TYPE             (gtk_user_list_dialog_get_type())
/*! Cast to GtkUserListDialog. */
#define GTK_USER_LIST_DIALOG(inst)            (G_TYPE_CHECK_INSTANCE_CAST((inst), GTK_USER_LIST_DIALOG_TYPE, GtkUserListDialog))
/*! Cast to GtkUserListDialogClass. */
#define GTK_USER_LIST_DIALOG_CLASS(class)     (G_TYPE_CHECK_CLASS_CAST((class), GTK_USER_LIST_DIALOG_TYPE, GtkUserListDialogClass))
/*! Check if instance is GtkUserListDialog. */
#define IS_GTK_USER_LIST_DIALOG(inst)         (G_TYPE_CHECK_INSTANCE_TYPE((inst), GTK_USER_LIST_DIALOGTYPE))
/*! Check if class is GtkUserListDialogClass. */
#define IS_GTK_USER_LIST_DIALOG_CLASS(class)  (G_TYPE_CHECK_CLASS_TYPE((class), GTK_USER_LIST_DIALOGTYPE))
/*! Get GtkUserListDialogClass from GtkUserListDialog. */
#define GTK_USER_LIST_DIALOG_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS((inst), GTK_USER_LIST_DIALOG_TYPE, GtkUserListDialogClass))

/*! A type definition for _GtkUserListDialogPrivate. */
typedef struct _GtkUserListDialogPrivate GtkUserListDialogPrivate;

/*! A type definition for _GtkUserListDialogClass. */
typedef struct _GtkUserListDialogClass GtkUserListDialogClass;

/*! A type definition for _GtkUserListDialog. */
typedef struct _GtkUserListDialog GtkUserListDialog;

/**
 * \struct _GtkUserListDialogClass
 * \brief The _GtkUserListDialog class structure.
 *
 * The _GtkUserListDialog class structure.
 * It has the following properties:
 * - \b checkbox-column-visible: (boolean, rw)
 * - \b checkbox-column-activatable: (boolean, rw)
 * - \b checkbox-column-title: (string, rw)
 * - \b username-column-title: (string, rw)
 * - \b show-user-count: (boolean, rw)
 */
struct _GtkUserListDialogClass
{
	/*! The parent class. */
	GtkDeletableDialogClass parent_class;

	/**
	 * \param dialog GtkUserListDialog instance
	 * \param username name of the user to add
	 * \param pixbuf an image
	 * \param checked checkbox status
	 *
	 * Adds an user to the list.
	 */
	void (* append_user)(GtkUserListDialog *dialog, const gchar *username, GdkPixbuf *pixbuf, gboolean checked);

	/**
	 * \param dialog GtkUserListDialog instance
	 * \param username name of an user
	 * \param pixbuf an image
	 *
	 * Sets a pixbuf.
	 */
	void (* set_user_pixbuf)(GtkUserListDialog *dialog, const gchar *username, GdkPixbuf *pixbuf);

	/**
	 * \param dialog GtkUserListDialog instance
	 * \param checked TRUE to get checked users
	 * \return a GList containing usernames
	 *
	 * Returns assigned users.
	 */
	GList *(* get_users)(GtkUserListDialog *userlistdialog, gboolean checked);
};

/**
 * \struct _GtkUserListDialog
 * \brief A dialog containing a user list.
 *
 * A dialog containing a user list. See _GtkUserListDialogClass for more details.
 */
struct _GtkUserListDialog
{
	/*! The parent instance. */
	GtkDeletableDialogClass parent_instance;

	/*! Private data. */
	GtkUserListDialogPrivate *priv;
};

/*! See _GtkUserListDialogClass::append_user for further information. */
void gtk_user_list_dialog_append_user(GtkUserListDialog *dialog, const gchar *username, GdkPixbuf *pixbuf, gboolean checked);
/*! See _GtkUserListDialogClass::set_user_pixbuf for further information. */
void gtk_user_list_dialog_set_user_pixbuf(GtkUserListDialog *dialog, const gchar *username, GdkPixbuf *pixbuf);
/*! See _GtkUserListDialogClass::get_users for further information. */
GList *gtk_user_list_dialog_get_users(GtkUserListDialog *dialog, gboolean checked);

/**
 * \return a GType
 *
 * Registers the type in the type system.
 */
GType gtk_user_list_dialog_get_type(void)G_GNUC_CONST;

/**
 * \param parent parent window
 * \param title window title
 * \param modal TRUE to set the dialog modal
 * \return a new GtkUserListDialog instance.
 *
 * Creates a new GtkUserListDialog instance.
 */
GtkWidget *gtk_user_list_dialog_new(GtkWindow *parent, const gchar *title, gboolean modal);

/**
 * @}
 * @}
 */
#endif


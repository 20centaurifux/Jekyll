/***************************************************************************
    begin........: May 2010
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
 * \file accountbrowser.h
 * \brief A tree containing accounts.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 27. February 2012
 */

#ifndef __ACCOUNTBROWSER_H__
#define __ACCOUNTBROWSER_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/*! Node types. */
typedef enum
{
	ACCOUNTBROWSER_TREEVIEW_NODE_ACCOUNT,
	ACCOUNTBROWSER_TREEVIEW_NODE_MESSAGES,
	ACCOUNTBROWSER_TREEVIEW_NODE_DIRECT_MESSAGES,
	ACCOUNTBROWSER_TREEVIEW_NODE_REPLIES,
	ACCOUNTBROWSER_TREEVIEW_NODE_USER_TIMELINE,
	ACCOUNTBROWSER_TREEVIEW_NODE_FRIENDS,
	ACCOUNTBROWSER_TREEVIEW_NODE_FOLLOWERS,
	ACCOUNTBROWSER_TREEVIEW_NODE_LISTS,
	ACCOUNTBROWSER_TREEVIEW_NODE_LIST,
	ACCOUNTBROWSER_TREEVIEW_NODE_PROTECTED_LIST,
	ACCOUNTBROWSER_TREEVIEW_NODE_SEARCH,
	ACCOUNTBROWSER_TREEVIEW_NODE_SEARCH_QUERY
} AccountBrowserTreeViewNodeType;


#include "mainwindow.h" /* include mainwindow.h after the declaration of AccountBrowserTreeViewNodeType */

/**
 * \param parent the mainwindow
 * \return a new widget
 *
 * Creates the accountbrowser.
 */
GtkWidget *accountbrowser_create(GtkWidget *parent);

/**
 * \param widget the accountbrowser widget
 *
 * Destroys the accountbrowser.
 */
void accountbrowser_destroy(GtkWidget *widget);

/**
 * \param widget the accountbrowser widget
 * \param username name of the account to append
 *
 * Appends an account to the accountbrowser.
 */
void accountbrowser_append_account(GtkWidget *widget, const gchar *username);

/**
 * \param widget the accountbrowser widget
 * \param username name of the account to remove
 *
 * Removes an account from the accountbrowser.
 */
void accountbrowser_remove_account(GtkWidget *widget, const gchar *username);

/**
 * \param widget the accountbrowser widget
 * \return a list containing populated accounts
 *
 * Gets a list containing populated accounts.
 */
GList *accountbrowser_get_accounts(GtkWidget *widget);

/**
 * \param widget the accountbrowser widget
 * \param username name of the list owner
 * \param listname name of the list to append
 * \param protected specifies if the list is protected
 *
 * Appends a list to the accountbrowser.
 */
void accountbrowser_append_list(GtkWidget *widget, const gchar * restrict username, const gchar * restrict listname, gboolean protected);

/**
 * \param widget the accountbrowser widget
 * \param username name of the list owner
 * \param listname name of the list to remove
 *
 * Removes a list from the accountbrowser.
 */
void accountbrowser_remove_list(GtkWidget *widget, const gchar * restrict username, const gchar * restrict listname);

/**
 * \param widget the accountbrowser widget
 * \param owner owner of the list to update
 * \param old_name the old listname
 * \param new_name new list name
 * \param protected protected
 *
 * Updates a list.
 */
void accountbrowser_update_list(GtkWidget *widget, const gchar * restrict owner, const gchar * restrict old_name, const gchar * restrict new_name, gboolean protected);

/**
 * \param widget the accountbrowser widget
 * \param username name of the lists owner 
 * \param lists GList containing lists
 *
 * Sets a list protected.
 */
void accountbrowser_set_lists(GtkWidget *widget, const gchar *username, GList *lists);

/**
 * \param widget the accountbrowser widget
 * \param query query to append
 *
 * Appends a search query to the treeview.
 */
void accountbrowser_append_search_query(GtkWidget *widget, const gchar *query);

/**
 * \param widget the accountbrowser widget
 * \param query query to remove
 *
 * Removes a search query to the treeview.
 */
void accountbrowser_remove_search_query(GtkWidget *widget, const gchar *query);

/**
 * @}
 */
#endif


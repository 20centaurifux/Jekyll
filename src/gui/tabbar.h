/***************************************************************************
    begin........: June 2010
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
 * \file tabbar.h
 * \brief Tab functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. September 2011
 */

#ifndef __TABS_H__
#define __TABS_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/*! Tab type identifiers. */
typedef enum
{
	TAB_TYPE_ID_PUBLIC_TIMELINE,
	TAB_TYPE_ID_DIRECT_MESSAGES,
	TAB_TYPE_ID_REPLIES,
	TAB_TYPE_ID_USER_TIMELINE,
	TAB_TYPE_ID_LIST
} TabTypeId;

/*! Typedef macro for _Tab structure. */
typedef struct _Tab Tab;

/*! Typedef macro for _Tab structure. */
typedef struct _TabFuncs TabFuncs;

/**
 * \struct _Tab
 *
 * \brief tab data
 */
struct _Tab
{
	/*! The page widget. */
	GtkWidget *widget;
	/*! Tab functions. */
	TabFuncs *funcs;
	/*! Type identifier. */
	guint type_id;
	/**
	 * \struct _id
	 * \brief Holds tab id and related mutex.
	 *
	 * \var id
	 * \brief Tab id and related mutex.
	 */
	struct _id
	{
		/*! Id of the tab. */
		gchar *id;
		/*! Mutex to protect the id. */
		GMutex *mutex;
	} id;
};

/**
 * \struct _TabFuncs
 *
 * \brief tab functions
 */
struct _TabFuncs
{
	/*! Called if a tab is going to be destroyed. */
	void (* destroy)(GtkWidget *widget);
	/*! Updates a tab. */
	void (* refresh)(GtkWidget *widget);
	/*! Sets a tab busy. */
	void (* set_busy)(GtkWidget *widget, gboolean busy);
	/*! Scroll function. */
	void (* scroll)(GtkWidget *widget, gboolean down);
};

/**
 * \param tab a tab
 * \return a new allocated string
 *
 * Gets the id of a tab.
 */
gchar *tab_get_id(Tab *tab);

/**
 * \param tab a tab
 * \param id id to set
 *
 * Sets the id of a tab.
 */
void tab_set_id(Tab *tab, const gchar *id);

/**
 * \param parent reference to the mainwindow
 * \return a new widget
 *
 * Creates the tabbar.
 */
GtkWidget *tabbar_create(GtkWidget *parent);

/**
 * \param widget the tabbar widget
 *
 * Destroys the tabbar.
 */
void tabbar_destroy(GtkWidget *widget);

/**
 * \param widget the tabbar widget
 *
 * Updates all tabs.
 */
void tabbar_refresh(GtkWidget *widget);

/**
 * \param widget the tabbar widget
 *
 * Returns the current tab id or NULL.
 */
gchar *tabbar_get_current_id(GtkWidget *widget);

/**
 * \param widget the tabbar widget
 *
 * Closes the current tab.
 */
void tabbar_close_current_tab(GtkWidget *widget);

/**
 * \param widget the tabbar widget
 * \param type_id id of the tab to close
 * \param id id of the tab to close
 *
 * Closes a tab.
 */
void tabbar_close_tab(GtkWidget *widget, TabTypeId type_id, const gchar *id);

/**
 * \param widget the tabbar widget
 * \param account name of an account
 *
 * Opens public timeline of an account.
 */
void tabbar_open_public_timeline(GtkWidget *widget, const gchar *account);

/**
 * \param widget the tabbar widget
 * \param account name of an account
 *
 * Opens direct messages of an account.
 */
void tabbar_open_direct_messages(GtkWidget *widget, const gchar *account);

/**
 * \param widget the tabbar widget
 * \param account name of an account
 *
 * Opens replies of an account.
 */
void tabbar_open_replies(GtkWidget *widget, const gchar *account);

/**
 * \param widget the tabbar widget
 * \param user name of an user
 *
 * Opens an user timeline.
 */
void tabbar_open_user_timeline(GtkWidget *widget, const gchar *user);

/**
 * \param widget the tabbar widget
 * \param user name of an user
 * \param list name of a list
 *
 * Opens a list.
 */
void tabbar_open_list(GtkWidget *widget, const gchar *user, const gchar *list);

/**
 * \param widget the tabbar widget
 * \param user name of an user
 * \param old_listname old name of a list
 * \param new_listname new name of a list
 *
 * Updates an open list.
 */
void tabbar_update_list(GtkWidget *widget, const gchar *user, const gchar *old_listname, const gchar *new_listname);

/**
 * \param widget the tabbar widget
 * \param busy status to set
 *
 * Sets the busy status.
 */
void tabbar_set_busy(GtkWidget *widget, gboolean busy);

/**
 * \param widget the tabbar widget
 *
 * Gets the busy status.
 */
gboolean tabbar_is_busy(GtkWidget *widget);

/**
 * \param widget the tabbar widget
 * \param page a page
 * \param busy status to set
 *
 * Indicates if a tab is working or not.
 */
void tabbar_set_page_busy(GtkWidget *widget, GtkWidget *page, gboolean busy);

/**
 * \param widget the tabbar widget
 * \return a pointer to the mainwindow
 *
 * Gets the mainwindow.
 */
GtkWidget *tabbar_get_mainwindow(GtkWidget *widget);

/**
 * @}
 */
#endif


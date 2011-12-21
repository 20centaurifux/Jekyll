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
 * \file gtk_helpers.h
 * \brief Gtk helper functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 21. April 2011
 */

#ifndef __GTK_HELPERS_H__
#define __GTK_HELPERS_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param widget a widget
 * \param busy status to set
 *
 * Disables a widget and updates the mouse cursor.
 */
void gtk_helpers_set_widget_busy(GtkWidget *widget, gboolean busy);

/**
 * \param model a GtkTreeModel
 * \param value value to search for
 * \param compare_value a function to check two values for equality
 * \param destroy_value a function to free a found value
 * \param column index of the column
 * \param iter location to store the found iterator
 * \return TRUE if the list item could be found
 *
 * Searches for a value in a GtkTreeModel.
 */
gboolean gtk_helpers_tree_model_find_iter(GtkTreeModel *model, gpointer value, GEqualFunc compare_value, GDestroyNotify destroy_value, gint column, GtkTreeIter *iter);

/**
 * \param model a GtkTreeModel
 * \param text text to search for
 * \param column index of the column
 * \param iter location to store the found iterator
 * \return TRUE if the list item could be found
 *
 * Searches for a text in a GtkTreeModel.
 */
gboolean gtk_helpers_tree_model_find_iter_by_string(GtkTreeModel *model, const gchar *text, gint column, GtkTreeIter *iter);

/**
 * \param model a GtkTreeModel
 * \param integer integer to search for
 * \param column index of the column
 * \param iter location to store the found iterator
 * \return TRUE if the list item could be found
 *
 * Searches for an integer in a GtkTreeModel.
 */
gboolean gtk_helpers_tree_model_find_iter_by_integer(GtkTreeModel *model, gint integer, gint column, GtkTreeIter *iter);

/**
 * \param tree a GtkTreeView
 * \return the number of nodes
 *
 * Counts the nodes in a GtkTreeView.
 */
gint gtk_helpers_tree_view_count_nodes(GtkWidget *tree);

/**
 * \param dialog dialog to run
 * \return always FALSE
 *
 * Worker to run and destroy a GtkDialog.
 */
gboolean gtk_helpers_run_and_destroy_dialog_worker(GtkWidget *dialog);

/**
 * @}
 */
#endif


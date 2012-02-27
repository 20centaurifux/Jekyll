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
 * \file gtk_helpers.c
 * \brief Gtk helper functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 27. February 2012
 */

#include "gtk_helpers.h"

/**
 * @addtogroup Gui
 * @{
 */

/*
 *	helpers:
 */
static gboolean
_gtk_helpers_int_equal(gconstpointer a, gconstpointer b)
{
	return GPOINTER_TO_INT(a) == GPOINTER_TO_INT(b);
}

/*
 *	public:
 */
void
gtk_helpers_set_widget_busy(GtkWidget *widget, gboolean busy)
{
	GdkCursor *cursor;
	GdkCursorType type;

	g_assert(GTK_IS_WIDGET(widget));

	/* update cursor */
	type = busy ? GDK_WATCH : GDK_LEFT_PTR;
	cursor = gdk_cursor_new(type);
	gdk_window_set_cursor(widget->window, cursor);
	gdk_cursor_unref(cursor);

	/* update window */
	gtk_widget_set_sensitive(widget, busy ? FALSE : TRUE);
}

gboolean
gtk_helpers_tree_model_find_iter(GtkTreeModel *model, gpointer value, GEqualFunc compare_value, GDestroyNotify destroy_value, gint column, GtkTreeIter *iter)
{
	gboolean found = FALSE;
	gboolean valid;
	gpointer ptr;

	g_assert(GTK_IS_TREE_MODEL(model));
	g_assert(compare_value != NULL);
	g_assert(column >= 0);

	valid = gtk_tree_model_get_iter_first(model, iter);

	while(valid && !found)
	{
		gtk_tree_model_get(model, iter, column, &ptr, -1);

		if(compare_value(value, ptr))
		{
			found = TRUE;
		}
		else
		{
			valid = gtk_tree_model_iter_next(model, iter);
		}

		if(destroy_value)
		{
			destroy_value(ptr);
		}
	}

	return found;
}

gboolean
gtk_helpers_tree_model_find_iter_by_string(GtkTreeModel *model, const gchar *text, gint column, GtkTreeIter *iter)
{
	return gtk_helpers_tree_model_find_iter(model, (gpointer)text, g_str_equal, g_free, column, iter);
}

gboolean
gtk_helpers_tree_model_find_iter_by_integer(GtkTreeModel *model, gint integer, gint column, GtkTreeIter *iter)
{
	return gtk_helpers_tree_model_find_iter(model, GINT_TO_POINTER(integer), _gtk_helpers_int_equal, NULL, column, iter);
}

gint
gtk_helpers_tree_view_count_nodes(GtkWidget *tree)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	int count = 0;
	gboolean loop;

	g_assert(GTK_IS_TREE_VIEW(tree));

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	loop = gtk_tree_model_get_iter_first(model, &iter);

	while(loop)
	{
		++count;
		loop = gtk_tree_model_iter_next(model, &iter);
	}

	return count;
}

gboolean
gtk_helpers_run_and_destroy_dialog_worker(GtkWidget *dialog)
{
	gdk_threads_enter();
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	gdk_threads_leave();

	return FALSE;
}

/**
 * @}
 */


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
 * \file edit_list_membership_dialog.c
 * \brief A dialog for editing the list membership of a Twitter user.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2011
 */

#include <glib/gi18n.h>

#include "edit_list_membership_dialog.h"
#include "mainwindow.h"
#include "gtk_helpers.h"
#include "../application.h"
#include "../twitterdb.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _EditListMembershipWindowPrivate
 * \brief Private data of the "edit list membership" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! A list containing Twitter lists. */
	GtkWidget *tree_lists;
	/*! Name of a Twitter user. */
	gchar *username;
	/*! "Apply" button. */
	GtkWidget *button_apply;
} _EditListMembershipWindowPrivate;

enum
{
	EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_CHECKBOX,
	EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_IMAGE,
	EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_USERNAME,
	EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_LISTNAME
};

/*
 *	dialog events:
 */
static void
_edit_list_membership_dialog_destroy(GtkWidget *dialog, gpointer user_data)
{
	_EditListMembershipWindowPrivate *private;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_EditListMembershipWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	g_free(private->username);
	g_free(private);
}

static gboolean
_edit_list_membership_dialog_delete(GtkDeletableDialog *dialog, GdkEvent event, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	if(gtk_widget_get_sensitive(GTK_WIDGET(dialog)))
	{
		gtk_deletable_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
	}

	return TRUE;
}

/*
 *	apply changes:
 */
static gboolean
_edit_list_membership_dialog_update_membership(TwitterClient *client, const gchar *owner, const gchar *list, const gchar *username, gboolean is_member, GError **err)
{
	gboolean result = FALSE;

	g_assert(!*err);

	if(is_member)
	{
		g_debug("Appending user \"%s\" to list \"%s\"@\"%s\"", username, owner, list);
		result = twitter_client_add_user_to_list(client, owner, list, username, err);
	}
	else
	{
		g_debug("Removing user \"%s\" from list \"%s\"@\"%s\"", username, owner, list);
		result = twitter_client_remove_user_from_list(client, owner, list, username, err);
	}

	return result;
}

static gpointer
_edit_list_membership_dialog_apply_worker(GtkWidget *dialog)
{
	_EditListMembershipWindowPrivate *private;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean loop;
	gboolean success = FALSE;
	TwitterDbHandle *handle;
	gboolean is_member;
	gboolean was_member;
	gchar *owner;
	gchar *list;
	TwitterClient *client;
	GtkWidget *message_dialog;
	GError *err = NULL;

	g_debug("Starting worker...");

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_EditListMembershipWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	/* get tree model*/
	gdk_threads_enter();
	model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(gtk_tree_view_get_model(GTK_TREE_VIEW(private->tree_lists))));
	gdk_threads_leave();

	/* search for membership updates */
	if((handle = twitterdb_get_handle(&err)))
	{
		loop = gtk_tree_model_get_iter_first(model, &iter);
		success = TRUE;

		while(loop && success)
		{
			gdk_threads_enter();
			gtk_tree_model_get(model,
			                   &iter,
			                   EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_CHECKBOX, &is_member,
			                   EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_USERNAME, &owner,
			                   EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_LISTNAME, &list,
			                   -1);
			gdk_threads_leave();

			was_member = twitterdb_user_is_list_member(handle, owner, list, private->username, &err);

			/* check if membership has been changed */
			if(!err && was_member != is_member)
			{
				/* update membership*/
				g_debug("Updating membership: \"%s\"@\"%s\" (\"%s\")", owner, list, private->username);

				if((client = mainwindow_create_twittter_client(private->parent, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
				{
					success = _edit_list_membership_dialog_update_membership(client, owner, list, private->username, is_member, &err);
					g_object_unref(client);
				}
				else
				{
					g_warning("Couldn't create TwitterClient instance, please check your configuration");
					success = FALSE;
				}
			}
			else if(err)
			{
				success = FALSE;
			}

			/* free memory & get next list */
			g_free(owner);
			g_free(list);

			loop = gtk_tree_model_iter_next(model, &iter);
		}

		twitterdb_close_handle(handle);
	}

	/* cleanup & show failure message */
	if(!success)
	{
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}

		/* show failure message */
		message_dialog = gtk_message_dialog_new(GTK_WINDOW(private->parent),
							GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							GTK_BUTTONS_OK,
							_("Couldn't update list membership, please try again later."));
		g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, message_dialog);
	}

	/* close dialog */
	gdk_threads_enter();
	gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), success ? GTK_RESPONSE_APPLY : GTK_RESPONSE_NONE);
	gdk_threads_leave();

	/* close dialog */
	g_debug("Leaving worker...");


	return NULL;
}

static void
_edit_list_membership_dialog_apply_clicked(GtkWidget *button, GtkWidget *dialog)
{
	GError *err = NULL;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	/* show busy cursor */
	gtk_helpers_set_widget_busy(dialog, TRUE);

	/* start worker thread */
	if(!g_thread_create((GThreadFunc)_edit_list_membership_dialog_apply_worker, dialog, FALSE, &err))
	{
		if(err)
		{
			g_warning(err->message);
			g_error_free(err);
		}

		g_error("%s: Couldn't launch worker.", __func__);
	}
}

/*
 *	treeview:
 */
static void
_edit_list_membership_dialog_iter_toggled(GtkCellRendererToggle *renderer, gchar *path, GtkTreeView *tree)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeIter child;

	model = gtk_tree_view_get_model(tree);

	if(gtk_tree_model_get_iter_from_string(model, &iter, path))
	{
		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(model), &child, &iter);
		model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(model));
		gtk_list_store_set(GTK_LIST_STORE(model),
		                                  &child,
		                                  EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_CHECKBOX,
		                                  gtk_cell_renderer_toggle_get_active(renderer) ? FALSE : TRUE,
		                                  -1);
	}
}

static gint
_edit_list_membership_dialog_compare_iter(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
	gchar *owner_a;
	gchar *owner_b;
	gchar *list_a;
	gchar *list_b;
	gint result = 0;

	gtk_tree_model_get(model, a, EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_USERNAME, &owner_a, EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_LISTNAME, &list_a, -1);
	gtk_tree_model_get(model, b, EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_USERNAME, &owner_b, EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_LISTNAME, &list_b, -1);

	if(owner_b && list_b)
	{
		if(!(result = g_ascii_strcasecmp(owner_a, owner_b)))
		{
			result = g_ascii_strcasecmp(list_a, list_b);
		}
	}

	g_free(owner_a);
	g_free(list_a);
	g_free(owner_b);
	g_free(list_b);

	return result;
}

static GtkWidget *
_edit_list_membership_dialog_create_tree(void)
{
	GtkWidget *tree;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	/* create treeview & model */
	tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);

	store = gtk_list_store_new(4, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_LISTNAME, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_LISTNAME, _edit_list_membership_dialog_compare_iter, NULL, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);

	/* insert column to display checkbox */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Member"));

	renderer = gtk_cell_renderer_toggle_new();
	gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
	g_signal_connect(G_OBJECT(renderer), "toggled", (GCallback)_edit_list_membership_dialog_iter_toggled, tree);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "active", EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_CHECKBOX);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* insert column to display icon & text */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("List owner"));

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_IMAGE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_USERNAME);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* insert column to display list name */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("List"));

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_LISTNAME);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* free data */
	g_object_unref(store);
	g_object_unref(model);

	return tree;
}

static void
_edit_list_membership_dialog_append_list(GtkWidget *tree, const gchar *owner, const gchar *listname, gboolean is_member)
{
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreeIter iter;

	g_assert(GTK_IS_TREE_VIEW(tree));

	model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(gtk_tree_view_get_model(GTK_TREE_VIEW(tree))));
	store = GTK_LIST_STORE(model);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store,
                           &iter,
                           EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_USERNAME,
                           owner,
                           EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_LISTNAME,
                           listname,
	                   EDIT_LIST_MEMBERSHIP_TREEVIEW_COLUMN_CHECKBOX,
	                   is_member,
                           -1);
}

static void
_edit_list_membership_dialog_populate_lists_from_account(GtkWidget *tree, const gchar *owner, const gchar *user)
{
	TwitterDbHandle *handle;
	GList *lists;
	GList *iter;
	gchar guid[32];
	TwitterUser userdata;
	TwitterList *list;
	gboolean is_member = FALSE	;
	GError *err = NULL;

	if((handle = twitterdb_get_handle(&err)))
	{
		if((twitterdb_map_username(handle, owner, guid, 32, &err)))
		{
			if((lists = iter = twitterdb_get_lists(handle, guid, &userdata, NULL)))
			{
				while(iter)
				{
					list = (TwitterList *)iter->data;
					is_member = twitterdb_user_is_list_member(handle, owner, list->name, user, &err);

					if(err)
					{
						g_warning("%s", err->message);
						g_error_free(err);
						err = NULL;
					}

					_edit_list_membership_dialog_append_list(tree, owner, list->name, is_member);
					iter = iter->next;
				}

				twitterdb_free_get_lists_result(lists);
			}
		}

		twitterdb_close_handle(handle);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}
}

static void
_edit_list_membership_dialog_populate(GtkWidget *parent, GtkWidget *tree, const gchar *user)
{
	Config *config;
	Section *section;
	Section *child;
	Value *value;
	gint count;

	/* get root section from configuration */
	config = mainwindow_lock_config(parent);
	section = config_get_root(config);

	/* find "Accounts" section */
	if((section = section_find_first_child(section, "Twitter")) && (section = section_find_first_child(section, "Accounts")))
	{
		count = section_n_children(section);

		for(gint i = 0; i < count; ++i)
		{
			child = section_nth_child(section, i);

			if(!g_ascii_strcasecmp(section_get_name(child), "Account"))
			{
				if((value = section_find_first_value(child, "username")) && VALUE_IS_STRING(value))
				{
					_edit_list_membership_dialog_populate_lists_from_account(tree, value_get_string(value), user);
				}
			}
		}
	}

	/* unlock configuration */
	mainwindow_unlock_config(parent);
}

/*
 *	public:
 */
GtkWidget *
edit_list_membership_dialog_create(GtkWidget *parent, const gchar *username)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *area;
	GtkWidget *align;
	gchar *filename;
	GtkWidget *image;
	_EditListMembershipWindowPrivate *private = (_EditListMembershipWindowPrivate *)g_malloc(sizeof(_EditListMembershipWindowPrivate));

	g_assert(GTK_IS_WINDOW(parent));
	g_assert(username != NULL);

	g_debug("Opening \"edit list membership\" dialog");

	private->parent = parent;
	private->username = g_strdup(username);

	dialog = gtk_deletable_dialog_new_with_buttons(_("List membership"), GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* mainbox */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog))), hbox);

	/* image */
	align = gtk_alignment_new(0, 0.1, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, FALSE, 5);

	filename = pathbuilder_build_image_path("list.png");
	image = gtk_image_new_from_file(filename);
	g_free(filename);

	gtk_container_add(GTK_CONTAINER(align), image);

	/* create account list */
	frame = gtk_frame_new("Lists");
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	private->tree_lists = _edit_list_membership_dialog_create_tree();
	gtk_box_pack_start(GTK_BOX(vbox), private->tree_lists, TRUE, TRUE, 5);

	/* insert buttons */
	area = gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));

	private->button_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_box_pack_start(GTK_BOX(area), private->button_apply, FALSE, FALSE, 0);

	gtk_deletable_dialog_add_action_widget(GTK_DELETABLE_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_CANCEL), GTK_RESPONSE_CANCEL);

	/* populate data */
	_edit_list_membership_dialog_populate(parent, private->tree_lists, username);

	/* set size & show widgets */
	gtk_widget_set_size_request(dialog, -1, 300);
	gtk_widget_show_all(dialog);

	/* signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_edit_list_membership_dialog_destroy), NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(_edit_list_membership_dialog_delete), NULL);
	g_signal_connect(private->button_apply, "clicked", G_CALLBACK(_edit_list_membership_dialog_apply_clicked), dialog);

	return dialog;
}

/**
 * @}
 */


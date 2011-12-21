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
 * \file accounts_dialog.c
 * \brief A dialog for editing accounts.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2011
 */

#include <glib/gi18n.h>

#include "accounts_dialog.h"
#include "add_account_dialog.h"
#include "authorize_account_dialog.h"
#include "mainwindow.h"
#include "gtkdeletabledialog.h"
#include "gtk_helpers.h"
#include "../configuration.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _AccountsWindowPrivate
 * \brief Private data of the "accounts" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! A list containing accounts. */
	GtkWidget *tree_accounts;
	/*! A button used to add accounts. */
	GtkWidget *button_add;
	/*! A button used to authorize an account. */
	GtkWidget *button_authenticate;
	/*! A button used to remove an account. */
	GtkWidget *button_remove;
} _AccountsWindowPrivate;

enum
{
	ACCOUNTS_TREEVIEW_COLUMN_IMAGE,
	ACCOUNTS_TREEVIEW_COLUMN_TEXT,
	ACCOUNTS_TREEVIEW_COLUMN_AUTHORIZED
};

/*
 *	misc:
 */
static gchar *
_accounts_dialog_get_selected_account(GtkWidget *dialog)
{
	_AccountsWindowPrivate *private;
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *account = NULL;

	g_assert(GTK_IS_DIALOG(dialog));

	private = (_AccountsWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(private->tree_accounts));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(private->tree_accounts));

	if(gtk_tree_selection_get_selected(sel, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, ACCOUNTS_TREEVIEW_COLUMN_TEXT, &account, -1);
	}

	return account;
}

static int
_accounts_dialog_count_accounts(GtkWidget *tree)
{
	g_assert(GTK_IS_TREE_VIEW(tree));

	return gtk_helpers_tree_view_count_nodes(tree);
}

static GList *
_accounts_dialog_get_accounts(GtkWidget *tree)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean loop;
	GList *accounts = NULL;
	gchar *account;

	g_assert(GTK_IS_TREE_VIEW(tree));

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	loop = gtk_tree_model_get_iter_first(model, &iter);

	while(loop)
	{
		gtk_tree_model_get(model, &iter, ACCOUNTS_TREEVIEW_COLUMN_TEXT, &account, -1);
		accounts = g_list_append(accounts, account);
		loop = gtk_tree_model_iter_next(model, &iter);
	}

	return accounts;
}



static void
_accounts_dialog_update_widgets(GtkWidget *dialog)
{
	_AccountsWindowPrivate *private;

	g_assert(GTK_IS_DIALOG(dialog));

	private = (_AccountsWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(private->tree_accounts))) > 0)
	{
		gtk_widget_set_sensitive(private->button_authenticate, TRUE);
		gtk_widget_set_sensitive(private->button_remove, (_accounts_dialog_count_accounts(private->tree_accounts) > 1) ? TRUE : FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(private->button_authenticate, FALSE);
		gtk_widget_set_sensitive(private->button_remove, FALSE);
	}
}

static GdkPixbuf *
_accounts_dialog_load_account_pixbuf(void)
{
	gchar *filename;
	GdkPixbuf *pixbuf;
	GError *err = NULL;

	filename = pathbuilder_build_icon_path("16x16", "account.png");

	if(!(pixbuf = gdk_pixbuf_new_from_file(filename, &err)))
	{
		g_warning("Couldn't load icon: \"%s\"", filename);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_free(filename);

	return pixbuf;
}

/*
 *	account tree;
 */
static gint
_accounts_dialog_accounts_tree_compare_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
	gchar *str_a;
	gchar *str_b;
	gint result;

	g_assert(GTK_IS_TREE_MODEL(model));

	/* get strings */
	gtk_tree_model_get(model, a, ACCOUNTS_TREEVIEW_COLUMN_TEXT, &str_a, -1);
	gtk_tree_model_get(model, b, ACCOUNTS_TREEVIEW_COLUMN_TEXT, &str_b, -1);

	/* compare strings */
	if(str_a && str_b)
	{
		result = g_ascii_strcasecmp(str_a, str_b);
	}
	else
	{
		result = 1;
	}

	/* free memory */
	if(str_a)
	{
		g_free(str_a);
	}

	if(str_b)
	{
		g_free(str_b);
	}

	return result;
}

static void
_accounts_dialog_accounts_tree_select(GtkWidget *tree, const gchar *username)
{
	GtkTreeModel *model; 
	GtkTreeIter iter;

	g_assert(GTK_IS_TREE_VIEW(tree));
	g_assert(username != NULL);

	model = GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));

	if(gtk_helpers_tree_model_find_iter_by_string(model, username, ACCOUNTS_TREEVIEW_COLUMN_TEXT, &iter))
	{
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), &iter);
	}
}

static void
_accounts_dialog_accounts_tree_append(GtkWidget *tree, const gchar *username, GdkPixbuf *pixbuf, gboolean authenticated)
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
                           ACCOUNTS_TREEVIEW_COLUMN_IMAGE,
                           pixbuf,
                           ACCOUNTS_TREEVIEW_COLUMN_TEXT,
                           username,
                           ACCOUNTS_TREEVIEW_COLUMN_AUTHORIZED,
                           authenticated ? "gtk-yes" : "gtk-no",
                           -1);
}

static void
_accounts_dialog_accounts_tree_mark_authorized(GtkWidget *tree, const gchar *username)
{
	GtkTreeModel *model; 
	GtkTreeIter iter;

	g_assert(GTK_IS_TREE_VIEW(tree));
	g_assert(username != NULL);

	g_debug("Marking \"%s\" authorized", username);

	model = GTK_TREE_MODEL(gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)))));

	if(gtk_helpers_tree_model_find_iter_by_string(model, username, ACCOUNTS_TREEVIEW_COLUMN_TEXT, &iter))
	{

		gtk_list_store_set(GTK_LIST_STORE(model), &iter, ACCOUNTS_TREEVIEW_COLUMN_AUTHORIZED, "gtk-yes", -1);
	}
}

static void
_accounts_dialog_populate_accounts_tree(GtkWidget *dialog, GtkWidget *tree)
{
	_AccountsWindowPrivate *private;
	GdkPixbuf *pixbuf;
	GtkTreePath *path;
	Config *config;
	GList *accounts;
	GList *iter;
	Section *child;
	Value *value;
	const gchar *username;
	gboolean authorized;

	g_assert(GTK_IS_DIALOG(dialog));
	g_assert(GTK_IS_TREE_VIEW(tree));

	private = (_AccountsWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	/* load pixbuf */
	pixbuf = _accounts_dialog_load_account_pixbuf();

	/* append accounts */
	config = mainwindow_lock_config(private->parent);

	if((accounts = section_select_children(config_get_root(config), "Twitter/Accounts/Account", NULL)))
	{
		iter = accounts;

		while(iter)
		{
			child = (Section *)iter->data;
			value = section_find_first_value(child, "username");

			if(VALUE_IS_STRING(value))
			{
				username = value_get_string(value);

				/* check if account has been authorized */
				if((value = section_find_first_value(child, "oauth_access_key")) && VALUE_IS_STRING(value) &&
				   (value = section_find_first_value(child, "oauth_access_secret")) && VALUE_IS_STRING(value))
				{
					authorized = TRUE;
				}
				else
				{
					authorized = FALSE;
				}

				/* append account to tree */
				_accounts_dialog_accounts_tree_append(tree, username, pixbuf, authorized);
			}

			iter = g_list_next(iter);
		}

		g_list_free(accounts);
	}

	mainwindow_unlock_config(private->parent);

	/* free pixbuf */
	if(pixbuf)
	{
		gdk_pixbuf_unref(pixbuf);
	}

	/* set cursor */
	path = gtk_tree_path_new_from_string("0");
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, NULL, FALSE);
	gtk_tree_path_free(path);
}

static void
_accounts_dialog_remove_selected_row(GtkWidget *tree)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeIter child_iter;

	g_assert(GTK_IS_TREE_VIEW(tree));

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

	if(gtk_tree_selection_get_selected(sel, &model, &iter))
	{
		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(model), &child_iter, &iter);
		gtk_list_store_remove(GTK_LIST_STORE(gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(model))), &child_iter);
	}
}

static GtkWidget *
_accounts_dialog_create_tree(void)
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

	store = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), 0, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), 0, _accounts_dialog_accounts_tree_compare_func, NULL, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);

	/* insert column to display icon & text */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Account"));

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", ACCOUNTS_TREEVIEW_COLUMN_IMAGE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", ACCOUNTS_TREEVIEW_COLUMN_TEXT);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* insert column to display authorization status */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Authorized"));

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "stock-id", ACCOUNTS_TREEVIEW_COLUMN_AUTHORIZED);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* free data */
	g_object_unref(store);
	g_object_unref(model);

	return tree;
}

/*
 *	authorize user:
 */
static gboolean
_accounts_dialog_write_oauth_data_to_config(Config *config, const gchar *username, const gchar *access_key, const gchar *access_secret)
{
	GList *accounts;
	GList *iter;
	gboolean found = FALSE;
	Section *section;
	Value *value;

	g_assert(config != NULL);
	g_assert(username != NULL);
	g_assert(access_key != NULL);
	g_assert(access_secret != NULL);

	g_debug("Updating oauth access key & secret");

	if((accounts = section_select_children(config_get_root(config), "/Twitter/Accounts/Account", NULL)))
	{
		g_debug("Searching for account (\"%s\")", username);
		iter = accounts;

		while(iter && !found)
		{
			section = (Section *)iter->data;
			if((value = section_find_first_value(section, "username")) && VALUE_IS_STRING(value))
			{
				if(!g_ascii_strcasecmp(value_get_string(value), username))
				{
					g_debug("Account found");
					found = TRUE;

					if(!(value = section_find_first_value(section, "oauth_access_key")))
					{
						value = section_append_value(section, "oauth_access_key", VALUE_DATATYPE_STRING);
					}

					value_set_string(value, access_key);

					if(!(value = section_find_first_value(section, "oauth_access_secret")))
					{
						value = section_append_value(section, "oauth_access_secret", VALUE_DATATYPE_STRING);
					}

					value_set_string(value, access_secret);
				}
			}

			iter = g_list_next(iter);
		}

		g_list_free(accounts);
	}

	if(!found)
	{
		g_warning("Couldn't update account: \"%s\"", username);
	}

	return found;
}

static gboolean
_accounts_dialog_authorize_user(GtkWidget *dialog, const gchar *username)
{
	_AccountsWindowPrivate *private;
	GtkWidget *authorize_dialog;
	gboolean success = FALSE;
	gchar *access_key = NULL;
	gchar *access_secret = NULL;
	Config *config;

	g_assert(GTK_IS_DIALOG(dialog));
	g_assert(username != NULL);

	private = (_AccountsWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	authorize_dialog = authorize_account_dialog_create(dialog, private->parent, username);

	if(gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(authorize_dialog)) == GTK_RESPONSE_OK)
	{
		if(authorize_account_dialog_get_token_and_secret(authorize_dialog, &access_key, &access_secret))
		{
			/* update configuration */
			config = mainwindow_lock_config(private->parent);
			success = _accounts_dialog_write_oauth_data_to_config(config, username, access_key, access_secret);
			mainwindow_unlock_config(private->parent);

			/* update tree */
			if(success)
			{
				_accounts_dialog_accounts_tree_mark_authorized(private->tree_accounts, username);
			}
		}
	}

	if(GTK_IS_WINDOW(authorize_dialog))
	{
		gtk_widget_destroy(authorize_dialog);
	}

	return success;
}


/*
 *	append account:
 */
static gboolean
_accounts_dialog_add_account_to_config(Config *config, const gchar *username)
{
	Section *section;
	Section *account;

	g_assert(config != NULL);
	g_assert(username != NULL);

	g_debug("Adding account \"%s\" to configuration", username);

	if(!(section = section_find_first_child(config_get_root(config), "Twitter")))
	{
		return FALSE;
	}

	if(!(section = section_find_first_child(section, "Accounts")))
	{
		return FALSE;
	}

	account = section_append_child(section, "Account");
	value_set_string(section_append_value(account, "username", VALUE_DATATYPE_STRING), username);

	return TRUE;
}

static void
_accounts_dialog_add_account(GtkWidget *dialog)
{
	_AccountsWindowPrivate *private;
	GList *accounts = NULL;
	gchar *username = NULL;
	GtkWidget *account_dialog;
	GdkPixbuf *pixbuf = NULL;
	Config *config;
	GtkWidget *dialog_message;
	gboolean success = FALSE;

	g_assert(GTK_IS_DIALOG(dialog));

	private = (_AccountsWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	/* run "add account" dialog */
	accounts = _accounts_dialog_get_accounts(private->tree_accounts);
	account_dialog = add_account_dialog_create(dialog, accounts);

	if(gtk_dialog_run(GTK_DIALOG(account_dialog)) == GTK_RESPONSE_OK)
	{
		/* get entered username */
		username = add_account_dialog_get_username(account_dialog);

		/* update configuration */
		config = mainwindow_lock_config(private->parent);
		success = _accounts_dialog_add_account_to_config(config, username);
		mainwindow_unlock_config(private->parent);

		if(success)
		{
			/* update treeview */
			g_debug("Adding account to list");
			pixbuf = _accounts_dialog_load_account_pixbuf();
			_accounts_dialog_accounts_tree_append(private->tree_accounts, username, pixbuf, FALSE);
			_accounts_dialog_accounts_tree_select(private->tree_accounts, username);
			_accounts_dialog_update_widgets(dialog);

			if(pixbuf)
			{
				gdk_pixbuf_unref(pixbuf);
			}
		}
		else
		{
			g_warning("Couldn't add account to configuration, settings file seems to be damaged");
			dialog_message = gtk_message_dialog_new(GTK_WINDOW(dialog),
			                                        GTK_DIALOG_MODAL,
			                                        GTK_MESSAGE_WARNING,
			                                        GTK_BUTTONS_OK,
			                                        _("Couldn't create dialog. Please check your configuration file."));
			gtk_dialog_run(GTK_DIALOG(dialog_message));
			gtk_widget_destroy(dialog_message);
		}
	}

	/* destroy dialog */
	gtk_widget_destroy(account_dialog);

	/* start account authentication */
	if(success)
	{
		_accounts_dialog_authorize_user(dialog, username);
	}

	/* free memory */
	if(accounts)
	{
		g_list_foreach(accounts, (GFunc)g_free, NULL);
		g_list_free(accounts);
	}

	if(username)
	{
		g_free(username);
	}
}

/*
 *	remove account:
 */
static void
_accounts_dialog_remove_account_from_config(Config *config, const gchar *username)
{
	Section *parent;
	GList *children;
	GList *iter;
	Value *value;

	g_assert(config != NULL);
	g_assert(username != NULL);

	g_debug("Removing account \"%s\" from configuration", username);

	if((children = section_select_children(config_get_root(config), "Twitter/Accounts/Account", &parent)))
	{
		iter = children;

		while(iter)
		{
			value = section_find_first_value((Section *)iter->data, "username");

			if(VALUE_IS_STRING(value))
			{
				if(!g_ascii_strcasecmp(value_get_string(value), username))
				{
					section_remove_child(parent, (Section *)iter->data);
				}
			}

			iter = g_list_next(iter);
		}

		g_list_free(children);
	}
}

static void
_accounts_dialog_remove_account(GtkWidget *dialog)
{
	_AccountsWindowPrivate *private;
	GtkWidget *dialog_message;
	gchar *username;
	gchar *escpaped;
	gchar *message;
	Config *config;

	g_assert(GTK_IS_DIALOG(dialog));

	private = (_AccountsWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	username = _accounts_dialog_get_selected_account(dialog);
	escpaped = g_markup_escape_text(username, -1);
	message = g_strdup_printf(_("Do you really want to delete the selected account (<b>%s</b>)?"), username);

	dialog_message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, NULL);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog_message), message);

	if(gtk_dialog_run(GTK_DIALOG(dialog_message)) == GTK_RESPONSE_YES)
	{
		/* remove account from configuration */
		config = mainwindow_lock_config(private->parent);
		_accounts_dialog_remove_account_from_config(config, username);
		mainwindow_unlock_config(private->parent);

		/* update tree */
		_accounts_dialog_remove_selected_row(private->tree_accounts);
		_accounts_dialog_update_widgets(dialog);
	}

	/* free memory */
	gtk_widget_destroy(dialog_message);

	if(escpaped)
	{
		g_free(escpaped);
	}

	g_free(username);
	g_free(message);
}

/*
 *	events:
 */
static void
_accounts_dialog_destroy(GtkDialog *dialog, gpointer user_data)
{
	g_assert(GTK_IS_DIALOG(dialog));

	g_free(g_object_get_data(G_OBJECT(dialog), "private"));
}

static void
_accounts_dialog_button_add_clicked(GtkWidget *widget, GtkWidget *dialog)
{
	g_assert(GTK_IS_BUTTON(widget));
	g_assert(GTK_IS_DIALOG(dialog));

	_accounts_dialog_add_account(dialog);
}

static void
_accounts_dialog_button_authenticate_clicked(GtkWidget *widget, GtkWidget *dialog)
{
	gchar *username;

	g_assert(GTK_IS_BUTTON(widget));
	g_assert(GTK_IS_DIALOG(dialog));

	if((username = _accounts_dialog_get_selected_account(dialog)))
	{
		_accounts_dialog_authorize_user(dialog, username);
		g_free(username);
	}
}

static void
_accounts_dialog_button_remove_clicked(GtkWidget *widget, GtkWidget *dialog)
{
	g_assert(GTK_IS_BUTTON(widget));
	g_assert(GTK_IS_DIALOG(dialog));

	_accounts_dialog_remove_account(dialog);
}

static void
_accounts_dialog_accounts_cursor_changed(GtkTreeView *tree, GtkWidget *dialog)
{
	g_assert(GTK_IS_TREE_VIEW(tree));
	g_assert(GTK_IS_DIALOG(dialog));

	_accounts_dialog_update_widgets(dialog);
}


/*
 *	public:
 */
GtkWidget *
accounts_dialog_create(GtkWidget *parent)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *area;
	_AccountsWindowPrivate *private = (_AccountsWindowPrivate *)g_malloc(sizeof(_AccountsWindowPrivate));

	g_assert(GTK_IS_WINDOW(parent));

	g_debug("Opening accounts dialog");

	private->parent = parent;

	dialog = gtk_dialog_new_with_buttons(_("Accounts"), GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* mainbox */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);

	/* create account list */
	frame = gtk_frame_new(_("Twitter accounts"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	private->tree_accounts = _accounts_dialog_create_tree();
	gtk_box_pack_start(GTK_BOX(vbox), private->tree_accounts, TRUE, TRUE, 5);

	/* insert buttons */
	area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));

	private->button_authenticate = gtk_button_new_with_label(_("Authorize"));
	gtk_box_pack_start(GTK_BOX(area), private->button_authenticate, FALSE, FALSE, 0);

	private->button_add = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(area), private->button_add, FALSE, FALSE, 0);

	private->button_remove = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	gtk_box_pack_start(GTK_BOX(area), private->button_remove, FALSE, FALSE, 0);

	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_CLOSE), GTK_RESPONSE_NONE);

	/* populate accounts */
	g_debug("Populating accounts list");
	_accounts_dialog_populate_accounts_tree(dialog, private->tree_accounts);

	/* update widgets */
	_accounts_dialog_update_widgets(dialog);

	/* set size & show widgets */
	gtk_widget_set_size_request(dialog, -1, 300);
	gtk_widget_show_all(dialog);

	/* signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_accounts_dialog_destroy), NULL);
	g_signal_connect(private->button_add, "clicked", G_CALLBACK(_accounts_dialog_button_add_clicked), dialog);
	g_signal_connect(private->button_authenticate, "clicked", G_CALLBACK(_accounts_dialog_button_authenticate_clicked), dialog);
	g_signal_connect(private->button_remove, "clicked", G_CALLBACK(_accounts_dialog_button_remove_clicked), dialog);
	g_signal_connect(private->tree_accounts, "cursor-changed", G_CALLBACK(_accounts_dialog_accounts_cursor_changed), dialog);

	return dialog;
}

/**
 * @}
 */


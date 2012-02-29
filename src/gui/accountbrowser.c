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
 * \file accountbrowser.c
 * \brief A tree containing accounts.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 29. February 2012
 */

#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include "accountbrowser.h"
#include "gtk_helpers.h"
#include "../pathbuilder.h"
#include "../twitter.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _AccountBrowserPrivate
 * \brief Private data data of the accountbrowser.
 */
typedef struct
{
	/*! The accountbrowser treeview. */
	GtkWidget *tree;
	/*! Various flags. */
	gint flags;
} _AccountBrowserPrivate;

enum
{
	ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
	ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
	ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE
};

enum
{
	ACCOUNTBROWSER_FLAG_SEARCH_NODE_CREATED = 0x01,
	ACCOUNTBROWSER_FLAG_TIMELINES_NODE_CREATED = 0x02
};

/*
 *	helpers:
 */
static GdkPixbuf *
_accountbrowser_load_pixbuf(const gchar *key)
{
	const gchar *filename;
	GdkPixbuf *pixbuf = NULL;
	GError *err = NULL;

	g_assert(key != NULL);

	if((filename = pathbuilder_load_path(key)))
	{
		if(!(pixbuf = gdk_pixbuf_new_from_file(filename, &err)))
		{
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
			else
			{
				g_warning("Couldn't load pixbuf: \"%s\"", filename);
			}
		}
	}
	else
	{
		g_warning("Couldn't find requested path: \"%s\"", key);
	}

	return pixbuf;
}

static gboolean
_accountbrowser_find_account_node(GtkTreeView *tree, GtkTreeIter *iter, const gchar *username)
{
	GtkTreeModel *model;
	gboolean found = FALSE;
	gboolean valid;
	gchar *text;
	AccountBrowserTreeViewNodeType type;

	g_assert(GTK_IS_TREE_VIEW(tree));
	g_assert(iter != NULL);
	g_assert(username != NULL);

	model = gtk_tree_view_get_model(tree);

	valid = gtk_tree_model_get_iter_first(model, iter);

	while(valid && !found)
	{
		gtk_tree_model_get(model, iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &text, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &type, -1);

		if(type == ACCOUNTBROWSER_TREEVIEW_NODE_ACCOUNT && !g_strcmp0(text, username))
		{
			found = TRUE;
		}
		else
		{
			valid = gtk_tree_model_iter_next(model, iter);
		}

		g_free(text);
	}

	return found;
}

static gboolean
_accountbrowser_find_account_child_node(GtkTreeView *tree, GtkTreeIter *parent, GtkTreeIter *iter, AccountBrowserTreeViewNodeType node_type)
{
	GtkTreeModel *model;
	gboolean valid;
	gboolean found = FALSE;
	AccountBrowserTreeViewNodeType type;

	g_assert(GTK_IS_TREE_VIEW(tree));
	g_assert(parent != NULL);
	g_assert(iter != NULL);

	model = gtk_tree_view_get_model(tree);

	if((valid = gtk_tree_model_iter_children(model, iter, parent)))
	{
		while(valid && !found)
		{
			gtk_tree_model_get(model, iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &type, -1);

			if(type == node_type)
			{
				found = TRUE;
			}
			else
			{
				valid = gtk_tree_model_iter_next(model, iter);
			}
		}
	}

	return found;
}

static void
_accountbrowser_remove_child_node(GtkTreeModel *model, GtkTreeIter *parent, const gchar *node_text)
{
	GtkTreeIter iter;
	gboolean found = FALSE;
	gboolean valid;
	gchar *text;

	valid = gtk_tree_model_iter_children(model, &iter, parent);

	while(valid && !found)
	{
		gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &text, -1);
	
		if(!g_ascii_strcasecmp(text, node_text))
		{
			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
			found = TRUE;
		}
		else
		{
			valid = gtk_tree_model_iter_next(model, &iter);
		}

		g_free(text);
	}
}

static void
_accountbrowser_insert_node_sorted(GtkTreeView *tree, GtkTreeIter *parent, AccountBrowserTreeViewNodeType node_type, const gchar *node_text, const gchar *node_icon, gboolean expand_parent)
{
	GtkTreeModel *model;
	GtkTreeStore *store;
	GtkTreeIter iter1;
	GtkTreeIter iter2;
	gchar *text;
	gboolean inserted = FALSE;
	gboolean found = FALSE;
	gboolean valid;
	gint result;
	GdkPixbuf *pixbuf;
	GtkTreePath *path;

	model = gtk_tree_view_get_model(tree);
	store = GTK_TREE_STORE(model);
	valid = gtk_tree_model_iter_children(model, &iter1, parent);

	/* try to find existing search query node or node position */
	while(valid && !inserted && !found)
	{
		gtk_tree_model_get(model, &iter1, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &text, -1);
	
		if((result = g_ascii_strcasecmp(node_text, text)) < 0)
		{
			/* insert node */
			gtk_tree_store_insert_before(store, &iter2, parent, &iter1);
			inserted = TRUE;
		}
		else if(!result)
		{
			found = TRUE;
		}
		else
		{
			valid = gtk_tree_model_iter_next(model, &iter1);
		}

		g_free(text);
	}

	/* create node */
	if(!inserted && !found)
	{
		gtk_tree_store_append(store, &iter2, parent);
	}

	if(!found)
	{
		pixbuf = _accountbrowser_load_pixbuf(node_icon);

		gtk_tree_store_set(store,
				   &iter2,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   node_text,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   node_type,
				   -1);

		g_object_unref(pixbuf);

		/* expand parent node */
		if(expand_parent)
		{
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), parent);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, TRUE);
			gtk_tree_path_free(path);
		}
	}
}

static gboolean
_accountbrowser_find_list_node(GtkTreeView *tree, GtkTreeIter *iter, const gchar *username, const gchar *listname)
{
	GtkTreeModel *model;
	GtkTreeIter account_iter;
	GtkTreeIter lists_iter;
	gboolean valid;
	gboolean found = FALSE;
	gchar *text;

	if(_accountbrowser_find_account_node(tree, &account_iter, username))
	{
		if(_accountbrowser_find_account_child_node(tree, &account_iter, &lists_iter, ACCOUNTBROWSER_TREEVIEW_NODE_LISTS))
		{
			model = gtk_tree_view_get_model(tree);
			valid = gtk_tree_model_iter_children(model, iter, &lists_iter);
		
			while(valid && !found)
			{
				gtk_tree_model_get(model, iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &text, -1);

				if(!g_ascii_strcasecmp(listname, text))
				{
					found = TRUE;
				}
				else
				{
					valid = gtk_tree_model_iter_next(model, iter);
				}

				g_free(text);
			}
		}
	}

	return found;
}

static GList *
_accountbrowser_get_lists(GtkWidget *widget, const gchar *username)
{
	_AccountBrowserPrivate *browser;
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeIter account_iter;
	GtkTreeIter lists_iter;
	GtkTreeIter iter;
	gboolean valid;
	gchar *text;
	GList *lists = NULL;

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);

	if(_accountbrowser_find_account_node(tree, &account_iter, username))
	{
		if(_accountbrowser_find_account_child_node(tree, &account_iter, &lists_iter, ACCOUNTBROWSER_TREEVIEW_NODE_LISTS))
		{
			model = gtk_tree_view_get_model(tree);
			valid = gtk_tree_model_iter_children(model, &iter, &lists_iter);
		
			while(valid)
			{
				gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &text, -1);
				lists = g_list_append(lists, text);
				valid = gtk_tree_model_iter_next(model, &iter);
			}
		}
	}

	return lists;
}

static void
_accountbrowser_update_global_node(GtkWidget *widget, AccountBrowserTreeViewNodeType type_id, const gchar *text, const gchar *icon, gint flag)
{
	GtkTreeView *tree;
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_VBOX(widget));

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);
	model = gtk_tree_view_get_model(tree);
	store = GTK_TREE_STORE(model);

	if(browser->flags & flag)
	{
		g_debug("Updating global node: \"%s\"", text);

		if(gtk_helpers_tree_model_find_iter_by_integer(model, type_id, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &iter))
		{
			gtk_tree_store_move_before(store, &iter, NULL);
		}
	}
	else
	{
		g_debug("Appending global node: \"%s\"", text);

		pixbuf = _accountbrowser_load_pixbuf(icon),
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store,
				   &iter,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   text,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   type_id,
				   -1);
		g_object_unref(pixbuf);

		browser->flags |= flag;
	}
}

static void
_accountbrowser_update_timelines_node(GtkWidget *widget)
{
	_accountbrowser_update_global_node(widget, ACCOUNTBROWSER_TREEVIEW_NODE_GLOBAL_TIMELINES, _("Timelines"), "icon_lists_16", ACCOUNTBROWSER_FLAG_TIMELINES_NODE_CREATED); // TODO: icon
}

static void
_accountbrowser_update_search_node(GtkWidget *widget)
{
	_accountbrowser_update_global_node(widget, ACCOUNTBROWSER_TREEVIEW_NODE_SEARCH, _("Search"), "icon_search_16", ACCOUNTBROWSER_FLAG_SEARCH_NODE_CREATED);
}

/*
 *	events:
 */
static void
_accountbrowser_treeview_node_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, GtkWidget *mainwindow)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	AccountBrowserTreeViewNodeType type;
	gchar *text;
	GtkTreeIter temp;
	AccountBrowserTreeViewNodeType parent_type;
	gchar *account;

	g_assert(GTK_IS_TREE_VIEW(tree));
	g_assert(path != NULL);
	g_assert(GTK_IS_TREE_VIEW_COLUMN(column));
	g_assert(GTK_IS_WINDOW(mainwindow));

	model = gtk_tree_view_get_model(tree);

	g_debug("%s: handling node activation", __func__);

	/* get selected node & values */
	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &type, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &text, -1);

		if(type == ACCOUNTBROWSER_TREEVIEW_NODE_SEARCH || type == ACCOUNTBROWSER_TREEVIEW_NODE_SEARCH_QUERY ||
		   type == ACCOUNTBROWSER_TREEVIEW_NODE_GLOBAL_TIMELINES || type == ACCOUNTBROWSER_TREEVIEW_NODE_GLOBAL_USER_TIMELINE)
		{
			mainwindow_account_node_activated(mainwindow, NULL, type, text);
			g_free(text);
		}
		else if(type == ACCOUNTBROWSER_TREEVIEW_NODE_ACCOUNT)
		{
			/* call mainwindow handler & free memory */
			mainwindow_account_node_activated(mainwindow, text, type, text);
			g_free(text);
		}
		else
		{
			g_debug("Searching for related account node");

			/* try to get related account node */
			parent_type = type;
			while(parent_type != ACCOUNTBROWSER_TREEVIEW_NODE_ACCOUNT)
			{
				temp = iter;
				gtk_tree_model_iter_parent(model, &iter, &temp);
				gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &parent_type, -1);
			}

			gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &account, -1);

			/* call mainwindow handler & free memory */
			mainwindow_account_node_activated(mainwindow, account, type, text);
			g_free(account);
			g_free(text);
		}
	}
	else
	{
		g_warning("%s: couldn't get acticated node", __func__);
	}
}

static gboolean
_accountbrowser_treeview_button_press_event(GtkWidget *tree, GdkEventButton *event, GtkWidget *mainwindow)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	AccountBrowserTreeViewNodeType type;
	GtkTreePath *path;
	gchar *text = NULL;
	GtkTreeIter temp;
	AccountBrowserTreeViewNodeType parent_type;
	gchar *account = NULL;

	g_assert(GTK_IS_TREE_VIEW(tree));
	g_assert(GTK_IS_WINDOW(mainwindow));

	/* test button */
	if(event->button != 3)
	{
		return FALSE;
	}

	g_debug("%s: handling right-click", __func__);

	/* get node */
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), event->x, event->y, &path, NULL, NULL, NULL))
	{
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

		if(gtk_tree_model_get_iter(model, &iter, path))
		{
			gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &type, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &text, -1);

			/* find related account node */
			g_debug("Searching for related account node");
			parent_type = type;
			while(parent_type != ACCOUNTBROWSER_TREEVIEW_NODE_ACCOUNT)
			{
				temp = iter;
				gtk_tree_model_iter_parent(model, &iter, &temp);
				gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &parent_type, -1);
			}

			gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &account, -1);

			/* mainwindow handler */
			mainwindow_account_node_right_clicked(mainwindow, event->time, account, type, text);

			/* cleanup */
			gtk_tree_path_free(path);
		}
	}
	else
	{
		g_warning("Couldn't get selected node.");
	}

	/* cleanup */
	g_free(account);
	g_free(text);

	return FALSE;
}

/*
 *	public:
 */
GtkWidget *
accountbrowser_create(GtkWidget *parent)
{
	GtkWidget *vbox;
	GtkWidget *tree;
	GtkWidget *scroll;
	GtkTreeStore *treestore;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_WINDOW(parent));

	g_debug("Creating accountbrowser");

	vbox = gtk_vbox_new(FALSE, 0);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
 
 	tree = gtk_tree_view_new();
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);
	gtk_container_add(GTK_CONTAINER(scroll), tree);

	treestore = gtk_tree_store_new(3, G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_INT);

	/* first treeview column: icon, text */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Accounts"));

	/* pixmap renderer */
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF);

	/* text renderer */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT);

	/* append column */
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	/* second treeview column: type */
	column = gtk_tree_view_column_new();

	/* text renderer */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE);

	/* append column */
	gtk_tree_view_column_set_visible(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	/* third treeview column: type */
	column = gtk_tree_view_column_new ();

	/* set model */
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(treestore));

	/* create accountbrowser structure */
	browser = (_AccountBrowserPrivate *)g_malloc(sizeof(_AccountBrowserPrivate));
	browser->tree = tree;
	browser->flags = 0;
	g_object_set_data(G_OBJECT(vbox), "browser", browser);

	/* build paths */
	g_debug("Building icon paths");
	pathbuilder_save_path("icon_twitter_16", pathbuilder_build_icon_path("16x16", "twitter.png"));
	pathbuilder_save_path("icon_message_16", pathbuilder_build_icon_path("16x16", "messages.png"));
	pathbuilder_save_path("icon_direct_message_16", pathbuilder_build_icon_path("16x16", "direct_messages.png"));
	pathbuilder_save_path("icon_replies_16", pathbuilder_build_icon_path("16x16", "replies.png"));
	pathbuilder_save_path("icon_usertimeline_16", pathbuilder_build_icon_path("16x16", "usertimeline.png"));
	pathbuilder_save_path("icon_lists_16", pathbuilder_build_icon_path("16x16", "lists.png"));
	pathbuilder_save_path("icon_friends_16", pathbuilder_build_icon_path("16x16", "friends.png"));
	pathbuilder_save_path("icon_followers_16", pathbuilder_build_icon_path("16x16", "followers.png"));
	pathbuilder_save_path("icon_list_16", pathbuilder_build_icon_path("16x16", "list.png"));
	pathbuilder_save_path("icon_list_protected_16", pathbuilder_build_icon_path("16x16", "list_protected.png"));
	pathbuilder_save_path("icon_search_16", pathbuilder_build_icon_path("16x16", "search.png"));

	/* signals */
	g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(_accountbrowser_treeview_node_activated), (gpointer)parent);
	g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(_accountbrowser_treeview_button_press_event), (gpointer)parent);

	/* free data */
	g_object_unref(treestore);

	return vbox;
}

void
accountbrowser_destroy(GtkWidget *widget)
{
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_VBOX(widget));

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	g_free(browser);
}

void
accountbrowser_append_account(GtkWidget *widget, const gchar *username)
{
	GtkTreeView *tree;
	GtkTreeStore *store;
	GtkTreeIter parent;
	GtkTreeIter iter;
	GtkTreePath *path;
	GdkPixbuf *pixbuf;
	_AccountBrowserPrivate *browser;

	/* insert search node */
	g_assert(GTK_IS_VBOX(widget));
	g_assert(username != NULL);

	g_debug("Appending account (\"%s\") to accountbrowser", username);

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);

	/* check if node does already exist */
	if(!_accountbrowser_find_account_node(tree, &iter, username))
	{
		store = GTK_TREE_STORE(gtk_tree_view_get_model(tree));

		/* create root node */
		pixbuf = _accountbrowser_load_pixbuf("icon_twitter_16");
		gtk_tree_store_append(store, &parent, NULL);
		gtk_tree_store_set(store,
				   &parent,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   username,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   ACCOUNTBROWSER_TREEVIEW_NODE_ACCOUNT,
				   -1);
		g_object_unref(pixbuf);	

		/* create child nodes */
		pixbuf = _accountbrowser_load_pixbuf("icon_message_16");
		gtk_tree_store_append(store, &iter, &parent);
		gtk_tree_store_set(store,
				   &iter,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   _("Messages"),
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   ACCOUNTBROWSER_TREEVIEW_NODE_MESSAGES,
				   -1);
		g_object_unref(pixbuf);

		pixbuf =_accountbrowser_load_pixbuf("icon_replies_16"),
		gtk_tree_store_append(store, &iter, &parent);
		gtk_tree_store_set(store,
				   &iter,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   _("Replies"),
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   ACCOUNTBROWSER_TREEVIEW_NODE_REPLIES,
				   -1);
		g_object_unref(pixbuf);

		/*
		pixbuf =_accountbrowser_load_pixbuf("icon_direct_message_16"),
		gtk_tree_store_append(store, &iter, &parent);
		gtk_tree_store_set(store,
				   &iter,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   _("Direct Messages"),
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   ACCOUNTBROWSER_TREEVIEW_NODE_DIRECT_MESSAGES,
				   -1);
		g_object_unref(pixbuf);
		*/

		pixbuf = _accountbrowser_load_pixbuf("icon_usertimeline_16"),
		gtk_tree_store_append(store, &iter, &parent);
		gtk_tree_store_set(store,
				   &iter,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf, 
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   _("Updates"),
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   ACCOUNTBROWSER_TREEVIEW_NODE_USER_TIMELINE,
				   -1);
		g_object_unref(pixbuf);

		pixbuf = _accountbrowser_load_pixbuf("icon_lists_16"),
		gtk_tree_store_append(store, &iter, &parent);
		gtk_tree_store_set(store,
				   &iter,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   _("Lists"),
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   ACCOUNTBROWSER_TREEVIEW_NODE_LISTS,
				   -1);
		g_object_unref(pixbuf);

		pixbuf = _accountbrowser_load_pixbuf("icon_friends_16"),
		gtk_tree_store_append(store, &iter, &parent);
		gtk_tree_store_set(store,
				   &iter,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   _("Friends"),
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   ACCOUNTBROWSER_TREEVIEW_NODE_FRIENDS,
				   -1);
		g_object_unref(pixbuf);

		pixbuf = _accountbrowser_load_pixbuf("icon_followers_16"),
		gtk_tree_store_append(store, &iter, &parent);
		gtk_tree_store_set(store,
				   &iter,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
				   pixbuf,
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
				   _("Followers"),
				   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
				   ACCOUNTBROWSER_TREEVIEW_NODE_FOLLOWERS,
				   -1);
		g_object_unref(pixbuf);

		/* expand parent node */
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &parent);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(browser->tree), path, TRUE);
		gtk_tree_path_free(path);

		/* update search & timelines node */
		_accountbrowser_update_search_node(widget);
		_accountbrowser_update_timelines_node(widget);
	}
}

void
accountbrowser_remove_account(GtkWidget *widget, const gchar *username)
{
	GtkTreeView *tree;
	GtkTreeIter iter;
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_VBOX(widget));
	g_assert(username != NULL);

	g_debug("Removing account (\"%s\") from accountbrowser", username);

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);

	if(_accountbrowser_find_account_node(tree, &iter, username))
	{
		gtk_tree_store_remove(GTK_TREE_STORE(gtk_tree_view_get_model(tree)), &iter);
	}
}

GList *
accountbrowser_get_accounts(GtkWidget *widget)
{
	_AccountBrowserPrivate *browser;
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *account;
	AccountBrowserTreeViewNodeType type;
	gboolean valid;
	GList *accounts = NULL;

	g_assert(GTK_IS_VBOX(widget));

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);
	model = gtk_tree_view_get_model(tree);

	valid = gtk_tree_model_get_iter_first(model, &iter);

	while(valid)
	{
		gtk_tree_model_get(model, &iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &account, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &type, -1);

		if(type == ACCOUNTBROWSER_TREEVIEW_NODE_ACCOUNT)
		{
			accounts = g_list_append(accounts, account);
		}
		else
		{
			g_free(account);
		}

		valid = gtk_tree_model_iter_next(model, &iter);
	}

	return accounts;
}

void
accountbrowser_append_list(GtkWidget *widget, const gchar * restrict username, const gchar * restrict listname, gboolean protected)
{
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeStore *store;
	GtkTreeIter parent;
	GtkTreeIter child;
	GtkTreeIter iter;
	GtkTreeIter list_iter;
	gboolean inserted = FALSE;
	gboolean found = FALSE;
	gchar *text;
	gboolean valid;
	gint result;
	GdkPixbuf *pixbuf;
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_VBOX(widget));
	g_assert(username != NULL);
	g_assert(listname != NULL);

	g_debug("Appending list (\"%s@%s\") accountbrowser", username, listname);

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);

	if(_accountbrowser_find_account_node(tree, &parent, username))
	{
		if(_accountbrowser_find_account_child_node(tree, &parent, &child, ACCOUNTBROWSER_TREEVIEW_NODE_LISTS))
		{
			model = gtk_tree_view_get_model(tree);
			store = GTK_TREE_STORE(model);
			valid = gtk_tree_model_iter_children(model, &list_iter, &child);

			while(valid && !inserted && !found)
			{
				gtk_tree_model_get(model, &list_iter, ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT, &text, -1);
			
				if((result = g_ascii_strcasecmp(listname, text)) < 0)
				{
					gtk_tree_store_insert_before(store, &iter, &child, &list_iter);
					inserted = TRUE;
				}
				else if(!result)
				{
					found = TRUE;
				}
				else
				{
					valid = gtk_tree_model_iter_next(model, &list_iter);
				}

				g_free(text);
			}

			/* create node */
			if(!inserted && !found)
			{
				gtk_tree_store_append(store, &iter, &child);
			}

			pixbuf = protected ? _accountbrowser_load_pixbuf("icon_list_protected_16") : _accountbrowser_load_pixbuf("icon_list_16");
		
			if(found)
			{
				gtk_tree_store_set(store,
						   &list_iter,
						   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
						   pixbuf,
						   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
						   protected ? ACCOUNTBROWSER_TREEVIEW_NODE_PROTECTED_LIST : ACCOUNTBROWSER_TREEVIEW_NODE_LIST,
						   -1);
			}
			else
			{
				gtk_tree_store_set(store,
						   &iter,
						   ACCOUNTBROWSER_TREEVIEW_COLUMN_PIXBUF,
						   pixbuf,
						   ACCOUNTBROWSER_TREEVIEW_COLUMN_TEXT,
						   listname,
						   ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE,
						   protected ? ACCOUNTBROWSER_TREEVIEW_NODE_PROTECTED_LIST : ACCOUNTBROWSER_TREEVIEW_NODE_LIST,
						   -1);
			}

			g_object_unref(pixbuf);
		}
	}
}

void
accountbrowser_remove_list(GtkWidget *widget, const gchar * restrict username, const gchar * restrict listname)
{
	_AccountBrowserPrivate *browser;
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeStore *store;
	GtkTreeIter iter;

	g_assert(GTK_IS_VBOX(widget));
	g_assert(username != NULL);
	g_assert(listname != NULL);

	g_debug("Removing list (\"%s@%s\") accountbrowser", username, listname);

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);
	
	if(_accountbrowser_find_list_node(tree, &iter, username, listname))
	{
		model = gtk_tree_view_get_model(tree);
		store = GTK_TREE_STORE(model);
		gtk_tree_store_remove(store, &iter);
	}
}

void
accountbrowser_update_list(GtkWidget *widget, const gchar * restrict owner, const gchar * restrict old_name, const gchar * restrict new_name, gboolean protected)
{
	accountbrowser_remove_list(widget, owner, old_name);
	accountbrowser_append_list(widget, owner, new_name, protected);
}

void
accountbrowser_set_lists(GtkWidget *widget, const gchar *username, GList *lists)
{
	GList *tree_lists;
	GList *el1 = lists;
	GList *el2;
	gboolean found;
	TwitterList *list;

	/* remove deleted lists */
	if((el1 = tree_lists = _accountbrowser_get_lists(widget, username)))
	{
		while(el1)
		{
			el2 = lists;
			found = FALSE;

			while(el2 && !found)
			{
				list = (TwitterList *)el2->data;

				if(!g_ascii_strcasecmp((const gchar *)el1->data, list->name))
				{
					found = TRUE;
				}

				el2 = el2->next;
			}
		
			if(!found)
			{
				accountbrowser_remove_list(widget, username, (gchar *)el1->data);
			}

			g_free(el1->data);
			el1 = el1->next;
		}
	
		g_list_free(tree_lists);
	}

	/* insert lists */
	el1 = lists;

	while(el1)
	{
		list = (TwitterList *)el1->data;
		accountbrowser_append_list(widget, username, list->name, list->protected);
		el1 = el1->next;
	}
}

void
accountbrowser_append_search_query(GtkWidget *widget, const gchar *query)
{
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeIter parent;
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_VBOX(widget));
	g_assert(query != NULL);

	g_debug("Appending search query: \"%s\"", query);

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(browser->tree));

	if(gtk_helpers_tree_model_find_iter_by_integer(model, ACCOUNTBROWSER_TREEVIEW_NODE_SEARCH, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &parent))
	{
		_accountbrowser_insert_node_sorted(tree, &parent, ACCOUNTBROWSER_TREEVIEW_NODE_SEARCH_QUERY, query, "icon_search_16", TRUE);
	}
}

void
accountbrowser_remove_search_query(GtkWidget *widget, const gchar *query)
{
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeIter parent;
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_VBOX(widget));
	g_assert(query != NULL);

	g_debug("Removing search query: \"%s\"", query);

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(browser->tree));

	if(gtk_helpers_tree_model_find_iter_by_integer(model, ACCOUNTBROWSER_TREEVIEW_NODE_SEARCH, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &parent))
	{
		_accountbrowser_remove_child_node(model, &parent, query);
	}
}

void
accountbrowser_append_user_timeline(GtkWidget *widget, const gchar *user)
{
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeIter parent;
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_VBOX(widget));
	g_assert(user != NULL);

	g_debug("Appending user timeline: \"%s\"", user);

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(browser->tree));

	if(gtk_helpers_tree_model_find_iter_by_integer(model, ACCOUNTBROWSER_TREEVIEW_NODE_GLOBAL_TIMELINES, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &parent))
	{
		_accountbrowser_insert_node_sorted(tree, &parent, ACCOUNTBROWSER_TREEVIEW_NODE_GLOBAL_USER_TIMELINE, user, "icon_usertimeline_16", TRUE);
	}
}

void
accountbrowser_remove_user_timeline(GtkWidget *widget, const gchar *user)
{
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeIter parent;
	_AccountBrowserPrivate *browser;

	g_assert(GTK_IS_VBOX(widget));
	g_assert(user != NULL);

	g_debug("Removing user timeline: \"%s\"", user);

	browser = (_AccountBrowserPrivate *)g_object_get_data(G_OBJECT(widget), "browser");
	tree = GTK_TREE_VIEW(browser->tree);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(browser->tree));

	if(gtk_helpers_tree_model_find_iter_by_integer(model, ACCOUNTBROWSER_TREEVIEW_NODE_GLOBAL_TIMELINES, ACCOUNTBROWSER_TREEVIEW_COLUMN_TYPE, &parent))
	{
		_accountbrowser_remove_child_node(model, &parent, user);
	}
}

/**
 * @}
 */


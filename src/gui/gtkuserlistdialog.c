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
 * \file gtkuserlistdialog.c
 * \brief A dialog containing an user list.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2011
 */

#include <glib/gi18n.h>

#include "gtkuserlistdialog.h"
#include "gtk_helpers.h"

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Widgets
 * 	@{
 */

static GObject *_gtk_user_list_dialog_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params);

/*! Define GtkUserListDialog. */
G_DEFINE_TYPE(GtkUserListDialog, gtk_user_list_dialog, GTK_TYPE_DELETABLE_DIALOG);

enum
{
	PROP_0,
	PROP_CHECKBOX_COLUMN_VISIBLE,
	PROP_CHECKBOX_COLUMN_ACTIVATABLE,
	PROP_CHECKBOX_COLUMN_TITLE,
	PROP_USERNAME_COLUMN_TITLE,
	PROP_SHOW_USER_COUNT
};

enum
{
	GTK_USER_LIST_TREEVIEW_COLUMN_CHECKBOX,
	GTK_USER_LIST_TREEVIEW_COLUMN_PIXBUF,
	GTK_USER_LIST_TREEVIEW_COLUMN_USERNAME
};

/**
 * \struct _GtkUserListDialogPrivate
 * \brief Private _GtkUserListDialog data.
 */
struct _GtkUserListDialogPrivate
{
	/*! TRUE to show checkboxes. */
	gboolean checkbox_column_visible;
	/*! TRUE to make the list activatable. */
	gboolean checkbox_column_activatable;
	/*! Title of the checkbox column. */
	gchar *checkbox_column_title;
	/*! Title of the username column. */
	gchar *username_column_title;
	/*! Show user counter. */
	gboolean show_user_count;
	/*! Tree containing users. */
	GtkWidget *tree;
	/*! Number of users. */
	gint count;
	/*! Label displaying the number of users. */
	GtkWidget *label_count;
};

/*
 *	treeview:
 */
static void
_gtk_user_list_dialog_update_tree_columns(GtkWidget *widget)
{
	GtkUserListDialogPrivate *priv;
	GtkTreeView *tree;
	GtkTreeViewColumn *column;

	g_assert(GTK_IS_DELETABLE_DIALOG(widget));

	priv = GTK_USER_LIST_DIALOG(widget)->priv;
	tree = GTK_TREE_VIEW(priv->tree);

	column = gtk_tree_view_get_column(tree, 0);
	gtk_tree_view_column_set_visible(column, priv->checkbox_column_visible);
	gtk_tree_view_column_set_title(column, priv->checkbox_column_title);

	column = gtk_tree_view_get_column(tree, 1);
	gtk_tree_view_column_set_title(column, priv->username_column_title);
}

static void
_gtk_user_list_dialog_tree_checkbox_toggled(GtkCellRendererToggle *renderer, gchar *path, GtkTreeView *widget)
{
	GtkUserListDialogPrivate *priv;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeIter child;

	g_assert(GTK_IS_DELETABLE_DIALOG(widget));

	priv = GTK_USER_LIST_DIALOG(widget)->priv;

	if(priv->checkbox_column_activatable)
	{
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->tree));

		if(gtk_tree_model_get_iter_from_string(model, &iter, path))
		{
			gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(model), &child, &iter);
			model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(model));
			gtk_list_store_set(GTK_LIST_STORE(model),
							  &child,
							  GTK_USER_LIST_TREEVIEW_COLUMN_CHECKBOX,
							  gtk_cell_renderer_toggle_get_active(renderer) ? FALSE : TRUE,
							  -1);
		}
	}
}

static GtkWidget *
_gtk_user_list_dialog_create_tree(GtkWidget *widget)
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

	store = gtk_list_store_new(3, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), GTK_USER_LIST_TREEVIEW_COLUMN_USERNAME, GTK_SORT_ASCENDING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);

	/* insert column to display checkbox */
	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_toggle_new();
	gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
	g_signal_connect(G_OBJECT(renderer), "toggled", (GCallback)_gtk_user_list_dialog_tree_checkbox_toggled, widget);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "active", GTK_USER_LIST_TREEVIEW_COLUMN_CHECKBOX);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* insert column to display icon & text */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "User");

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", GTK_USER_LIST_TREEVIEW_COLUMN_PIXBUF);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", GTK_USER_LIST_TREEVIEW_COLUMN_USERNAME);

	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	/* free data */
	g_object_unref(store);
	g_object_unref(model);

	return tree;
}

static void
_gtk_user_list_dialog_append_node(GtkWidget *tree, const gchar *username, GdkPixbuf *pixbuf, gboolean checked)
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
                           GTK_USER_LIST_TREEVIEW_COLUMN_CHECKBOX,
                           checked,
                           GTK_USER_LIST_TREEVIEW_COLUMN_USERNAME,
                           username,
	                   GTK_USER_LIST_TREEVIEW_COLUMN_PIXBUF,
	                   pixbuf,
                           -1);
}

/*
 *	implementation:
 */
static void
_gtk_user_list_dialog_append_user(GtkUserListDialog *dialog, const gchar *username, GdkPixbuf *pixbuf, gboolean checked)
{
	GtkUserListDialogPrivate *priv = dialog->priv;
	gchar text[32];

	/* add user to tree */
	_gtk_user_list_dialog_append_node(priv->tree, username, pixbuf, checked);

	/* update label */
	++priv->count;
	sprintf(text, _("<b>Users:</b> %d"), priv->count);
	gtk_label_set_markup(GTK_LABEL(priv->label_count), text);
}

static void
_gtk_user_list_dialog_set_user_pixbuf(GtkUserListDialog *dialog, const gchar *username, GdkPixbuf *pixbuf)
{
	GtkUserListDialogPrivate *priv = dialog->priv;
	GtkWidget *tree;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeIter child;
	GtkListStore *store;

	tree = priv->tree;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

	if(gtk_helpers_tree_model_find_iter_by_string(model, username, GTK_USER_LIST_TREEVIEW_COLUMN_USERNAME, &iter))
	{
		gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(model), &child, &iter);
		store = GTK_LIST_STORE(gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(model)));
		gtk_list_store_set(store, &child, GTK_USER_LIST_TREEVIEW_COLUMN_PIXBUF, pixbuf, -1);
	}
}

static GList *
_gtk_user_list_dialog_get_users(GtkUserListDialog *dialog, gboolean checked)
{
	GtkUserListDialogPrivate *priv = dialog->priv;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean loop;
	gchar *username;
	gboolean activated;
	GList *users = NULL;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->tree));
	loop = gtk_tree_model_get_iter_first(model, &iter);

	while(loop)
	{
		gtk_tree_model_get(model, &iter, GTK_USER_LIST_TREEVIEW_COLUMN_USERNAME, &username, GTK_USER_LIST_TREEVIEW_COLUMN_CHECKBOX, &activated, -1);

		if(activated == checked)
		{
			users = g_list_append(users, username);
		}
		else
		{
			g_free(username);
		}

		loop = gtk_tree_model_iter_next(model, &iter);
	}

	return users;
}

static void
_gtk_user_list_dialog_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GtkUserListDialog *dialog = GTK_USER_LIST_DIALOG(object);

	switch(prop_id)
	{
		case PROP_CHECKBOX_COLUMN_VISIBLE:
			g_value_set_boolean(value, dialog->priv->checkbox_column_visible);
			break;

		case PROP_CHECKBOX_COLUMN_ACTIVATABLE:
			g_value_set_boolean(value, dialog->priv->checkbox_column_activatable);
			break;

		case PROP_CHECKBOX_COLUMN_TITLE:
			g_value_set_string(value, dialog->priv->checkbox_column_title);
			break;

		case PROP_USERNAME_COLUMN_TITLE:
			g_value_set_string(value, dialog->priv->username_column_title);
			break;

		case PROP_SHOW_USER_COUNT:
			g_value_set_boolean(value, dialog->priv->show_user_count);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
_gtk_user_list_dialog_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GtkUserListDialog *dialog = GTK_USER_LIST_DIALOG(object);

	switch(prop_id)
	{
		case PROP_CHECKBOX_COLUMN_VISIBLE:
			dialog->priv->checkbox_column_visible = g_value_get_boolean(value);
			_gtk_user_list_dialog_update_tree_columns(GTK_WIDGET(dialog));
			break;

		case PROP_CHECKBOX_COLUMN_ACTIVATABLE:
			dialog->priv->checkbox_column_activatable = g_value_get_boolean(value);
			break;

		case PROP_CHECKBOX_COLUMN_TITLE:
			g_free(dialog->priv->checkbox_column_title);
			dialog->priv->checkbox_column_title = g_value_dup_string(value);
			_gtk_user_list_dialog_update_tree_columns(GTK_WIDGET(dialog));
			break;

		case PROP_USERNAME_COLUMN_TITLE:
			g_free(dialog->priv->username_column_title);
			dialog->priv->username_column_title = g_value_dup_string(value);
			_gtk_user_list_dialog_update_tree_columns(GTK_WIDGET(dialog));
			break;

		case PROP_SHOW_USER_COUNT:
			dialog->priv->show_user_count = g_value_get_boolean(value);
			gtk_widget_set_visible(dialog->priv->label_count, dialog->priv->show_user_count);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/*
 *	public:
 */
void
gtk_user_list_dialog_append_user(GtkUserListDialog *dialog, const gchar *username, GdkPixbuf *pixbuf, gboolean checked)
{
	GTK_USER_LIST_DIALOG_GET_CLASS(dialog)->append_user(dialog, username, pixbuf, checked);
}

void
gtk_user_list_dialog_set_user_pixbuf(GtkUserListDialog *dialog, const gchar *username, GdkPixbuf *pixbuf)
{
	GTK_USER_LIST_DIALOG_GET_CLASS(dialog)->set_user_pixbuf(dialog, username, pixbuf);
}

GList *
gtk_user_list_dialog_get_users(GtkUserListDialog *dialog, gboolean checked)
{
	return GTK_USER_LIST_DIALOG_GET_CLASS(dialog)->get_users(dialog, checked);
}

GtkWidget *
gtk_user_list_dialog_new(GtkWindow *parent, const gchar *title, gboolean modal)
{
	GtkUserListDialog *dialog;

	dialog = (GtkUserListDialog *)g_object_new(GTK_USER_LIST_DIALOG_TYPE, NULL);

	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	gtk_window_set_modal(GTK_WINDOW(dialog), modal);

	return GTK_WIDGET(dialog);
}

/*
 *	initialization/finalization:
 */
static GObject *
_gtk_user_list_dialog_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params)
{
	GObject *object;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *scrolled;
	GtkUserListDialogPrivate *priv;

	object = G_OBJECT_CLASS(gtk_user_list_dialog_parent_class)->constructor(type, n_construct_properties, construct_params);

	gtk_widget_push_composite_child();

	priv = GTK_USER_LIST_DIALOG(object)->priv;

	/* create main box */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(object))), hbox);

	/* create frame */
	frame = gtk_frame_new(_("Users"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 2);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 2);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 2);

	/* create tree */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

	priv->tree = _gtk_user_list_dialog_create_tree(GTK_WIDGET(object));
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), priv->tree);

	/* label */
	priv->label_count = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(vbox), priv->label_count, FALSE, TRUE, 2);

	/* show widgets */
	gtk_widget_show_all(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(object)));
	gtk_widget_show_all(gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(object)));

	gtk_widget_push_composite_child();

	return object;
}

static void
_gtk_user_list_dialog_finalize(GObject *object)
{
	if(G_OBJECT_CLASS(gtk_user_list_dialog_parent_class)->finalize)
	{
		(*G_OBJECT_CLASS(gtk_user_list_dialog_parent_class)->finalize)(object);
	}
}

static void
gtk_user_list_dialog_class_init(GtkUserListDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(GtkUserListDialogClass));

	gobject_class->constructor= _gtk_user_list_dialog_constructor;
	gobject_class->finalize = _gtk_user_list_dialog_finalize;
	gobject_class->get_property = _gtk_user_list_dialog_get_property;
	gobject_class->set_property = _gtk_user_list_dialog_set_property;

	klass->append_user = _gtk_user_list_dialog_append_user;
	klass->set_user_pixbuf = _gtk_user_list_dialog_set_user_pixbuf;
	klass->get_users = _gtk_user_list_dialog_get_users;

	g_object_class_install_property(gobject_class, PROP_CHECKBOX_COLUMN_VISIBLE,
	                                g_param_spec_boolean("checkbox-column-visible", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class, PROP_CHECKBOX_COLUMN_ACTIVATABLE,
	                                g_param_spec_boolean("checkbox-column-activatable", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class, PROP_CHECKBOX_COLUMN_TITLE,
	                                g_param_spec_string("checkbox-column-title", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class, PROP_USERNAME_COLUMN_TITLE,
	                                g_param_spec_string("username-column-title", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class, PROP_SHOW_USER_COUNT,
	                                g_param_spec_boolean("show-user-count", NULL, NULL, FALSE, G_PARAM_READWRITE));
}

static void
gtk_user_list_dialog_init(GtkUserListDialog *userlistdialog)
{
	/* register private data */
	userlistdialog->priv = G_TYPE_INSTANCE_GET_PRIVATE(userlistdialog, GTK_USER_LIST_DIALOG_TYPE, GtkUserListDialogPrivate);
}

/**
 * @}
 * @}
 */


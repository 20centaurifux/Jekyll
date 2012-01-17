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
 * \file composer_dialog.c
 * \brief A dialog for composing messages.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 13. January 2012
 */

#include <string.h>
#include "composer_dialog.h"
#include "gtkdeletabledialog.h"
#include "mainwindow.h"
#include "../pathbuilder.h"
#include "../twitterdb.h"

/**
 * @addtogroup Gui
 * @{
 */

enum
{
	COMPOSER_DIALOG_TREEVIEW_COLUMN_PIXBUF,
	COMPOSER_DIALOG_TREEVIEW_COLUMN_USERNAME
};

/**
 * \struct _ComposerWindowPrivate
 * \brief Private data of the "composer" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! Unique name of the pixbuf group. */
	gchar pixbuf_group[64];
	/*! The previous status. */
	gchar *prev_status;
	/*! Comboxbox containing users. */
	GtkWidget *combo_users;
	/*! Text field. */
	GtkWidget *view;
	/*! Character count. */
	GtkWidget *label_count;
	/*! "Apply" button. */
	GtkWidget *button_ok;
	/*! "Apply" callback. */
	ComposerApplyCallback callback;
	/*! "Apply" callback user data. */
	gpointer user_data;
} _ComposerWindowPrivate;

/**
 * \struct _ComposerWindowPixbufArg
 * \brief Holds an username and a GdkPixbuf to set.
 */
typedef struct
{
	/*! A dialog. */
	GtkWidget *dialog;
	/*! An username. */
	gchar username[64];
	/*! A pixbuf. */
	GdkPixbuf *pixbuf;
}
_ComposerWindowPixbufArg;

/*
 *	helpers:
 */
static void
_composer_dialog_update_remaining_characters(GtkWidget *widget)
{
	_ComposerWindowPrivate *private = (_ComposerWindowPrivate *)g_object_get_data(G_OBJECT(widget), "private");
	GtkTextBuffer *buffer;
	gint count;
	gchar *text;
	PangoAttrList *attrs;
	PangoAttrIterator *iter;
	PangoAttrColor *attr = NULL;

	/* update text */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(private->view));
	count = gtk_text_buffer_get_char_count(buffer);
	text = g_strdup_printf("Remaining characters: %d", 140 - count);
	gtk_label_set_text(GTK_LABEL(private->label_count), text);
	g_free(text);

	/* update attributes */
	if((attrs = gtk_label_get_attributes(GTK_LABEL(private->label_count))))
	{
		iter = pango_attr_list_get_iterator(attrs);
		attr = (PangoAttrColor *)pango_attr_iterator_get(iter, PANGO_ATTR_FOREGROUND);
		pango_attr_iterator_destroy(iter);

		if(count >= 140)
		{
			attr->color.red = 0xEA60;
			attr->color.green = 0x2710;
			attr->color.blue = 0x2710;
		}
		else
		{
			attr->color.red = 0x2710;
			attr->color.green = 0x2710;
			attr->color.blue = 0x9C40;
		}

		gtk_label_set_attributes(GTK_LABEL(private->label_count), attrs);
		gtk_widget_set_sensitive(private->button_ok, count > 140 ? FALSE : TRUE);
	}
}

/*
 *	events:
 */
gboolean
_composer_dialog_apply_worker(GtkWidget *dialog)
{
	_ComposerWindowPrivate *private = (_ComposerWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	gchar *user = NULL;
	gchar *text = NULL;
	gint response = GTK_RESPONSE_APPLY;

	g_assert(private->callback != NULL);

	if((user = composer_dialog_get_user(dialog)))
	{
		if((text = composer_dialog_get_text(dialog)) && strlen(text))
		{
			if(!private->callback(user, text, private->prev_status, private->user_data))
			{
				response = GTK_RESPONSE_CANCEL;
			}
		}
		else
		{
			response = GTK_RESPONSE_CANCEL;
		}
	}
	
	gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), response);

	g_free(user);
	g_free(text);

	return FALSE;
}

static void
_composer_dialog_apply_clicked(GtkWidget *button, GtkWidget *dialog)
{
	gtk_widget_set_sensitive(dialog, FALSE);
	g_idle_add((GSourceFunc)_composer_dialog_apply_worker, dialog);
}

static void
_composer_dialog_text_buffer_changed(GtkTextBuffer *buffer, GtkWidget *widget)
{
	_composer_dialog_update_remaining_characters(widget);
}

static void
_composer_dialog_destroy(GtkWidget *dialog, _ComposerWindowPrivate *private)
{
	g_free(private->prev_status);
	mainwindow_remove_pixbuf_group(private->parent, private->pixbuf_group);
}

/*
 *	pixbuf loader:
 */
static _ComposerWindowPixbufArg *
_composer_dialog_create_pixbuf_arg(GtkWidget *dialog, const gchar *username, GdkPixbuf *pixbuf)
{
	_ComposerWindowPixbufArg *arg;

	arg = (_ComposerWindowPixbufArg *)g_slice_alloc(sizeof(_ComposerWindowPixbufArg));
	arg->dialog = dialog;
	g_strlcpy(arg->username, username, 64);
	arg->pixbuf = pixbuf;

	return arg;
}

static void
_composer_dialog_destroy_pixbuf_arg(_ComposerWindowPixbufArg *arg)
{
	if(arg->pixbuf)
	{
		gdk_pixbuf_unref(arg->pixbuf);
	}

	g_slice_free1(sizeof(_ComposerWindowPixbufArg), arg);
}


static gboolean
_composer_dialog_set_image_worker(_ComposerWindowPixbufArg *arg)
{
	_ComposerWindowPrivate *private = (_ComposerWindowPrivate *)g_object_get_data(G_OBJECT(arg->dialog), "private");
	GtkTreeModel *model;
	GtkListStore *store;
	gboolean valid;
	gboolean found = FALSE;
	GtkTreeIter iter;
	gchar *username;

	gdk_threads_enter();

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(private->combo_users));
	store = GTK_LIST_STORE(model);
	valid = gtk_tree_model_get_iter_first(model, &iter);

	while(valid && !found)
	{
		gtk_tree_model_get(model, &iter, COMPOSER_DIALOG_TREEVIEW_COLUMN_USERNAME, &username, -1);

		if(!g_strcmp0(username, arg->username))
		{
			found = TRUE;
			gtk_list_store_set(store, &iter, COMPOSER_DIALOG_TREEVIEW_COLUMN_PIXBUF, arg->pixbuf, -1);
		}
		else
		{
			valid = gtk_tree_model_iter_next(model, &iter);
		}

		g_free(username);
	}

	_composer_dialog_destroy_pixbuf_arg(arg);

	gdk_threads_leave();

	return FALSE;
}

static void
_composer_dialog_set_image(GdkPixbuf *pixbuf, _ComposerWindowPixbufArg *arg)
{
	g_idle_add((GSourceFunc)_composer_dialog_set_image_worker, _composer_dialog_create_pixbuf_arg(arg->dialog, arg->username, gdk_pixbuf_scale_simple(pixbuf, 24, 24, GDK_INTERP_BILINEAR)));
}

static void
_composer_dialog_populate_users(GtkWidget *dialog, gchar **users, gint count, const gchar *selected_user)
{
	_ComposerWindowPrivate *private = (_ComposerWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreeIter iter;
	TwitterDbHandle *handle;
	TwitterUser user;
	gboolean selected = FALSE;
	GError *err = NULL;

	if((handle = twitterdb_get_handle(&err)))
	{
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(private->combo_users));
		store = GTK_LIST_STORE(model);

		for(gint i = 0; i < count; ++i)
		{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store,
					   &iter,
					   COMPOSER_DIALOG_TREEVIEW_COLUMN_USERNAME,
					   users[i],
					   -1);

			if((!selected_user && !i) || (selected_user && !g_ascii_strcasecmp(selected_user, users[i])))
			{
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(private->combo_users), &iter);
				selected = TRUE;
			}

			if(twitterdb_get_user_by_name(handle, users[i], &user, &err))
			{
				mainwindow_load_pixbuf(private->parent,
				                       private->pixbuf_group,
				                       user.image,
				                       (PixbufLoaderCallback)_composer_dialog_set_image,
				                       _composer_dialog_create_pixbuf_arg(dialog, users[i], NULL),
				                       (GFreeFunc)_composer_dialog_destroy_pixbuf_arg);
			}
			else if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
				err = NULL;
			}
		}

		if(!selected)
		{
			if(gtk_tree_model_get_iter_first(model, &iter))
			{
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(private->combo_users), &iter);
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

/*
 *	public:
 */
GtkWidget *
composer_dialog_create(GtkWidget *parent, gchar **users, gint users_count, const gchar *selected_user, const gchar *prev_status, const gchar *title)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *area;
	GtkWidget *align;
	gchar *filename;
	GtkWidget *image;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkWidget *scrolled;
	PangoAttrList *attrs;
	PangoAttribute *attr;
	GtkTextBuffer *buffer;
	static gint group_id = 0;
	_ComposerWindowPrivate *private = (_ComposerWindowPrivate *)g_malloc(sizeof(_ComposerWindowPrivate));

	g_assert(GTK_IS_WINDOW(parent));

	g_debug("Opening \"composer\" dialog");

	/* create unique group name */
	++group_id;

	/* create meta data */
	private->parent = parent;
	sprintf(private->pixbuf_group, "composer-%d", group_id);
	private->prev_status = prev_status ? g_strdup(prev_status) : NULL;

	dialog = gtk_deletable_dialog_new_with_buttons(title, GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* mainbox */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog))), hbox);

	/* image */
	align = gtk_alignment_new(0, 0.1, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, FALSE, 5);

	filename = pathbuilder_build_image_path("composer.png");
	image = gtk_image_new_from_file(filename);
	g_free(filename);

	gtk_container_add(GTK_CONTAINER(align), image);

	/* vertical box */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	/* combobox containing users */
	store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	private->combo_users = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	gtk_box_pack_start(GTK_BOX(vbox), private->combo_users, TRUE, TRUE, 5);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(private->combo_users), renderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(private->combo_users), renderer, "pixbuf", COMPOSER_DIALOG_TREEVIEW_COLUMN_PIXBUF);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(private->combo_users), renderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(private->combo_users), renderer, "text", COMPOSER_DIALOG_TREEVIEW_COLUMN_USERNAME);

	g_object_unref(store);

	/* create editor */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_widget_set_size_request(scrolled, 350, 80);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);
	
	private->view = gtk_text_view_new();
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(private->view), 2);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(private->view), 2);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(private->view), GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(scrolled), private->view);

	/* count */
	private->label_count = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), private->label_count, TRUE, TRUE, 5);

	attrs = pango_attr_list_new();

	attr = pango_attr_foreground_new(0, 0, 0xFFFF);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
	pango_attr_list_insert(attrs, attr);

	gtk_label_set_attributes(GTK_LABEL(private->label_count), attrs);
	pango_attr_list_unref(attrs);

	/* insert buttons */
	area = gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));

	private->button_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_start(GTK_BOX(area), private->button_ok, FALSE, FALSE, 0);
	g_signal_connect(private->button_ok, "clicked", (GCallback)_composer_dialog_apply_clicked, dialog);

	gtk_deletable_dialog_add_action_widget(GTK_DELETABLE_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_CANCEL), GTK_RESPONSE_CANCEL);

	/* populate user list */
	_composer_dialog_populate_users(dialog, users, users_count, selected_user);

	/* show widgets */
	gtk_widget_show_all(dialog);
	_composer_dialog_update_remaining_characters(dialog);

	if(users_count <= 1)
	{
		gtk_widget_set_visible(private->combo_users, FALSE);
	}

	/* signals */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(private->view));
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(_composer_dialog_destroy), private);
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(_composer_dialog_text_buffer_changed), dialog);

	/* set focus */
	gtk_widget_grab_focus(private->view);

	return dialog;
}

gchar *
composer_dialog_get_text(GtkWidget *dialog)
{
	_ComposerWindowPrivate *private;
	GtkTextBuffer *buffer;
	gchar *text;
	GtkTextIter start;
	GtkTextIter end;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_ComposerWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(private->view));
	gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
	gtk_text_buffer_get_iter_at_offset(buffer, &end, -1);
	text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);

	return text;
}

void
composer_dialog_set_text(GtkWidget *dialog, const gchar *text)
{
	_ComposerWindowPrivate *private;
	GtkTextBuffer *buffer;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_ComposerWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(private->view));
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), text, -1);
}

gchar *
composer_dialog_get_user(GtkWidget *dialog)
{
	_ComposerWindowPrivate *private;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *username = NULL;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_ComposerWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(private->combo_users));

	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(private->combo_users), &iter))
	{
		gtk_tree_model_get(model, &iter, COMPOSER_DIALOG_TREEVIEW_COLUMN_USERNAME, &username, -1);
	}

	return username;
}

void
composer_dialog_set_apply_callback(GtkWidget *dialog, ComposerApplyCallback callback, gpointer user_data)
{
	_ComposerWindowPrivate *private;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_ComposerWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	private->callback = callback;
	private->user_data = user_data;
}

/**
 * @}
 */


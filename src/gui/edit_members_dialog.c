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
 * \file edit_members_dialog.c
 * \brief A dialog for editing (list) members.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 19. January 2012
 */

#include "edit_members_dialog.h"
#include "mainwindow.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _EditMembersDialogPrivate;
 * \brief Private data of the "edit members" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! Pixbuf group.*/
	gchar pixbuf_group[64];
	/*! Default user image. */
	GdkPixbuf *pixbuf;
} _EditMembersDialogPrivate;

/**
 * \struct _EditMembersDialogPixbufArg
 * \brief Holds a username and a GdkPixbuf to set.
 */
typedef struct
{
	/*! A dialog. */
	GtkWidget *dialog;
	/*! A username. */
	gchar username[64];
	/*! A GdkPixbuf to set. */
	GdkPixbuf *pixbuf;
}
_EditMembersDialogPixbufArg;

/*
 *	image functions:
 */
static _EditMembersDialogPixbufArg *
_edit_members_dialog_create_pixbuf_arg(GtkWidget *dialog, const gchar *username, GdkPixbuf *pixbuf)
{
	_EditMembersDialogPixbufArg *arg = g_slice_alloc(sizeof(_EditMembersDialogPixbufArg));

	arg->dialog = dialog;
	g_strlcpy(arg->username, username, 64);
	arg->pixbuf = pixbuf;

	return arg;
}

static void
_edit_members_dialog_destroy_pixbuf_arg(_EditMembersDialogPixbufArg *arg)
{
	if(arg->pixbuf)
	{
		gdk_pixbuf_unref(arg->pixbuf);
	}

	g_slice_free1(sizeof(_EditMembersDialogPixbufArg), arg);
}

static gboolean 
_edit_members_dialog_set_image_worker(_EditMembersDialogPixbufArg *arg)
{
	gdk_threads_enter();
	gtk_user_list_dialog_set_user_pixbuf(GTK_USER_LIST_DIALOG(arg->dialog), arg->username, arg->pixbuf);
	gdk_threads_leave();

	_edit_members_dialog_destroy_pixbuf_arg(arg);

	return FALSE;
}

static void
_edit_members_dialog_set_image(GdkPixbuf *pixbuf, _EditMembersDialogPixbufArg *arg)
{
	g_assert(pixbuf != NULL);
	g_assert(GTK_IS_DELETABLE_DIALOG(arg->dialog));

	gdk_pixbuf_ref(pixbuf);
	g_idle_add((GSourceFunc)_edit_members_dialog_set_image_worker, _edit_members_dialog_create_pixbuf_arg(arg->dialog, arg->username, pixbuf));
}

/*
 *	destroy worker:
 */
static gboolean
_edit_members_dialog_destroy_worker(GtkWidget *dialog)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	gdk_threads_enter();
	gtk_widget_destroy(dialog);
	gdk_threads_leave();

	return FALSE;
}

/*
 *	events:
 */
static gboolean
_edit_members_dialog_delete(GtkDeletableDialog *dialog, GdkEvent event, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	if(gtk_widget_get_sensitive(GTK_WIDGET(dialog)))
	{
		gtk_deletable_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
	}

	return TRUE;
}

/*
 *	public:
 */
GtkWidget *
edit_members_dialog_create(GtkWidget *parent, const gchar *title)
{
	GtkWidget *dialog;
	_EditMembersDialogPrivate *private;
	static gint group_id = 0;
	gchar *filename;

	g_assert(GTK_IS_WINDOW(parent));

	private = g_slice_alloc(sizeof(_EditMembersDialogPrivate));
	private->parent = parent;

	/* load default user image */
	filename = pathbuilder_build_image_path("default-avatar.png");
	private->pixbuf = gdk_pixbuf_new_from_file_at_size(filename, 48, 48, NULL);
	g_free(filename);

	/* create unique group name */
	++group_id;
	sprintf(private->pixbuf_group, "userlist-%d", group_id);

	/* create dialog */
	g_debug("Creating \"edit membership\" dialog");
	dialog = gtk_user_list_dialog_new(GTK_WINDOW(parent), title, TRUE);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* events */
	g_signal_connect(dialog, "delete-event", (GCallback)_edit_members_dialog_delete, NULL);

	/* set default size */
	gtk_widget_set_size_request(dialog, 350, 500);

	return dialog;
}

void
edit_members_dialog_destroy(GtkWidget *dialog)
{
	_EditMembersDialogPrivate *private;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_EditMembersDialogPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	/* remove pixbuf group */
	mainwindow_remove_pixbuf_group(private->parent, private->pixbuf_group);

	/* free default user image */
	if(private->pixbuf)
	{
		gdk_pixbuf_unref(private->pixbuf);
	}

	/* destroy the widget in a worker (otherwise an image worker might access the destroyed dialog) */
	g_idle_add((GSourceFunc)_edit_members_dialog_destroy_worker, dialog);
}

void
edit_members_dialog_add_user(GtkWidget *dialog, TwitterUser user, gboolean checked)
{
	_EditMembersDialogPrivate *private;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_EditMembersDialogPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	/* append user to list */
	gtk_user_list_dialog_append_user(GTK_USER_LIST_DIALOG(dialog), user.screen_name, private->pixbuf, checked);

	/* load pixbuf */
	mainwindow_load_pixbuf(private->parent,
	                       private->pixbuf_group,
	                       user.image,
	                       (PixbufLoaderCallback)_edit_members_dialog_set_image,
	                       _edit_members_dialog_create_pixbuf_arg(dialog, user.screen_name, NULL),
	                       (GFreeFunc)_edit_members_dialog_destroy_pixbuf_arg);
}

/**
 * @}
 */



/***************************************************************************
    begin........: January 2012
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
 * \file select_account_dialog.c
 * \brief A dialog for selecting an account.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 17. August 2012
 */

#include <glib/gi18n.h>
#include "select_account_dialog.h"
#include "mainwindow.h"
#include "../pathbuilder.h"
#include "../twitterdb.h"

/**
 * \struct _SelectUserDialogPrivate;
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
} _SelectUserDialogPrivate;

/**
 * \struct _SelectUserDialogPixbufArg
 * \brief Holds a username and a GdkPixbuf to set.
 */
typedef struct
{
	/*! A dialog. */
	GtkWidget *dialog;
	/*! A username. */
	gchar username[64];
	/*! A pixbuf. */
	GdkPixbuf *pixbuf;
}
_SelectUserDialogPixbufArg;

/*
 *	events:
 */
static gboolean
_select_account_dialog_delete(GtkDeletableDialog *dialog, GdkEvent event, gpointer user_data)
{
	_SelectUserDialogPrivate *private = (_SelectUserDialogPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	if(gtk_widget_get_sensitive(GTK_WIDGET(dialog)))
	{
		g_object_unref(private->pixbuf);
		g_free(private);
		gtk_deletable_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
	}

	return TRUE;
}

/*
 *	pixbuf loader:
 */
static _SelectUserDialogPixbufArg *
_select_account_dialog_create_pixbuf_arg(GtkWidget *dialog, const gchar *username, GdkPixbuf *pixbuf)
{
	_SelectUserDialogPixbufArg *arg;

	arg = (_SelectUserDialogPixbufArg *)g_slice_alloc(sizeof(_SelectUserDialogPixbufArg));
	arg->dialog = dialog;
	g_strlcpy(arg->username, username, 64);
	arg->pixbuf = pixbuf;

	return arg;
}

static void
_select_account_dialog_destroy_pixbuf_arg(_SelectUserDialogPixbufArg *arg)
{
	if(arg->pixbuf)
	{
		g_object_unref(G_OBJECT(arg->pixbuf));
	}

	g_slice_free1(sizeof(_SelectUserDialogPixbufArg), arg);
}

static gboolean 
_select_account_dialog_set_image_worker(_SelectUserDialogPixbufArg *arg)
{
	gdk_threads_enter();
	gtk_user_list_dialog_set_user_pixbuf(GTK_USER_LIST_DIALOG(arg->dialog), arg->username, arg->pixbuf);
	gdk_threads_leave();

	_select_account_dialog_destroy_pixbuf_arg(arg);

	return FALSE;
}

static void
_select_account_dialog_set_image(GdkPixbuf *pixbuf, _SelectUserDialogPixbufArg *arg)
{
	g_assert(pixbuf != NULL);
	g_assert(GTK_IS_DELETABLE_DIALOG(arg->dialog));

	g_object_ref(G_OBJECT(pixbuf));
	g_idle_add((GSourceFunc)_select_account_dialog_set_image_worker, _select_account_dialog_create_pixbuf_arg(arg->dialog, arg->username, pixbuf));
}

/*
 *	populate dialog:
 */
static void
_select_account_dialog_populate(GtkWidget *dialog, gchar **accounts, gint length)
{
	TwitterDbHandle *handle;
	GError *err = NULL;
	TwitterUser user;
	_SelectUserDialogPrivate *private = (_SelectUserDialogPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	/* append accounts */
	if((handle = twitterdb_get_handle(&err)))
	{
		for(gint i = 0; i < length; ++i)
		{
			gtk_user_list_dialog_append_user(GTK_USER_LIST_DIALOG(dialog), accounts[i], private->pixbuf, FALSE);

			if(twitterdb_get_user_by_name(handle, accounts[i], &user, &err))
			{
				mainwindow_load_pixbuf(private->parent,
				                       private->pixbuf_group,
				                       user.image,
				                       (PixbufLoaderCallback)_select_account_dialog_set_image,
				                       _select_account_dialog_create_pixbuf_arg(dialog, accounts[i], NULL),
				                       (GFreeFunc)_select_account_dialog_destroy_pixbuf_arg);
			}
			else
			{
				g_warning("Couldn't get user from database: \"%s\"", accounts[i]);

				if(err)
				{
					g_warning("%s", err->message);
					g_error_free(err);
					err = NULL;
				}
			}
		}

		twitterdb_close_handle(handle);
	}
	else
	{
		g_warning("Couldn't create database handle.");
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
select_account_dialog_create(GtkWidget *parent, gchar **accounts, gint length, const gchar *title, const gchar *text)
{
	GtkWidget *dialog;
	_SelectUserDialogPrivate *private;
	static gint group_id = 0;
	gchar *filename;
	static guint id = 0;

	g_assert(GTK_IS_WINDOW(parent));

	private = g_slice_alloc(sizeof(_SelectUserDialogPrivate));
	private->parent = parent;

	/* load default user image */
	filename = pathbuilder_build_image_path("default-avatar.png");
	private->pixbuf = gdk_pixbuf_new_from_file_at_size(filename, 48, 48, NULL);
	g_free(filename);

	/* create unique group name */
	++group_id;
	sprintf(private->pixbuf_group, "accounts-%d", group_id);

	/* create dialog */
	g_debug("Creating \"select status\" dialog");
	dialog = gtk_user_list_dialog_new(GTK_WINDOW(parent), title, TRUE);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	g_object_set(G_OBJECT(dialog),
	             "checkbox-column-visible", FALSE,
	             "username-column-title", _("Accounts"),
	             "show-user-count", FALSE,
	             "message", _(text),
	             NULL);

	/* events */
	g_signal_connect(dialog, "delete-event", (GCallback)_select_account_dialog_delete, NULL);

	/* set default size */
	gtk_widget_set_size_request(dialog, -1, 300);

	/* generate unique group id */
	if(id == G_MAXUINT)
	{
		id = 0;
	}

	sprintf(private->pixbuf_group, "select-account-%d", id);

	/* populate dialog */
	_select_account_dialog_populate(dialog, accounts, length);

	return dialog;
}

gint select_account_dialog_run(GtkWidget *dialog)
{
	return gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));
}

gchar *
select_account_dialog_get_account(GtkWidget *dialog)
{
	return gtk_user_list_dialog_get_selected_user(GTK_USER_LIST_DIALOG(dialog));
}

GtkWidget *
select_account_dialog_add_button(GtkWidget *dialog, const gchar *button_text, gint response_id)
{
	return gtk_deletable_dialog_add_button(GTK_DELETABLE_DIALOG(dialog), button_text, response_id);
}

GtkWidget *
select_account_dialog_get_action_area(GtkWidget *dialog)
{
	return gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));
}

/**
 * @}
 */


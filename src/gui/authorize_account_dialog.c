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
 * \file authorize_account_dialog.c
 * \brief A dialog for account authorization.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2011
 */

#include <string.h>
#include <glib/gi18n.h>

#include "authorize_account_dialog.h"
#include "mainwindow.h"
#include "gtkdeletabledialog.h"
#include "gtk_helpers.h"
#include "../application.h"
#include "../helpers.h"
#include "../twitter.h"
#include "../oauth/twitter_oauth.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _AuthorizeAccountWindowPrivate
 * \brief Private data of the "authorize account" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! The mainwindow. */
	GtkWidget *mainwindow;
	/*! Entry to enter the provided PIN. */
	GtkWidget *entry_pin;
	/*! Button to request PIN. */
	GtkWidget *button_get_pin;
	/*! Button to authorize account. */
	GtkWidget *button_ok;
	/*! Indicates if a PIN has been generated. */
	gboolean pin_generated;
	/*! OAuth request key. */
	gchar *request_key;
	/*! OAuth request secret. */
	gchar *request_secret;
	/*! OAuth access key. */
	gchar *access_key;
	/*! OAuth access secret. */
	gchar *access_secret;
	/*! Indicates if the account has been authorized. */
	gboolean authorized;
} _AuthorizeAccountWindowPrivate;

/*
 *	helpers:
 */
static void
_authorize_account_dialog_entry_pin_changed(GtkWidget *widget, GtkWidget *dialog)
{
	_AuthorizeAccountWindowPrivate *private;
	const char *text;
	gint length;

	g_assert(GTK_IS_ENTRY(widget));
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_AuthorizeAccountWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	gtk_widget_set_sensitive(private->button_ok, FALSE);

	/* check if PIN has been generated already */
	if(!private->pin_generated)
	{
		return;
	}

	/* check PIN format */
	text = gtk_entry_get_text(GTK_ENTRY(private->entry_pin));

	if((length = strlen(text)) < 5)
	{
		return;
	}

	for(gint i = 0; i < length; ++i)
	{
		if(!g_ascii_isdigit(text[i]))
		{
			return;
		}
	}

	gtk_widget_set_sensitive(private->button_ok, TRUE);
}

/*
 *	workers:
 */
static gboolean
_authorize_account_dialog_get_pin_worker(GtkWidget *dialog)
{
	_AuthorizeAccountWindowPrivate *private;
	gchar *request_key = NULL;
	gchar *request_secret = NULL;
	Config *config;
	UrlOpener *urlopener;
	GtkWidget *failure_dialog;
	gulong delete_handler;
	GError *err = NULL;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	g_debug("Twitter pin worker started");
	private = (_AuthorizeAccountWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	/* free old request key & secret */
	if(private->request_key)
	{
		g_free(private->request_key);
		private->request_key = NULL;
	}

	if(private->request_secret)
	{
		g_free(private->request_secret);
		private->request_secret = NULL;
	}

	/* lock GUI */
	gdk_threads_enter();

	/* disable window & set busy cursor */
	gtk_helpers_set_widget_busy(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog)), TRUE);
	delete_handler = g_signal_connect(dialog, "delete-event", G_CALLBACK(gtk_true), NULL);

	gtk_main_iteration();

	/* create url opener */
	config = mainwindow_lock_config(private->mainwindow);
	urlopener = helpers_create_urlopener_from_config(config);
	mainwindow_unlock_config(private->mainwindow);

	gtk_main_iteration();

	/* get request key & secret */
	if(twitter_request_authorization(urlopener, &request_key, &request_secret, &err))
	{
		private->pin_generated = TRUE;
		private->request_key = request_key;
		private->request_secret = request_secret;

		/* enable PIN entry field */
		gtk_widget_set_sensitive(private->entry_pin, TRUE);
		gtk_widget_grab_focus(private->entry_pin);
	}
	else
	{
		/* display failure message */
		if(err)
		{
			failure_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", err->message);
			gtk_dialog_run(GTK_DIALOG(failure_dialog));
			gtk_widget_destroy(failure_dialog);
			g_error_free(err);
		}
	}

	if(urlopener)
	{
		url_opener_free(urlopener);
	}

	/* enable window & set default cursor */
	gtk_helpers_set_widget_busy(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog)), FALSE);
	g_signal_handler_disconnect(dialog, delete_handler);

	/* unlock GUI */
	gdk_threads_leave();

	g_debug("Twitter pin worker finished");

	return FALSE;
}

static gboolean
_authorize_account_dialog_authorization_worker(GtkWidget *dialog)
{
	_AuthorizeAccountWindowPrivate *private;
	gchar *access_key = NULL;
	gchar *access_secret = NULL;
	gulong delete_handler;

	g_debug("Twitter authorization worker started");

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private  = (_AuthorizeAccountWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	g_assert(private->request_key != NULL);
	g_assert(private->request_secret != NULL);

	/* free old access key & secret */
	if(private->access_key)
	{
		g_free(private->access_key);
		private->access_key = NULL;
	}

	if(private->access_secret)
	{
		g_free(private->access_secret);
		private->access_secret = NULL;
	}

	gdk_threads_enter();

	/* disable window & set busy cursor */
	gtk_helpers_set_widget_busy(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog)), TRUE);
	delete_handler = g_signal_connect(dialog, "delete-event", G_CALLBACK(gtk_true), NULL);

	gtk_main_iteration();

	if((private->authorized = twitter_oauth_access_token(private->request_key,
	                                                     private->request_secret,
	                                                     gtk_entry_get_text(GTK_ENTRY(private->entry_pin)),
	                                                     &access_key,
	                                                     &access_secret)))
	{
		private->authorized = TRUE;
		private->access_key = access_key;
		private->access_secret = access_secret;
	}
	else
	{
		if(access_key)
		{
			g_free(access_key);
		}

		if(access_secret)
		{
			g_free(access_secret);
		}
	}

	/* enable window */
	gtk_helpers_set_widget_busy(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog)), FALSE);
	g_signal_handler_disconnect(dialog, delete_handler);

	gdk_threads_leave();

	/* set dialog response */
	if(private->authorized)
	{
		gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), GTK_RESPONSE_OK);
	}

	g_debug("Twitter authorization worker finished");

	return FALSE;
}

/*
 *	events:
 */
static void
_authorize_account_dialog_destroy(GtkDialog *dialog, gpointer user_data)
{
	_AuthorizeAccountWindowPrivate *private;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	private = (_AuthorizeAccountWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	if(private->request_key)
	{
		g_free(private->request_key);
	}

	if(private->request_secret)
	{
		g_free(private->request_secret);
	}

	g_free(private);
}

static void
_authorize_account_dialog_button_get_pin_clicked(GtkDialog *widget, GtkWidget *dialog)
{
	g_assert(GTK_IS_BUTTON(widget));
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	g_debug("Starting \"get pin\" worker...");
	g_idle_add((GSourceFunc)_authorize_account_dialog_get_pin_worker, dialog);
}

static void
_authorize_account_dialog_button_ok_clicked(GtkWidget *widget, GtkWidget *dialog)
{
	g_assert(GTK_IS_BUTTON(widget));
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	g_debug("Starting \"authorization\" worker...");
	g_idle_add((GSourceFunc)_authorize_account_dialog_authorization_worker, dialog);
}

/*
 *	public:
 */
GtkWidget *
authorize_account_dialog_create(GtkWidget *parent, GtkWidget *mainwindow, const gchar *username)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *image;
	GtkWidget *alignment;
	GtkWidget *label;
	GtkWidget *area;
	gchar *text;
	gchar *escpaped;
	_AuthorizeAccountWindowPrivate *private = (_AuthorizeAccountWindowPrivate *)g_malloc(sizeof(_AuthorizeAccountWindowPrivate));

	g_assert(GTK_IS_WINDOW(parent));
	g_assert(GTK_IS_WINDOW(mainwindow));
	g_assert(username != NULL);

	g_debug("Opening accounts dialog");

	private->parent = parent;
	private->mainwindow = mainwindow;
	private->pin_generated = FALSE;
	private->request_key = NULL;
	private->request_secret = NULL;
	private->access_key = NULL;
	private->access_secret = NULL;
	private->authorized = FALSE;

	dialog = gtk_deletable_dialog_new_with_buttons(_("Authorize request"), GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* main boxes */
	frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(gtk_deletable_dialog_get_content_area(GTK_DELETABLE_DIALOG(dialog))), frame);

	hbox = gtk_hbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* insert image */
	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

	/* insert info label & entry */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);

	escpaped = g_markup_escape_text(username, -1);
	text = g_strdup_printf(_("Please enter the PIN provided by Twitter to authorize %s for <b>%s</b>."), APPLICATION_NAME, escpaped);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), text);
	gtk_container_add(GTK_CONTAINER(alignment), label);

	if(escpaped)
	{
		g_free(escpaped);
	}

	g_free(text);

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

	private->entry_pin = gtk_entry_new();
	gtk_widget_set_sensitive(private->entry_pin, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), private->entry_pin, TRUE, TRUE, 0);

	private->button_get_pin = gtk_button_new_with_label(_("Get PIN"));
	gtk_box_pack_start(GTK_BOX(hbox), private->button_get_pin, FALSE, TRUE, 0);

	/* insert action buttons */
	area = gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));

	private->button_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_set_sensitive(private->button_ok, FALSE);
	gtk_box_pack_start(GTK_BOX(area), private->button_ok, FALSE, FALSE, 0);

	gtk_deletable_dialog_add_action_widget(GTK_DELETABLE_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_CANCEL), GTK_RESPONSE_CANCEL);

	/* set size & show widgets */
	gtk_widget_show_all(dialog);

	/* signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_authorize_account_dialog_destroy), NULL);
	g_signal_connect(private->entry_pin, "changed", G_CALLBACK(_authorize_account_dialog_entry_pin_changed), dialog);
	g_signal_connect(private->button_get_pin, "clicked", G_CALLBACK(_authorize_account_dialog_button_get_pin_clicked), dialog);
	g_signal_connect(private->button_ok, "clicked", G_CALLBACK(_authorize_account_dialog_button_ok_clicked), dialog);

	return dialog;
}

gboolean
authorize_account_dialog_get_token_and_secret(GtkWidget *dialog, gchar **access_key, gchar **access_secret)
{
	_AuthorizeAccountWindowPrivate *private;
	gboolean result = FALSE;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));
	g_assert(access_key != NULL);
	g_assert(access_secret != NULL);

	private = (_AuthorizeAccountWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	if(private->authorized)
	{
		*access_key = g_strdup(private->access_key);
		*access_secret = g_strdup(private->access_secret);
		result = TRUE;
	}

	return result;
}

/**
 * @}
 */


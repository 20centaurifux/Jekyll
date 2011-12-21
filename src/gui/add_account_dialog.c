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
 * \file add_account_dialog.c
 * \brief A dialog for adding an account.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 30. September 2011
 */

#include <string.h>
#include <glib/gi18n.h>

#include "add_account_dialog.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _AddAccountWindowPrivate
 * \brief Private data of the "add account" dialog.
 */
typedef struct
{
	/*! An entry holding an username. */
	GtkWidget *entry_username;
	/*! Label used to display failure messages. */
	GtkWidget *label_failure;
	/*! List containing existing user accounts. */
	GList *existing_accounts;
} _AddAccountWindowPrivate;

/*
 *	helpers:
 */
static gboolean
_add_account_dialog_account_exists(GList *existing_accounts, const gchar *username)
{
	GList *account = existing_accounts;

	g_assert(username != NULL);

	while(account)
	{
		if(!g_ascii_strcasecmp(account->data, username))
		{
			return TRUE;
		}

		account = g_list_next(account);
	}

	return FALSE;
}

/*
 *	events:
 */
static void
_add_account_dialog_destroy(GtkDialog *dialog, gpointer user_data)
{
	g_assert(GTK_IS_DIALOG(dialog));

	g_free(g_object_get_data(G_OBJECT(dialog), "private"));
}

static void
_add_account_dialog_button_add_clicked(GtkWidget *widget, GtkWidget *dialog)
{
	_AddAccountWindowPrivate *private;
	gchar *username;
	const gchar *message = NULL;

	g_assert(GTK_IS_BUTTON(widget));
	g_assert(GTK_IS_DIALOG(dialog));

	private = (_AddAccountWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	username = add_account_dialog_get_username(dialog);

	/* check if string is empty */
	if(!strlen(username))
	{
		message = _("Please enter an username.");
	}
	else
	{
		/* check if account does already exist */
		if(_add_account_dialog_account_exists(private->existing_accounts, username))
		{
			message = _("The specified account does already exist. Please enter a different username.");
		}
	}

	/* display failure message if message text has been set */
	if(message)
	{
		gtk_widget_set_visible(private->label_failure, TRUE);
		gtk_label_set_text(GTK_LABEL(private->label_failure), message);
		gtk_widget_grab_focus(private->entry_username);
	}
	else
	{
		/* close dialog */
		gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	}
}

/*
 *	public:
 */
GtkWidget *
add_account_dialog_create(GtkWidget *parent, GList *existing_accounts)
{
	GtkWidget *dialog;
	GtkWidget *area;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *image;
	gchar *filename;
	GtkWidget *label;
	GtkWidget *alignment;
	PangoAttrList *attrs;
	PangoAttribute *attr;
	_AddAccountWindowPrivate *private;

	g_assert(GTK_IS_WINDOW(parent));

	/* create dialog */
	dialog = gtk_dialog_new_with_buttons(_("Add Account"), GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	/* create window data */
	private = (_AddAccountWindowPrivate *)g_malloc(sizeof(_AddAccountWindowPrivate));
	private->existing_accounts = existing_accounts;
	g_object_set_data(G_OBJECT(dialog), "private", private);

	/* main boxes */
	frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), frame);

	hbox = gtk_hbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* insert image */
	filename = pathbuilder_build_image_path("account.png");
	image = gtk_image_new_from_file(filename);
	g_free(filename);
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

	/* insert info label & entry */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	label = gtk_label_new(_("Please enter the name of the Twitter account you want to add."));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	private->entry_username = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), private->entry_username, FALSE, FALSE, 0);

	/* insert label to display failure message */
	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, FALSE, 0);

	private->label_failure = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(alignment), private->label_failure);

	attrs = pango_attr_list_new();
	attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_foreground_new(50000, 10000, 0);
	pango_attr_list_insert(attrs, attr);

	gtk_label_set_line_wrap(GTK_LABEL(private->label_failure), TRUE);
	gtk_label_set_attributes(GTK_LABEL(private->label_failure), attrs);
	pango_attr_list_unref(attrs);

	/* insert buttons */
	area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));

	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_start(GTK_BOX(area),button, FALSE, FALSE, 0);

	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_CANCEL), GTK_RESPONSE_CANCEL);

	/* show widgets */
	gtk_widget_show_all(dialog);
	gtk_widget_set_visible(private->label_failure, FALSE);

	/* signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_add_account_dialog_destroy), NULL);
	g_signal_connect(button, "clicked", G_CALLBACK(_add_account_dialog_button_add_clicked), dialog);

	return dialog;
}

gchar *
add_account_dialog_get_username(GtkWidget *dialog)
{
	_AddAccountWindowPrivate *private;
	gchar *username = NULL;

	g_assert(GTK_IS_WIDGET(dialog));

	if((private = (_AddAccountWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private")))
	{
		username = g_strdup(gtk_entry_get_text(GTK_ENTRY(private->entry_username)));
		username = g_strstrip(username);
	}

	return username;
}

/**
 * @}
 */


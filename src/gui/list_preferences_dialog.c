/***************************************************************************
    begin........: April 2011
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
 * \file list_preferences_dialog.c
 * \brief Edit general list properties.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 27. February 2012
 */

#include <string.h>
#include <glib/gi18n.h>

#include "list_preferences_dialog.h"
#include "mainwindow.h"
#include "gtk_helpers.h"
#include "../pathbuilder.h"
#include "../twitterdb.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _ListPreferencesWindowData
 * \brief Private data of the "list preferences" dialog.
 */
typedef struct
{
	/*! The parent window. */
	GtkWidget *parent;
	/*! "Apply" button. */
	GtkWidget *button_apply;
	/*! List owner. */
	gchar owner[64];
	/*! Initial list name. */
	gchar previous_listname[64];
	/*! Entry holding name of the list. */
	GtkWidget *entry_name;
	/*! Entry holding description of the list. */
	GtkWidget *entry_description;
	/*! Indicates if the list is protected. */
	GtkWidget *checkbox_protected;
	/*! Label used to display failure messages. */
	GtkWidget *label_failure;
} _ListPreferencesWindowData;

/*
 *	helpers:
 */
static void
_list_preferences_dialog_show_failure(GtkWidget *dialog, const gchar *message)
{
	_ListPreferencesWindowData *windata;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	windata = (_ListPreferencesWindowData *)g_object_get_data(G_OBJECT(dialog), "windata");
	gtk_widget_set_visible(windata->label_failure, TRUE);
	gtk_label_set_text(GTK_LABEL(windata->label_failure), message);
}

static void
_list_preferences_dialog_hide_failure(GtkWidget *dialog)
{
	_ListPreferencesWindowData *windata;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	windata = (_ListPreferencesWindowData *)g_object_get_data(G_OBJECT(dialog), "windata");
	gtk_widget_set_visible(windata->label_failure, FALSE);
}

static gboolean
_list_preferences_dialog_validate_username(GtkWidget *dialog)
{
	_ListPreferencesWindowData *windata;
	gchar *name;
	TwitterDbHandle *handle;
	GError *err = NULL;
	gboolean result = TRUE;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	windata = (_ListPreferencesWindowData *)g_object_get_data(G_OBJECT(dialog), "windata");

	/* get entered name */
	name = g_strdup(gtk_entry_get_text(GTK_ENTRY(windata->entry_name)));
	name = g_strstrip(name);

	/* check if name has been modified */
	if(g_strcmp0(windata->previous_listname, name))
	{
		result = FALSE;

		/* check if name is empty */
		if(strlen(name))
		{
			/* check if list does already exist */
			if((handle = twitterdb_get_handle(&err)))
			{
				if(twitterdb_list_exists(handle, windata->owner, name, &err))
				{
					_list_preferences_dialog_show_failure(GTK_WIDGET(dialog), _("A list with the given name does already exist."));
				}
				else if(err)
				{
					_list_preferences_dialog_show_failure(GTK_WIDGET(dialog), err->message);
					g_error_free(err);
				}
				else
				{
					result = TRUE;
				}

				twitterdb_close_handle(handle);
			}
			else
			{
				if(err)
				{
					_list_preferences_dialog_show_failure(GTK_WIDGET(dialog), err->message);
					g_error_free(err);
				}
				else
				{
					_list_preferences_dialog_show_failure(GTK_WIDGET(dialog), _("Couldn't update list, an internal failure occured."));
				}
			}
		}
		else
		{
			_list_preferences_dialog_show_failure(GTK_WIDGET(dialog), _("Please enter a name for the list."));
		}
	}

	if(!result)
	{
		gtk_widget_grab_focus(windata->entry_name);
	}

	g_free(name);

	return result;
}

/*
 *	populate data:
 */
static void
_list_preferences_dialog_populate(GtkWidget *dialog)
{
	TwitterDbHandle *handle;
	_ListPreferencesWindowData *windata;
	TwitterList list;
	GError *err = NULL;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	windata = (_ListPreferencesWindowData *)g_object_get_data(G_OBJECT(dialog), "windata");

	if((handle = twitterdb_get_handle(&err)))
	{
		if(twitterdb_get_list(handle, windata->owner, windata->previous_listname, &list, &err))
		{
			gtk_entry_set_text(GTK_ENTRY(windata->entry_description), list.description);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(windata->checkbox_protected), list.protected);
		}

		twitterdb_close_handle(handle);
	}
}

/*
 *	dialog events:
 */
static void
_list_preferences_dialog_destroy(GtkWidget *dialog, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	g_free((_ListPreferencesWindowData *)g_object_get_data(G_OBJECT(dialog), "windata"));
}

static gboolean
_list_preferences_dialog_delete(GtkDeletableDialog *dialog, GdkEvent event, gpointer user_data)
{
	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	if(gtk_widget_get_sensitive(GTK_WIDGET(dialog)))
	{
		gtk_deletable_dialog_response(dialog, GTK_RESPONSE_DELETE_EVENT);
	}

	return TRUE;
}

static gboolean
_list_preferences_enable_window_worker(GtkWidget *dialog)
{
	gdk_threads_enter();
	gtk_helpers_set_widget_busy(dialog, FALSE);
	gdk_threads_leave();

	return FALSE;
}

static gpointer
_list_preferences_dialog_apply_worker(GtkDeletableDialog *dialog)
{
	_ListPreferencesWindowData *windata;
	TwitterClient *client;
	gchar *listname = NULL;
	gboolean valid;
	GError *err = NULL;
	GtkWidget *message_dialog;
	gboolean success = FALSE;
	gboolean response = TRUE;
	GtkWidget *parent;

	windata = (_ListPreferencesWindowData *)g_object_get_data(G_OBJECT(dialog), "windata");
	parent = windata->parent;

	/* validate listname */
	gdk_threads_enter();
	valid = _list_preferences_dialog_validate_username(GTK_WIDGET(dialog));
	gdk_threads_leave();

	if(valid)
	{
		if(mainwindow_get_current_sync_status(parent) != MAINWINDOW_SYNC_STATUS_SYNC_LISTS)
		{
			/* update list */
			if((client = mainwindow_create_twittter_client(windata->parent, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
			{
				listname = twitter_client_update_list(client,
								      windata->owner,
								      windata->previous_listname,
								      gtk_entry_get_text(GTK_ENTRY(windata->entry_name)),
								      gtk_entry_get_text(GTK_ENTRY(windata->entry_description)),
								      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(windata->checkbox_protected)),
								      &err);

				/* update GUI */
				if(listname)
				{
					success = TRUE;

					gdk_threads_enter();
					mainwindow_notify_list_updated(windata->parent,
							               windata->owner,
							               windata->previous_listname,
							               listname,
							               gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(windata->checkbox_protected)));
					gdk_threads_leave();
				}

				/* cleanup */
				g_object_unref(client);
			}
		}
	}
	else
	{
		response = FALSE;
	}	

	/* show failure message */
	if(response)
	{
		/* close dialog (on success) */
		gdk_threads_enter();
		gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(dialog), GTK_RESPONSE_APPLY);
		gdk_threads_leave();

		if(!success)
		{
			/* show failure message */
			message_dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
								GTK_DIALOG_MODAL,
								GTK_MESSAGE_WARNING,
								GTK_BUTTONS_OK,
								_("Couldn't update list, please try again later."));
			g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, message_dialog);
		}
	}
	else
	{
		/* enable window */
		g_idle_add((GSourceFunc)_list_preferences_enable_window_worker, dialog);
	}

	/* cleanup */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_free(listname);

	return NULL;
}

static void
_list_preferences_dialog_apply_clicked(GtkDeletableDialog *dialog, gpointer user_data)
{
	GError *err = NULL;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	/* show busy cursor & hide last failure message */
	gtk_helpers_set_widget_busy(GTK_WIDGET(dialog), TRUE);
	_list_preferences_dialog_hide_failure(GTK_WIDGET(dialog));

	/* start worker thread */
	if(!g_thread_create((GThreadFunc)_list_preferences_dialog_apply_worker, dialog, FALSE, &err))
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
 *	public:
 */
GtkWidget *
list_preferences_dialog_create(GtkWidget *parent, const gchar *title, const gchar *owner, const gchar *listname)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *area;
	GtkWidget *align;
	GtkWidget *table;
	GtkWidget *label;
	gchar *filename;
	GtkWidget *image;
	PangoAttrList *attrs;
	PangoAttribute *attr;
	_ListPreferencesWindowData *windata = (_ListPreferencesWindowData *)g_malloc(sizeof(_ListPreferencesWindowData));

	g_assert(GTK_IS_WINDOW(parent));
	g_assert(owner != NULL);
	g_assert(listname != NULL);

	g_debug("Opening \"list preferences\" dialog");

	windata->parent = parent;
	g_strlcpy(windata->owner, owner, 64);
	g_strlcpy(windata->previous_listname, listname, 64);

	dialog = gtk_deletable_dialog_new_with_buttons(title, GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "windata", windata);

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

	/* create table */
	frame = gtk_frame_new(_("List preferences"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	table = gtk_table_new(3, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 5);

	/* list name */
	align = gtk_alignment_new(0, 0.5, 0, 0);
	label = gtk_label_new(_("Name"));
	gtk_container_add(GTK_CONTAINER(align), label);

	windata->entry_name = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(windata->entry_name), 32);
	gtk_entry_set_text(GTK_ENTRY(windata->entry_name), listname);

	gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), windata->entry_name, 1, 2, 0, 1);

	/* list description */
	align = gtk_alignment_new(0, 0.5, 0, 0);
	label = gtk_label_new(_("Description"));
	gtk_container_add(GTK_CONTAINER(align), label);

	windata->entry_description = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(windata->entry_description), 140);

	gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), windata->entry_description, 1, 2, 1, 2);

	/* protected */
	align = gtk_alignment_new(0, 0, 0, 0);
	label = gtk_label_new(_("Private"));
	gtk_container_add(GTK_CONTAINER(align), label);

	windata->checkbox_protected = gtk_check_button_new();

	gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), windata->checkbox_protected, 1, 2, 2, 3);

	/* insert label to display failure message */
	windata->label_failure = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), windata->label_failure, TRUE, FALSE, 2);

	attrs = pango_attr_list_new();
	attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_foreground_new(50000, 10000, 0);
	pango_attr_list_insert(attrs, attr);

	gtk_label_set_line_wrap(GTK_LABEL(windata->label_failure), TRUE);
	gtk_label_set_attributes(GTK_LABEL(windata->label_failure), attrs);
	pango_attr_list_unref(attrs);

	/* insert buttons */
	area = gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog));

	windata->button_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_box_pack_start(GTK_BOX(area), windata->button_apply, FALSE, FALSE, 0);

	gtk_deletable_dialog_add_action_widget(GTK_DELETABLE_DIALOG(dialog), gtk_button_new_from_stock(GTK_STOCK_CANCEL), GTK_RESPONSE_CANCEL);

	/* populate data */
	_list_preferences_dialog_populate(dialog);

	/* set size & show widgets */
	gtk_widget_show_all(dialog);
	gtk_widget_set_visible(windata->label_failure, FALSE);

	/* signals */
	g_signal_connect(dialog, "destroy", G_CALLBACK(_list_preferences_dialog_destroy), NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(_list_preferences_dialog_delete), NULL);
	g_signal_connect_swapped(windata->button_apply, "clicked", G_CALLBACK(_list_preferences_dialog_apply_clicked), dialog);

	return dialog;
}

const gchar *
preferences_dialog_get_listname(GtkWidget *dialog)
{
	_ListPreferencesWindowData *windata;

	g_assert(GTK_IS_DELETABLE_DIALOG(dialog));

	windata = (_ListPreferencesWindowData *)g_malloc(sizeof(_ListPreferencesWindowData));

	return gtk_entry_get_text(GTK_ENTRY(windata->entry_name));
}

/**
 * @}
 */


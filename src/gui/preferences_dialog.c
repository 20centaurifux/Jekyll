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
 * \file preferences_dialog.c
 * \brief Preferences dialog.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 31. December 2011
 */

#include <string.h>
#include <glib/gi18n.h>

#include "preferences_dialog.h"
#include "mainwindow.h"
#include "../configuration.h"
#include "../urlopener.h"
#include "../pathbuilder.h"

#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _PreferenceWindowPrivate
 * \brief Private data of the "preferences" dialog.
 */
typedef struct
{
	/*! Category list. */
	GtkWidget *tree_categories;
	/*! Array containg frames. */
	GtkWidget *frames[3];
	/*! Checkbox to enable systray icon. */
	GtkWidget *checkbox_systray;
	/*! Checkbox to start program minimized. */
	GtkWidget *checkbox_start_minimized;
	/*! Checkbox to restore window status on startup. */
	GtkWidget *checkbox_restore_window_status;
	/*! Radio button to select the default browser. */
	GtkWidget *toggle_default_browser;
	/*! Radio button to select a custom browser. */
	GtkWidget *toggle_custom_browser;
	/*! Button to open an executable file. */
	GtkWidget *button_open_browser;
	/*! Textbox holding the filename of a selected executable. */
	GtkWidget *entry_browser;
	/*! Button showing status background color. */
	GtkWidget *button_color;
	/*! Checkbox to show the notification area. */
	GtkWidget *checkbox_notification_area;
	/*! Option to set "debug" notification level. */
	GtkWidget *toggle_notification_level_debug;
	/*! Option to set "info" notification level. */
	GtkWidget *toggle_notification_level_info;
	/*! Option to set "warning" notification level. */
	GtkWidget *toggle_notification_level_warning;
} _PreferenceWindowPrivate;

enum
{
	PREFERENCES_TREEVIEW_COLUMN_TEXT,
	PREFERENCES_TREEVIEW_COLUMN_ID
};

/**
 * \enum PREFERENCE_CATEGORY_ID
 * \brief A collection of categories.
 */
typedef enum
{
	PREFERENCES_CATEGORY_ID_GENERAL,
	PREFERENCES_CATEGORY_ID_BROWSER,
	PREFERENCES_CATEGORY_ID_VIEW
} PREFERENCE_CATEGORY_ID;

/*
 *	events:
 */
static void
_preferences_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	Config *config;
	Section *root;
	Section *section;
	Value *value;
	const gchar *text;
	GdkColor color;
	gchar *spec;

	if(response_id == GTK_RESPONSE_APPLY)
	{
		/* lock configuration & receive root section */
		config = mainwindow_lock_config((GtkWidget *)user_data);
		root = config_get_root(config);

		/* save general preferences */
		if(!(section = section_find_first_child(root, "Window")))
		{
			section = section_append_child(root, "Window");
		}

		if(!(value = section_find_first_value(section, "enable-systray")))
		{
			value = section_append_value(section, "enable-systray", VALUE_DATATYPE_BOOLEAN);
		}

		value_set_bool(value, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->checkbox_systray)));

		if(!(value = section_find_first_value(section, "start-minimized")))
		{
			value = section_append_value(section, "start-minimized", VALUE_DATATYPE_BOOLEAN);
		}

		value_set_bool(value, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->checkbox_start_minimized)));

		if(!(value = section_find_first_value(section, "restore-window")))
		{
			value = section_append_value(section, "restore-window", VALUE_DATATYPE_BOOLEAN);
		}

		value_set_bool(value, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->checkbox_restore_window_status)));

		/* save browser preferences */
		if(!(section = section_find_first_child(root, "Browser")))
		{
			section = section_append_child(root, "Browser");
		}

		if(!(value = section_find_first_value(section, "custom")))
		{
			value = section_append_value(section, "custom", VALUE_DATATYPE_BOOLEAN);
		}

		value_set_bool(value, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->toggle_custom_browser)));

		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->toggle_custom_browser)))
		{
			if((text = gtk_entry_get_text(GTK_ENTRY(private->entry_browser))) && strlen(text))
			{
				if(!(value = section_find_first_value(section, "executable")))
				{
					value = section_append_value(section, "executable", VALUE_DATATYPE_STRING);
				}

				value_set_string(value, text);
			}
		}

		/* save view preferences */
		if(!(section = section_find_first_child(root, "View")))
		{
			section = section_append_child(root, "View");
		}

		if(!(value = section_find_first_value(section, "show-notification-area")))
		{
			value = section_append_value(section, "show-notification-area", VALUE_DATATYPE_BOOLEAN);
		}

		value_set_bool(value, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->checkbox_notification_area)));

		if(!(value = section_find_first_value(section, "notification-area-level")))
		{
			value = section_append_value(section, "notification-area-level", VALUE_DATATYPE_INT32);
		}

		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->toggle_notification_level_warning)))
		{
			value_set_int32(value, 0);
		}
		else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->toggle_notification_level_info)))
		{
			value_set_int32(value, 1);
		}
		else
		{
			value_set_int32(value, 2);
		}

		if(!(value = section_find_first_value(section, "tweet-background-color")))
		{
			value = section_append_value(section, "tweet-background-color", VALUE_DATATYPE_STRING);
		}

		gtk_color_button_get_color(GTK_COLOR_BUTTON(private->button_color), &color);
		spec = gdk_color_to_string(&color);
		value_set_string(value, spec);
		g_free(spec);

		/* unlock configuration */
		mainwindow_unlock_config((GtkWidget *)user_data);
	}
}

static void
_preferences_dialog_destroy(GtkDialog *dialog, gpointer user_data)
{
	g_free(g_object_get_data(G_OBJECT(dialog), "private"));
}

/*
 *	populate data:
 */
static void
_preferences_dialog_populate_general_settings(_PreferenceWindowPrivate *private, Config *config)
{
	Section *root;
	Section *section;
	Value *value;

	g_debug("Populating general preferences");

	root = config_get_root(config);

	if((section = section_find_first_child(root, "Window")))
	{
		if((value = section_find_first_value(section, "enable-systray")) && VALUE_IS_BOOLEAN(value))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->checkbox_systray), value_get_bool(value));
		}

		if((value = section_find_first_value(section, "start-minimized")) && VALUE_IS_BOOLEAN(value))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->checkbox_start_minimized), value_get_bool(value));
		}

		if((value = section_find_first_value(section, "restore-window")) && VALUE_IS_BOOLEAN(value))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->checkbox_restore_window_status), value_get_bool(value));
		}
	}
}

static void
_preferences_dialog_populate_browser_settings(_PreferenceWindowPrivate *private, Config *config)
{
	Section *root;
	Section *section;
	Value *value;

	g_debug("Populating browser preferences");

	root = config_get_root(config);

	if((section = section_find_first_child(root, "Browser")))
	{
		if((value = section_find_first_value(section, "custom")) && VALUE_IS_BOOLEAN(value))
		{
			if(value_get_bool(value))
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->toggle_custom_browser), TRUE);
			}
			else
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->toggle_default_browser), TRUE);
			}
		}

		if((value = section_find_first_value(section, "executable")) && VALUE_IS_STRING(value))
		{
			gtk_entry_set_text(GTK_ENTRY(private->entry_browser), value_get_string(value));
		}
	}
}

static void
_preferences_dialog_populate_view_settings(_PreferenceWindowPrivate *private, Config *config)
{
	Section *root;
	Section *section;
	Value *value;
	gint level;
	GdkColor color;

	g_debug("Populating browser preferences");

	root = config_get_root(config);

	if((section = section_find_first_child(root, "View")))
	{
		if((value = section_find_first_value(section, "show-notification-area")) && VALUE_IS_BOOLEAN(value))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->checkbox_notification_area), value_get_bool(value));
		}

		if((value = section_find_first_value(section, "notification-area-level")) && VALUE_IS_INT32(value))
		{
			if((level = value_get_int32(value)) == 0)
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->toggle_notification_level_warning), TRUE);
			}
			else if(level == 2)
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->toggle_notification_level_debug), TRUE);
			}
			else
			{
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->toggle_notification_level_info), TRUE);
			}
		}

		if((value = section_find_first_value(section, "tweet-background-color")) && VALUE_IS_STRING(value))
		{
			if(gdk_color_parse(value_get_string(value), &color))
			{
				gtk_color_button_set_color(GTK_COLOR_BUTTON(private->button_color), &color);
			}
			else
			{
				g_warning("Couldn't parse tweet-background-color");
			}
		}
	}
}

static void
_preferences_dialog_populate(GtkWidget *dialog, GtkWidget *parent)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	Config *config;

	/* lock configuration */
	config = mainwindow_lock_config(parent);

	/* populate preferences */
	_preferences_dialog_populate_general_settings(private, config);
	_preferences_dialog_populate_browser_settings(private, config);
	_preferences_dialog_populate_view_settings(private, config);

	/* unlock configuration */
	mainwindow_unlock_config(parent);
}

/*
 *	show content:
 */
static void
_preferences_dialog_show_frame(GtkWidget *dialog, PREFERENCE_CATEGORY_ID id)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");

	for(gint i = 0; i < sizeof(private->frames) / sizeof(private->frames[0]); ++i)
	{
		gtk_widget_set_visible(private->frames[i], (id == i) ? TRUE : FALSE);
	}
}

/*
 *	general:
 */
static GtkWidget *
_preferences_dialog_create_general_frame(GtkWidget *dialog)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *subframe;
	GtkWidget *framehbox;
	GtkWidget *framevbox;

	/* create frame */
	frame = gtk_frame_new(_("General"));

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(hbox), vbox);

	subframe = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(vbox), subframe, FALSE, FALSE, 0);

	framehbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(subframe), framehbox);

	framevbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(framehbox), framevbox, FALSE, FALSE, 5);

	private->checkbox_systray = gtk_check_button_new_with_label(_("show program in systray"));
	gtk_box_pack_start(GTK_BOX(framevbox), private->checkbox_systray, FALSE, FALSE, 2);

	private->checkbox_start_minimized = gtk_check_button_new_with_label(_("open window minimized/hidden"));
	gtk_box_pack_start(GTK_BOX(framevbox), private->checkbox_start_minimized, FALSE, FALSE, 2);

	private->checkbox_restore_window_status = gtk_check_button_new_with_label(_("restore window size and position"));
	gtk_box_pack_start(GTK_BOX(framevbox), private->checkbox_restore_window_status, FALSE, FALSE, 2);

	return frame;
}

/*
 *	browser:
 */
static void
_preferences_dialog_update_browser_widgets(GtkWidget *dialog)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate*)g_object_get_data(G_OBJECT(dialog), "private");
	gboolean active;

	active = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->toggle_default_browser))) ? FALSE : TRUE;
	gtk_widget_set_sensitive(private->entry_browser, active);
	gtk_widget_set_sensitive(private->button_open_browser, active);
}

static void
_preferences_dialog_browser_button_toggled(GtkWidget *widget, GtkWidget *dialog)
{
	_preferences_dialog_update_browser_widgets(dialog);
}

static gboolean
_preferences_dialog_browser_file_filter(const GtkFileFilterInfo *filter_info, gpointer data)
{
	return g_file_test(filter_info->filename, G_FILE_TEST_IS_EXECUTABLE);
}

static void
_preferences_dialog_open_browser_clicked(GtkWidget *widget, GtkWidget *dialog)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate*)g_object_get_data(G_OBJECT(dialog), "private");
	GtkWidget *chooser;
	GtkFileFilter *filter;
	gchar *filename;

	chooser = gtk_file_chooser_dialog_new(_("Open File"),
	                                      NULL,
	                                      GTK_FILE_CHOOSER_ACTION_OPEN,
	                                      GTK_STOCK_CANCEL,
	                                      GTK_RESPONSE_CANCEL,
	                                      GTK_STOCK_OPEN,
	                                      GTK_RESPONSE_ACCEPT,
	                                      NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Executable file"));
	gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, _preferences_dialog_browser_file_filter, NULL, NULL);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser), filter);

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		gtk_entry_set_text(GTK_ENTRY(private->entry_browser), filename);
		g_free(filename);
	}

	gtk_widget_destroy(chooser);
}

static GtkWidget *
_preferences_dialog_create_browser_frame(GtkWidget *dialog)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *subframe;
	GtkWidget *framehbox;
	GtkWidget *framevbox;
	GtkWidget *buttonbox;
	GtkWidget *label;
	UrlOpener *opener;

	/* create frame */
	frame = gtk_frame_new(_("Browser preferences"));

	/* create container */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(hbox), vbox);

	/* create subframe */
	subframe = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(vbox), subframe, FALSE, FALSE, 0);

	framehbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(subframe), framehbox);

	framevbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(framehbox), framevbox, FALSE, FALSE, 5);

	/* insert label */
	label = gtk_label_new(_("Please specifiy your favorite internet browser."));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(framevbox), label, FALSE, FALSE, 5);

	/* create toggle buttons */	
	private->toggle_default_browser = gtk_radio_button_new_with_label(NULL, _("use default browser"));
	gtk_box_pack_start(GTK_BOX(framevbox), private->toggle_default_browser, FALSE, FALSE, 0);

	private->toggle_custom_browser = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(private->toggle_default_browser)), _("use custom browser"));
	gtk_box_pack_start(GTK_BOX(framevbox), private->toggle_custom_browser, FALSE, FALSE, 0);

	/* insert entry & button */	
	buttonbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(framevbox), buttonbox, FALSE, FALSE, 5);

	gtk_box_pack_start(GTK_BOX(buttonbox), gtk_image_new_from_stock(GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_BUTTON), FALSE, FALSE, 2);

	private->entry_browser = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(private->entry_browser), FALSE);
	gtk_box_pack_start(GTK_BOX(buttonbox), private->entry_browser, FALSE, FALSE, 0);

	private->button_open_browser = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start(GTK_BOX(buttonbox), private->button_open_browser, FALSE, FALSE, 2);

	/* try to get default UrlOpener */
	if(!(opener = url_opener_new_default()))
	{
		gtk_widget_set_sensitive(private->toggle_default_browser, FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->toggle_custom_browser), TRUE);
	}
	else
	{
		url_opener_free(opener);
	}

	/* signals */
	g_signal_connect(private->toggle_default_browser, "toggled", G_CALLBACK(_preferences_dialog_browser_button_toggled), (gpointer)dialog);
	g_signal_connect(private->button_open_browser, "clicked", G_CALLBACK(_preferences_dialog_open_browser_clicked), (gpointer)dialog);

	/* update widgets */
	_preferences_dialog_update_browser_widgets(dialog);

	return frame;
}

/*
 *	view:
 */
static void
_preferences_dialog_update_view_widgets(GtkWidget *dialog)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	gboolean enabled;

	enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->checkbox_notification_area));
	gtk_widget_set_sensitive(private->toggle_notification_level_debug, enabled);
	gtk_widget_set_sensitive(private->toggle_notification_level_info, enabled);
	gtk_widget_set_sensitive(private->toggle_notification_level_warning, enabled);
}

static void
_preferences_dialog_notification_button_toggled(GtkWidget *widget, GtkWidget *dialog)
{
	_preferences_dialog_update_view_widgets(dialog);
}

static GtkWidget *
_preferences_dialog_create_view_frame(GtkWidget *dialog)
{
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate *)g_object_get_data(G_OBJECT(dialog), "private");
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *subframe;
	GtkWidget *framehbox;
	GtkWidget *framevbox;
	GtkWidget *table;
	GtkWidget *align;
	GtkWidget *label;
	GSList *group;

	/* create frame */
	frame = gtk_frame_new(_("View"));

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(hbox), vbox);

	/* colors */
	subframe = gtk_frame_new(_("Colors"));
	gtk_box_pack_start(GTK_BOX(vbox), subframe, FALSE, FALSE, 0);

	framehbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(subframe), framehbox);

	framevbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(framehbox), framevbox, FALSE, FALSE, 5);

	table = gtk_table_new(1, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(framevbox), table, FALSE, TRUE, 2);

	align = gtk_alignment_new(0, 0.5, 0, 0);
	label = gtk_label_new(_("Tweet background"));
	gtk_container_add(GTK_CONTAINER(align), label);
	gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, 0, 1);

	private->button_color = gtk_color_button_new();
	gtk_table_attach_defaults(GTK_TABLE(table), private->button_color, 1, 2, 0, 1);

	/* notification area */
	subframe = gtk_frame_new(_("Notifications"));
	gtk_box_pack_start(GTK_BOX(vbox), subframe, FALSE, FALSE, 5);

	framehbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(subframe), framehbox);

	framevbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(framehbox), framevbox, FALSE, FALSE, 5);

	private->checkbox_notification_area = gtk_check_button_new_with_label(_("display notification area"));
	gtk_box_pack_start(GTK_BOX(framevbox), private->checkbox_notification_area, FALSE, FALSE, 2);

	private->toggle_notification_level_debug = gtk_radio_button_new_with_label(NULL, _("show all messages"));
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(private->toggle_notification_level_debug));
	gtk_box_pack_start(GTK_BOX(framevbox), private->toggle_notification_level_debug, FALSE, FALSE, 2);

	private->toggle_notification_level_info = gtk_radio_button_new_with_label(group, _("show notifications and warnings"));
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(private->toggle_notification_level_info));
	gtk_box_pack_start(GTK_BOX(framevbox), private->toggle_notification_level_info, FALSE, FALSE, 2);

	private->toggle_notification_level_warning = gtk_radio_button_new_with_label(group, _("show only warnings"));
	gtk_box_pack_start(GTK_BOX(framevbox), private->toggle_notification_level_warning, FALSE, FALSE, 2);

	/* update widgets */
	_preferences_dialog_update_view_widgets(dialog);

	/* signal */
	g_signal_connect(private->checkbox_notification_area, "toggled", G_CALLBACK(_preferences_dialog_notification_button_toggled), (gpointer)dialog);

	return frame;
}

/*
 *	categories:
 */
static void
_preferences_dialog_category_cursor_changed(GtkTreeView *tree, gpointer user_data)
{
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	GtkTreePath *path;
	GtkTreeIter iter;
	PREFERENCE_CATEGORY_ID id;

	/* get selected row & read category id */
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(tree), &path, NULL);
	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, PREFERENCES_TREEVIEW_COLUMN_ID, &id, -1);

		/* display frame */
		_preferences_dialog_show_frame((GtkWidget *)user_data, id);
	}

	/* free memory  */
	gtk_tree_path_free(path);
}

static GtkWidget *
_preferences_dialog_create_category_tree(GtkWidget *dialog)
{
	GtkWidget *tree;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;

	/* create treeview & model */
	tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);
	gtk_widget_set_size_request(tree, 100, -1);

	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));

	/* insert column to display text */
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, PREFERENCES_TREEVIEW_COLUMN_TEXT);
	gtk_tree_view_column_add_attribute(column, renderer, "text", PREFERENCES_TREEVIEW_COLUMN_TEXT);

	/* insert categories */
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, PREFERENCES_TREEVIEW_COLUMN_TEXT, _("General"), PREFERENCES_TREEVIEW_COLUMN_ID, PREFERENCES_CATEGORY_ID_GENERAL, -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, PREFERENCES_TREEVIEW_COLUMN_TEXT, _("Browser"), PREFERENCES_TREEVIEW_COLUMN_ID, PREFERENCES_CATEGORY_ID_BROWSER, -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, PREFERENCES_TREEVIEW_COLUMN_TEXT, _("View"), PREFERENCES_TREEVIEW_COLUMN_ID, PREFERENCES_CATEGORY_ID_VIEW, -1);

	/* register signals */
	g_signal_connect(G_OBJECT(tree), "cursor-changed", G_CALLBACK(_preferences_dialog_category_cursor_changed), (gpointer)dialog);

	return tree;
}

/*
 *	public:
 */
GtkWidget *
preferences_dialog_create(GtkWidget *parent)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *frame;
	_PreferenceWindowPrivate *private = (_PreferenceWindowPrivate *)g_malloc(sizeof(_PreferenceWindowPrivate));

	dialog = gtk_dialog_new_with_buttons(_("Preferences"),
	                                     GTK_WINDOW(parent),
	                                     GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
	                                     GTK_STOCK_APPLY,
	                                     GTK_RESPONSE_APPLY,
	                                     GTK_STOCK_CANCEL,
	                                     GTK_RESPONSE_CANCEL,
	                                     NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	g_object_set_data(G_OBJECT(dialog), "private", (gpointer)private);

	/* mainbox */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);

	/* create category list */
	frame = gtk_frame_new(NULL);
	private->tree_categories = _preferences_dialog_create_category_tree(dialog);
	gtk_container_add(GTK_CONTAINER(frame), private->tree_categories);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);

	/* general */
	private->frames[PREFERENCES_CATEGORY_ID_GENERAL] = _preferences_dialog_create_general_frame(dialog);

	/* browser preferences */
	private->frames[PREFERENCES_CATEGORY_ID_BROWSER] = _preferences_dialog_create_browser_frame(dialog);

	/* view */
	private->frames[PREFERENCES_CATEGORY_ID_VIEW] = _preferences_dialog_create_view_frame(dialog);

	/* insert frames */
	for(gint i = 0; i < sizeof(private->frames) / sizeof(private->frames[0]); ++i)
	{
		gtk_box_pack_start(GTK_BOX(hbox), private->frames[i], TRUE, TRUE, 5);
	}

	/* populate settings */
	_preferences_dialog_populate(dialog, parent);

	/* set size & show widgets */
	gtk_widget_set_size_request(dialog, 600, 300);
	gtk_widget_show_all(dialog);

	/* signals */
	g_signal_connect(dialog, "response", G_CALLBACK(_preferences_dialog_response), (gpointer)parent);
	g_signal_connect(dialog, "destroy", G_CALLBACK(_preferences_dialog_destroy), NULL);

	return dialog;
}

/**
 * @}
 */
#endif


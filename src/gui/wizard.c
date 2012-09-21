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
 * \file wizard.c
 * \brief Startup wizard.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 21. September 2012
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>
#include <string.h>

#include "wizard.h"
#include "gtk_helpers.h"
#include "../application.h"
#include "../pathbuilder.h"
#include "../urlopener.h"
#include "../twitter.h"
#include "../oauth/twitter_oauth.h"

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Wizard
 * 	@{
 */

enum
{
	WIZARD_PAGE_WELCOME,
	WIZARD_PAGE_DEFAULT_BROWSER,
	WIZARD_PAGE_ACCOUNT,
	WIZARD_PAGE_CONFIRM
};

/**
 * \struct _WizardWindowPrivate
 * \brief Private data of the wizard.
 */
typedef struct
{
	/*! Radio button to select the default browser. */
	GtkWidget *toggle_default_browser;
	/*! Radio button to select a custom browser. */
	GtkWidget *toggle_custom_browser;
	/*! Button to open an executable file. */
	GtkWidget *button_open_browser;
	/*! Textbox holding the filename of a selected executable. */
	GtkWidget *entry_browser;
	/*! An UrlOpener. */
	UrlOpener *url_opener;
	/*! Textbox to enter the username of the Twitter account. */
	GtkWidget *entry_username;
	/*! Textbox to enter the password of the Twitter account. */
	GtkWidget *entry_pin;
	/*! Button to get a PIN. */
	GtkWidget *button_get_pin;
	/*! Button to start the PIN authorization. */
	GtkWidget *button_authorize;
	/*! Wizard status. */
	gboolean completed;
	/*! OAuth request key. */
	gchar *request_key;
	/*! OAuth request secret. */
	gchar *request_secret;
	/*! OAuth access key. */
	gchar *access_key;
	/*! OAuth access secret. */
	gchar *access_secret;
} _WizardWindowPrivate;

/*
 *	helpers:
 */
static void
_wizard_enable(GtkAssistant *assistant, gboolean enabled)
{
	GdkCursor *cursor;
	GdkCursorType type;

	/* update cursor */
	type = enabled ? GDK_LEFT_PTR : GDK_WATCH;
	cursor = gdk_cursor_new(type);
	gdk_window_set_cursor(GTK_WIDGET(assistant)->window, cursor);
	gdk_cursor_unref(cursor);

	/* update window */
	gtk_widget_set_sensitive(GTK_WIDGET(assistant), enabled);
}

static void
_wizard_save_account(Config *config, const gchar *username, const gchar *access_key, const gchar *access_secret, gboolean custom_browser, const gchar *browser_executable)
{
	Section *root;
	Section *browser;
	Section *twitter;
	Section *accounts;
	Section *account;

	root = config_get_root(config);

	if(!(twitter = section_find_first_child(root, "Twitter")))
	{
		twitter = section_append_child(root, "Twitter");
	}

	if(!(accounts = section_find_first_child(twitter, "Accounts")))
	{
		accounts = section_append_child(twitter, "Accounts");
	}

	account = section_append_child(accounts, "Account");
	value_set_string(section_append_value(account, "username", VALUE_DATATYPE_STRING), username);
	value_set_string(section_append_value(account, "oauth_access_key", VALUE_DATATYPE_STRING), access_key);
	value_set_string(section_append_value(account, "oauth_access_secret", VALUE_DATATYPE_STRING), access_secret);

	if(!(browser = section_find_first_child(root, "Browser")))
	{
		browser = section_append_child(root, "Browser");
	}

	value_set_bool(section_append_value(browser, "custom", VALUE_DATATYPE_BOOLEAN), custom_browser);
	if(custom_browser && browser_executable)
	{
		value_set_string(section_append_value(browser, "executable", VALUE_DATATYPE_STRING), browser_executable);
	}
}

/*
 *	welcome page:
 */
static GtkWidget *
_wizard_page_welcome_create(void)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *image;
	GtkWidget *label;
	gchar *str;

	/* create container */
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);

	/* insert image  */
	vbox = gtk_vbox_new(FALSE, 0);
	str = pathbuilder_build_image_path("wizard.png");
	image = gtk_image_new_from_file(str);
	g_free(str);
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	/* insert label */
	vbox = gtk_vbox_new(FALSE, 0);
	str = g_strdup_printf(_("<b>Welcome to %s!</b>\n\n"
	                        "You have no Twitter accounts configured yet. This wizard will guide you "
	                        "through all necessary configuration steps. Please click the <b>Forward</b> "
	                        "button to create your first account."),
	                      APPLICATION_NAME);
	label = gtk_label_new(str);
	g_free(str);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	return hbox;
}

/*
 *	browser page:
 */
static void
_wizard_page_browser_update_widgets(GtkAssistant *assistant)
{
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");
	GtkWidget *page = gtk_assistant_get_nth_page(assistant, gtk_assistant_get_current_page(assistant));
	gboolean use_custom_browser = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->toggle_custom_browser));

	gtk_widget_set_sensitive(private->entry_browser, use_custom_browser);
	gtk_widget_set_sensitive(private->button_open_browser, use_custom_browser);

	gtk_assistant_set_page_complete(assistant, page, TRUE);
	if(use_custom_browser)
	{
		if(!strlen(gtk_entry_get_text(GTK_ENTRY(private->entry_browser))))
		{
			gtk_assistant_set_page_complete(assistant, page, FALSE);
		}
	}
}

static void
_wizard_page_browser_set_url_opener(GtkAssistant *assistant)
{
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");
	const gchar *filename;

	if(private->url_opener)
	{
		url_opener_free(private->url_opener);
		private->url_opener = NULL;
	}

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->toggle_default_browser)))
	{
		private->url_opener = url_opener_new_default();
	}
	else
	{
		if((filename = gtk_entry_get_text(GTK_ENTRY(private->entry_browser))))
		{
			if(strlen(filename))
			{
				private->url_opener = url_opener_new_executable(filename, NULL);
			}
		}
	}
}

static void
_wizard_page_account_button_toggled(GtkWidget *widget, GtkAssistant *assistant)
{
	_wizard_page_browser_update_widgets(assistant);
	_wizard_page_browser_set_url_opener(assistant);
}

static gboolean
_wizard_page_browser_file_filter(const GtkFileFilterInfo *filter_info, gpointer data)
{
	return g_file_test(filter_info->filename, G_FILE_TEST_IS_EXECUTABLE);
}

static void
_wizard_page_browser_button_open_browser_clicked(GtkWidget *widget, GtkAssistant *assistant)
{
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");
	GtkWidget *dialog;
	GtkFileFilter *filter;
	gchar *filename;

	dialog = gtk_file_chooser_dialog_new (_("Open File"),
	                                      NULL,
	                                      GTK_FILE_CHOOSER_ACTION_OPEN,
	                                      GTK_STOCK_CANCEL,
	                                      GTK_RESPONSE_CANCEL,
	                                      GTK_STOCK_OPEN,
	                                      GTK_RESPONSE_ACCEPT,
	                                      NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Executable file"));
	gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, _wizard_page_browser_file_filter, NULL, NULL);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_entry_set_text(GTK_ENTRY(private->entry_browser), filename);
		g_free(filename);

		/* update widgets & create UrlOpener */
		_wizard_page_browser_update_widgets(assistant);
		_wizard_page_browser_set_url_opener(assistant);
	}

	gtk_widget_destroy(dialog);
}

static GtkWidget *
_wizard_page_browser_create(GtkAssistant *assistant)
{
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *buttonbox;
	gchar *path;

	/* create container */
	hbox = gtk_hbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);

	/* insert image  */
	vbox = gtk_vbox_new(FALSE, 0);
	path = pathbuilder_build_image_path("browser.png");
	image = gtk_image_new_from_file(path);
	g_free(path);
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* insert label */
	vbox = gtk_vbox_new(FALSE, 8);
	label = gtk_label_new(_("Please specifiy your favorite internet browser."));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* create toggle buttons */
	private->toggle_default_browser = gtk_radio_button_new_with_label(NULL, _("use default browser"));
	gtk_box_pack_start(GTK_BOX(vbox), private->toggle_default_browser, FALSE, FALSE, 0);

	private->toggle_custom_browser = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(private->toggle_default_browser)), _("use custom browser"));
	gtk_box_pack_start(GTK_BOX(vbox), private->toggle_custom_browser, FALSE, FALSE, 0);

	/* insert entry & button */	
	buttonbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(buttonbox), gtk_image_new_from_stock(GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_BUTTON), FALSE, FALSE, 5);

	private->entry_browser = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(private->entry_browser), FALSE);
	gtk_box_pack_start(GTK_BOX(buttonbox), private->entry_browser, FALSE, FALSE, 0);

	private->button_open_browser = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start(GTK_BOX(buttonbox), private->button_open_browser, FALSE, FALSE, 1);

	/* try to get default UrlOpener */
	if(!(private->url_opener = url_opener_new_default()))
	{
		gtk_widget_set_sensitive(private->toggle_default_browser, FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(private->toggle_custom_browser), TRUE);
	}

	/* signals */
	g_signal_connect(private->toggle_default_browser, "toggled", G_CALLBACK(_wizard_page_account_button_toggled), (gpointer)assistant);
	g_signal_connect(private->button_open_browser, "clicked", G_CALLBACK(_wizard_page_browser_button_open_browser_clicked), (gpointer)assistant);

	return hbox;
}


/*
 *	account page:
 */
static void
_wizard_page_account_entry_changed(GtkWidget *widget, GtkAssistant *assistant)
{
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");
	const char *text;
	gint length;

	gtk_widget_set_sensitive(private->button_authorize, FALSE);

	/* check length of the username */
	if(!strlen(gtk_entry_get_text(GTK_ENTRY(private->entry_username))))
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

	gtk_widget_set_sensitive(private->button_authorize, TRUE);
}

static gboolean
_wizard_page_account_get_pin_worker(GtkAssistant *assistant)
{
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");
	gchar *request_key = NULL;
	gchar *request_secret = NULL;
	GtkWidget *dialog = NULL;
	GError *err = NULL;

	g_debug("Twitter pin worker started");

	g_assert(private->url_opener != NULL);

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
	_wizard_enable(assistant, FALSE);

	gtk_main_iteration();

	/* get request key & secret */
	if(twitter_request_authorization(private->url_opener, &request_key, &request_secret, &err))
	{
		private->request_key = request_key;
		private->request_secret = request_secret;

		/* enable PIN entry field */
		gtk_widget_set_sensitive(GTK_WIDGET(private->entry_pin), TRUE);
		gtk_widget_grab_focus(private->entry_pin);
	}
	else
	{
		/* display failure message */
		if(err)
		{
			dialog = gtk_message_dialog_new(GTK_WINDOW(assistant), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", err->message);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			g_error_free(err);
		}
	}

	/* enable window & set default cursor */
	_wizard_enable(assistant, TRUE);

	/* unlock GUI */
	gdk_threads_leave();

	g_debug("Twitter pin worker finished");

	return FALSE;
}

static void
_wizard_page_account_button_get_pin_clicked(GtkWidget *widget, GtkAssistant *assistant)
{
	g_debug("Starting worker...");
	g_idle_add((GSourceFunc)_wizard_page_account_get_pin_worker, assistant);
	g_debug("Leaving event callback");
}

static gboolean
_wizard_page_account_authorization_worker(GtkAssistant *assistant)
{
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");
	GtkWidget *page;
	gchar *access_key = NULL;
	gchar *access_secret = NULL;
	gboolean success;
	GtkWidget *dialog;

	g_debug("Twitter authorization worker started");

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

	/* lock GUI */
	gdk_threads_enter();

	/* disable window & set busy cursor */
	_wizard_enable(assistant, FALSE);

	gtk_main_iteration();

	/* get page  */
	page = gtk_assistant_get_nth_page(assistant, gtk_assistant_get_current_page(assistant));

	gtk_main_iteration();

	/* try to get access token & secret */
	success = twitter_oauth_access_token(private->request_key,
	                                     private->request_secret,
	                                     gtk_entry_get_text(GTK_ENTRY(private->entry_pin)),
	                                     &access_key,
	                                     &access_secret);

	/* enable window & set default cursor */
	_wizard_enable(assistant, TRUE);

	if(success)
	{
		private->access_key = access_key;
		private->access_secret = access_secret;

		/* update widgets */
		gtk_assistant_set_page_complete(assistant, page, TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(private->entry_username), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(private->entry_pin), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(private->button_get_pin), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(private->button_authorize), FALSE);

		/* create message dialog */
		dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(assistant),
		                                            GTK_DIALOG_MODAL,
		                                            GTK_MESSAGE_INFO,
		                                            GTK_BUTTONS_OK,
		                                            _("<b>%s</b> has been authorized successfully."),
		                                            APPLICATION_NAME);

	}
	else
	{
		/* create message dialog */
		dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(assistant),
		                                            GTK_DIALOG_MODAL,
		                                            GTK_MESSAGE_ERROR,
		                                            GTK_BUTTONS_OK,
		                                            _("Couldn't authorize <b>%s</b>, please try again later."),
		                                            APPLICATION_NAME);

		/* free memory */
		if(access_key)
		{
			g_free(access_key);
		}

		if(access_secret)
		{
			g_free(access_secret);
		}
	}

	/* run message dialog */
	gtk_dialog_run(GTK_DIALOG(dialog));

	/* destroy message dialog */
	if(GTK_IS_WIDGET(dialog))
	{
		gtk_widget_destroy(dialog);
	}

	/* unlock GUI */
	gdk_threads_leave();

	g_debug("Twitter authorization worker finished");

	return FALSE;
}

static void
_wizard_page_account_button_authorize_clicked(GtkWidget *widget, GtkAssistant *assistant)
{
	g_debug("Starting worker...");
	g_idle_add((GSourceFunc)_wizard_page_account_authorization_worker, assistant);
	g_debug("Leaving event handler");
}

static GtkWidget *
_wizard_page_account_create(GtkAssistant *assistant)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *hbox_buttons;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *table;
	gchar *str;
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");

	private->request_key = NULL;
	private->request_secret = NULL;
	private->access_key = NULL;
	private->access_secret = NULL;

	/* create container */
	hbox = gtk_hbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);

	/* insert image  */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	str = pathbuilder_build_image_path("account.png");
	image = gtk_image_new_from_file(str);
	g_free(str);
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

	/* insert label */
	vbox = gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	str = g_strdup_printf(_("Please enter your username. Like any other Twitter application <b>%s</b> has to be authorized. "
	                        "Please enter the PIN you were provided and click <b>Authorize</b>."), APPLICATION_NAME);
	label = gtk_label_new(str);
	g_free(str);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* create table */
	table = gtk_table_new(3, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 1);

	label = gtk_label_new(_("Username"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new("PIN");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 5);

	private->entry_username = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(private->entry_username), 32);
	gtk_table_attach(GTK_TABLE(table), private->entry_username, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, 0, 0, 0);

	private->entry_pin = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(private->entry_pin), 32);
	gtk_widget_set_sensitive(private->entry_pin, FALSE);
	gtk_table_attach(GTK_TABLE(table), private->entry_pin, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);

	hbox_buttons = gtk_hbox_new(TRUE, 5);
	gtk_table_attach(GTK_TABLE(table), hbox_buttons, 1, 2, 2, 3, GTK_FILL, 0, 0, 5);

	private->button_get_pin = gtk_button_new_with_label(_("Get PIN"));
	gtk_box_pack_start(GTK_BOX(hbox_buttons), private->button_get_pin, TRUE, TRUE, 0);

	private->button_authorize = gtk_button_new_with_label(_("Authorize"));
	gtk_box_pack_start(GTK_BOX(hbox_buttons), private->button_authorize, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(private->button_authorize, FALSE);

	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	/* signals */
	g_signal_connect(G_OBJECT(private->entry_username), "changed", G_CALLBACK(_wizard_page_account_entry_changed), (gpointer)assistant);
	g_signal_connect(G_OBJECT(private->entry_pin), "changed", G_CALLBACK(_wizard_page_account_entry_changed), (gpointer)assistant);
	g_signal_connect(G_OBJECT(private->button_get_pin), "clicked", G_CALLBACK(_wizard_page_account_button_get_pin_clicked), (gpointer)assistant);
	g_signal_connect(G_OBJECT(private->button_authorize), "clicked", G_CALLBACK(_wizard_page_account_button_authorize_clicked), (gpointer)assistant);

	return hbox;
}

/*
 *	confirm page:
 */
static GtkWidget *
_wizard_page_confirm_create(void)
{
	GtkWidget *hbox;
	GtkWidget *label;
	gchar *text;

	/* create container */
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);

	/* insert label */
	text = g_strdup_printf(_("Please click on <b>Apply</b> to save all settings and start <b>%s</b>."), APPLICATION_NAME);
	label = gtk_label_new(text);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

	return hbox;
}

/*
 *	assistant:
 */
static void
_wizard_assistant_closed(GtkAssistant *assistant, gpointer user_data)
{
	_WizardWindowPrivate *private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");

	/* set result & leave mainloop */
	private->completed = TRUE;
	gtk_main_quit();
}

static void
_wizard_assistant_prepare(GtkAssistant *assistant, GtkWidget *page, gpointer user_data)
{
	if(gtk_assistant_get_current_page(assistant) == WIZARD_PAGE_DEFAULT_BROWSER)
	{
		_wizard_page_browser_update_widgets(assistant);
	}
}

static void
_wizard_assistant_append_page(GtkAssistant *assistant, const gchar *title, GtkWidget *child, GtkAssistantPageType type, gboolean completed)
{
	gtk_assistant_append_page(assistant, child);
	gtk_assistant_set_page_title(assistant, child, title);
	gtk_assistant_set_page_type(assistant, child, type);
	gtk_assistant_set_page_complete(assistant, child, completed);
}

static GtkWidget *
_wizard_assistant_create(void)
{
	GtkWidget *assistant;
	_WizardWindowPrivate *private;

	assistant = gtk_assistant_new();
	gtk_window_set_position(GTK_WINDOW(assistant), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(assistant), APPLICATION_NAME);

	private = (_WizardWindowPrivate *)g_malloc(sizeof(_WizardWindowPrivate));
	private->completed = FALSE;
	private->url_opener = NULL;
	private->request_key = NULL;
	private->request_secret = NULL;
	private->access_key = NULL;
	private->access_secret = NULL;
	g_object_set_data(G_OBJECT(assistant), "private", (gpointer)private);

	/* pages */
	_wizard_assistant_append_page(GTK_ASSISTANT(assistant),
	                              _("Welcome"),
	                              _wizard_page_welcome_create(),
	                              GTK_ASSISTANT_PAGE_INTRO,
	                              TRUE);

	_wizard_assistant_append_page(GTK_ASSISTANT(assistant),
	                              _("Internet Browser"),
	                              _wizard_page_browser_create(GTK_ASSISTANT(assistant)),
	                              GTK_ASSISTANT_PAGE_CONTENT,
	                              FALSE);

	_wizard_assistant_append_page(GTK_ASSISTANT(assistant),
	                              _("Account settings"),
	                              _wizard_page_account_create(GTK_ASSISTANT(assistant)),
	                              GTK_ASSISTANT_PAGE_CONTENT,
	                              FALSE);

	_wizard_assistant_append_page(GTK_ASSISTANT(assistant),
	                              _("Finish configuration"),
	                              _wizard_page_confirm_create(),
	                              GTK_ASSISTANT_PAGE_CONFIRM,
	                              TRUE);

	/* signals */
	g_signal_connect(G_OBJECT(assistant), "cancel", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(assistant), "close", G_CALLBACK(_wizard_assistant_closed), (gpointer)assistant);
	g_signal_connect(G_OBJECT(assistant), "prepare", G_CALLBACK(_wizard_assistant_prepare), (gpointer)assistant);

	gtk_widget_show_all(assistant);

	return assistant;
}

/*
 *	public:
 */
gboolean
wizard_start(Config *config)
{
	GtkWidget *assistant;
	_WizardWindowPrivate *private;
	gboolean result;

	/* create & start assistant */
	assistant = _wizard_assistant_create();
	gtk_helpers_set_window_icon_from_image_folder(assistant, "wizard.png");
	gtk_main();

	/* get result */
	private = (_WizardWindowPrivate *)g_object_get_data(G_OBJECT(assistant), "private");
	if((result = private->completed))
	{
		/* save account */
		_wizard_save_account(config,
		                     gtk_entry_get_text(GTK_ENTRY(private->entry_username)),
		                     private->access_key,
		                     private->access_secret,
		                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(private->toggle_custom_browser)),
		                     gtk_entry_get_text(GTK_ENTRY(private->entry_browser)));
	}

	/* free memory  */
	if(private->request_key)
	{
		g_free(private->request_key);
	}

	if(private->request_secret)
	{
		g_free(private->request_secret);
	}

	if(private->access_key)
	{
		g_free(private->access_key);
	}

	if(private->access_secret)
	{
		g_free(private->access_secret);
	}

	if(private->url_opener)
	{
		url_opener_free(private->url_opener);
	}

	g_free(private);
	gtk_widget_destroy(assistant);

	return result;
}

/**
 * @}
 * @}
 */


/***************************************************************************
    begin........: June 2010
    copyright....: Sebastian Fedrau
    email........: lord-kefir@arcor.de
 ***************************************************************************/

/***************************************************************************
    this program is free software; you can redistribute it and/or modify
    it under the terms of the gnu general public license as published by
    the free software foundation.

    this program is distributed in the hope that it will be useful,
    but without any warranty; without even the implied warranty of
    merchantability or fitness for a particular purpose. see the gnu
    general public license for more details.
 ***************************************************************************/
/*!
 * \file statustab.c
 * \brief A tab containing twitter statuses.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 22. January 2012
 */

#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <string.h>

#include "statustab.h"
#include "tabbar.h"
#include "mainwindow.h"
#include "gtktwitterstatus.h"
#include "pixbuf_helpers.h"
#include "edit_list_membership_dialog.h"
#include "edit_members_dialog.h"
#include "gtk_helpers.h"
#include "gtkuserlistdialog.h"
#include "list_preferences_dialog.h"
#include "remove_list_dialog.h"
#include "composer_dialog.h"
#include "retweet_dialog.h"
#include "select_account_dialog.h"
#include "replies_dialog.h"
#include "../twitterdb.h"
#include "../urlopener.h"
#include "../helpers.h"
#include "../pathbuilder.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \struct _StatusTab
 * \brief Widgets and data of the statustab.
 */
typedef struct
{
	/*! Padding. */
	Tab padding;
	/*! An unique id. */
	guint tab_id;
	/*! Name of the owner. */
	gchar *owner;
	/*! TRUE if tab has been initialized. */
	gboolean initialized;
	/*! The page widget. */
	GtkWidget *page;
	/*! Box containing tweets. */
	GtkWidget *vbox;
	/*! Box containing buttons. */
	GtkWidget *action_area;
	/*! Scrolled window. */
	GtkWidget *scrolled;
	/*! The related tabbar. */
	GtkWidget *tabbar;
	/*! A GCancellable to abort database operations. */
	GCancellable *cancellable;
	/**
	 * \struct _worker
	 * \brief Holds background worker thread and message data.
	 *
	 * \var worker
	 * \brief Background worker thread and message data.
	 */
	struct _worker
	{
		/*! Thread of the background worker. */
		GThread *thread;
		/*! Condition to send signals to the thread. */
		GCond *condition;
		/*! Mutex protecting the condition. */
		GStaticMutex mutex;
		/*! Value of the signal. */
		gint signal;
	} worker;
	/**
	 * \struct _status
	 * \brief Holds tab "waiting" flag and related mutex.
	 *
	 * \var status
	 * Status flag of the tab.
	 */
	struct _status
	{
		/*! TRUE if worker thread is waiting. */
		gboolean waiting;
		/*! Mutex protecting waiting status. */
		GMutex *mutex;
	} status;
	/**
	 * \struct _accountlist
	 * \brief Holds a list containing accounts and a related mutex.
	 *
	 * \var accountlist
	 * \brief A list containing registered accounts.
	 */
	struct _accountlist
	{
		/*! A list containing accounts. */
		gchar **accounts;
		/*! Mutex protecting the list. */
		GMutex *mutex;
	} accountlist;
	/*! The background color. */
	gchar *background_color;
	/*! Indicates if background color has been changed. */
	gboolean background_changed;
} _StatusTab;

enum
{
	/*! Destroy tab. */
	STATUS_TAB_SIGNAL_DESTROY = 1,
	/*! Refresh tab. */
	STATUS_TAB_SIGNAL_REFRESH = 2,
	/*! Do nothing. */
	STATUS_TAB_SIGNAL_IDLE = 3
};

/**
 * \struct _StatusTabListWorkerArg
 * \brief Holds a tab, a dialog and list information.
 */
typedef struct
{
	/*! Tab information. */
	_StatusTab *tab;
	/*! A dialog. */
	GtkWidget *dialog;
	/*! Username. */
	gchar username[64];
	/*! Listname. */
	gchar listname[64];
}
_StatusTabListWorkerArg;

/**
 * \struct _StatusTabFriendshipWorkerArg
 * \brief Holds a tab, a dialog and a username.
 */
typedef struct
{
	/*! Tab information. */
	_StatusTab *tab;
	/*! A dialog. */
	GtkWidget *dialog;
	/*! Username. */
	gchar username[64];
}
_StatusTabFriendshipWorkerArg;

/**
 * \struct _RetweetArg
 * \brief Holds data required to retweet a status.
 */
typedef struct
{
	/*! A "select status" dialog.  */
	GtkWidget *dialog;
	/*! The Mainwindow. */
	GtkWidget *mainwindow;
	/*! Guid of the status to retweet. */
	gchar guid[32];
	/*! An user account. */
	gchar *account;
} _RetweetArg;

/*
 *	url handling:
 */
static void
_status_tab_open_browser(_StatusTab *tab, const gchar *url)
{
	GtkWidget *window;
	Config *config;
	UrlOpener *opener;

	g_debug("Opening \"%s\" in browser", url);

	/* create UrlOpener */
	window = tabbar_get_mainwindow(tab->tabbar);
	config = mainwindow_lock_config(window);
	opener = helpers_create_urlopener_from_config(config);
	mainwindow_unlock_config(window);

	if(opener)
	{
		/* open Url in browser */
		url_opener_launch(opener, url);
		url_opener_free(opener);
	}
}

static gboolean
_status_tab_url_check_username(const gchar *text)
{
	while(text[0])
	{
		if(text[0] == '/')
		{
			return FALSE;
		}

		++text;
	}

	return TRUE;
}

static void
_status_tab_url_activated(GtkTwitterStatus *status, const gchar *url, _StatusTab *tab)
{
	gchar *lower = NULL;
	const gchar *part = NULL;
	gchar *query = NULL;
	gboolean open_browser = TRUE;

	g_debug("Handling url: \"%s\"", url);

	/* check if target is "twitter.com" */
	if(strlen(url) > 24)
	{
		lower = g_ascii_strdown(url, -1);
		part = (g_str_has_prefix(lower, "http://")) ? lower + 7 : lower;

		if(g_str_has_prefix(part, "www."))
		{
			part += 4;
		}

		if(g_str_has_prefix(part, "twitter.com/"))
		{
			part += 12;
		}

		if(g_str_has_prefix(part, "#!/"))
		{
			part += 3;
		}

		if(g_str_has_prefix(part, "search?q=%23"))
		{
			/* open search tab */
			query = g_strdup_printf("#%s", part + 12);
			g_debug("Found search query url: \"%s\"", query);
			tabbar_open_search_query(tab->tabbar, query);
			open_browser = FALSE;
		}
		else
		{
			if(_status_tab_url_check_username(part))
			{
				/* open user timline */
				g_debug("Found usertimeline url: \"%s\"", part);
				tabbar_open_user_timeline(tab->tabbar, part);
				open_browser = FALSE;
			}
		}

		/* cleanup */
		g_free(query);
		g_free(lower);
	}

	/* open url in browser */
	if(open_browser)
	{
		_status_tab_open_browser(tab, url);
	}
}

static void
_status_tab_username_activated(GtkTwitterStatus *status, const gchar *username, _StatusTab *tab)
{
	g_debug("Found usertimeline url: \"%s\"", username);
	tabbar_open_user_timeline(tab->tabbar, username);
}

/*
 *	helpers:
 */
static glong
_status_tab_get_tooltip_diff(const gchar *username, GQuark *last_user, glong *last_query)
{
	GTimeVal now;
	glong diff = -1;

	if(*last_user == g_quark_from_string(username))
	{
		g_get_current_time(&now);

		if(*last_query)
		{
			diff = now.tv_sec - *last_query;
		}

		*last_query = now.tv_sec;
	}
	else
	{
		*last_user = g_quark_from_string(username);
	}

	return diff;
}

static gchar **
_status_tab_get_accounts(Config *config)
{
	Section *section;
	Section *child;
	Value *value;
	gint count;
	gint length = 0;
	gchar **accounts = NULL;

	section = config_get_root(config);

	if((section = section_find_first_child(section, "Twitter")) && (section = section_find_first_child(section, "Accounts")))
	{
		count = section_n_children(section);
		accounts = (gchar **)g_malloc(sizeof(gchar *) * (count + 1));

		for(gint i = 0; i < count; ++i)
		{
			child = section_nth_child(section, i);

			if(!g_ascii_strcasecmp(section_get_name(child), "Account"))
			{
				if((value = section_find_first_value(child, "username")) && VALUE_IS_STRING(value))
				{
					++length;
					accounts[length - 1] = g_strdup(value_get_string(value));
				}
			}
		}

		++length;
		accounts[length - 1] = NULL;
	}

	return accounts;
}

static gboolean
_status_tab_account_list_contains(gchar **accounts, const gchar *account)
{
	gint i = 0;

	while(accounts[i])
	{
		if(!g_strcmp0(accounts[i], account))
		{
			return TRUE;
		}

		++i;
	}

	return FALSE;
}

static gint
_status_tab_copy_accounts(_StatusTab *tab, const gchar *exclude, gchar ***usernames)
{
	gint length = 0;
	gint i = 0;

	g_mutex_lock(tab->accountlist.mutex);

	(*usernames) = (gchar **)g_malloc(sizeof(gchar *) * (g_strv_length(tab->accountlist.accounts) + 1));

	while(tab->accountlist.accounts[i])
	{
		if(!exclude || g_ascii_strcasecmp(exclude, tab->accountlist.accounts[i]))
		{
			(*usernames)[length] = g_strdup(tab->accountlist.accounts[i]);
			++length;
		}

		++i;
	}

	(*usernames)[length] = NULL;

	g_mutex_unlock(tab->accountlist.mutex);

	return length;
}

/*
 *	status replies:
 */
static void
_status_tab_replies_button_clicked(GtkTwitterStatus *status, const gchar *guid, _StatusTab *tab)
{
	GtkWidget *dialog;
	gchar *username = NULL;
	gchar *friend = NULL;
	TwitterDbHandle *handle;
	gint i = 0;
	gboolean failure = FALSE;
	GError *err = NULL;

	if(tab->owner)
	{
		username = g_strdup(username);
	}
	else
	{
		/* try to find account following author of the status */
		if((handle = twitterdb_get_handle(&err)))
		{
			g_object_get(G_OBJECT(status), "username", &friend, NULL);
			g_debug("Searching friend of user \"%s\"", friend);

			g_mutex_lock(tab->accountlist.mutex);

			while(!failure && !username && tab->accountlist.accounts[i])
			{
				g_debug("Testing friendship \"%s\" => \"%s\"", tab->accountlist.accounts[i], friend);
				if(twitterdb_is_follower(handle, tab->accountlist.accounts[i], friend, &err))
				{
					g_debug("\"%s\" is following \"%s\"", tab->accountlist.accounts[i], friend);
					username = g_strdup(tab->accountlist.accounts[i]);
				}
				else if(err)
				{
					failure = TRUE;
				}
			
				++i;
			}

			if(!username)
			{
				/* didn't find friend in database => use first account */
				username = g_strdup(tab->accountlist.accounts[0]);
			}

			g_mutex_unlock(tab->accountlist.mutex);

			twitterdb_close_handle(handle);
		}
		else
		{
			g_warning("Couldn't connect to database.");
		}
	}

	dialog = replies_dialog_create(tabbar_get_mainwindow(tab->tabbar), username, gtk_twitter_status_get_guid(status));
	replies_dialog_run(dialog);

	if(GTK_IS_WIDGET(dialog))
	{
		gtk_widget_destroy(dialog);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_free(friend);
	g_free(username);
}


/*
 *	reply:
 */
static gboolean
_status_tab_compose_tweet_callback(const gchar *username, const gchar *text, const gchar *prev_status, _StatusTab *tab)
{
	GtkWidget *mainwindow;
	TwitterClient *client;
	GtkWidget *dialog;
	GError *err = NULL;
	gboolean result = FALSE;

	mainwindow = tabbar_get_mainwindow(tab->tabbar);

	if((client = mainwindow_create_twittter_client(mainwindow, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
	{
		if((result = twitter_client_post(client, username, text, prev_status, &err)))
		{
			mainwindow_sync_gui(mainwindow);
		}
		else
		{
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}

			dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow),
		                                        GTK_DIALOG_MODAL,
		                                        GTK_MESSAGE_WARNING,
		                                        GTK_BUTTONS_OK,
		                                        _("Couldn't publish your tweet, please try again later."));
			g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, dialog);
		}

		g_object_unref(client);
	}

	return result;
}

static void
_status_tab_reply_button_clicked(GtkTwitterStatus *status, const gchar *guid, _StatusTab *tab)
{
	GtkWidget *dialog;
	gchar *username;
	gchar *text;
	gchar **usernames = NULL;
	gchar *author = NULL;
	gint length;

	if(tab->owner)
	{
		usernames = (gchar **)g_malloc(sizeof(gchar *) * 2);
		usernames[0] = g_strdup(tab->owner);
		usernames[1] = NULL;
		length = 1;
	}
	else
	{
		usernames = (gchar **)g_realloc(usernames, sizeof(gchar *) * (g_strv_length(tab->accountlist.accounts) + 1));
		g_object_get(G_OBJECT(status), "username", &author, NULL);
		length = _status_tab_copy_accounts(tab, author, &usernames);
	}

	/* create & run composer dialog */
	dialog = composer_dialog_create(tabbar_get_mainwindow(tab->tabbar), usernames, length, tab->owner, gtk_twitter_status_get_guid(status), _("Reply"));
	g_object_get(G_OBJECT(status), "username", &username, NULL);
	text = g_strdup_printf("@%s ", username);
	composer_dialog_set_apply_callback(dialog, (ComposerApplyCallback)_status_tab_compose_tweet_callback, tab);
	composer_dialog_set_text(dialog, text);
	gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));

	/* cleanup */
	g_free(author);
	g_strfreev(usernames);
	g_free(username);
	g_free(text);
	

	if(GTK_IS_WIDGET(dialog))
	{
		gtk_widget_destroy(dialog);
	}
}

/*
 *	retweet:
 */
static gpointer
_status_tab_retweet_worker(_RetweetArg *arg)
{
	GtkWidget *dialog;
	TwitterClient *client;
	GError *err = NULL;
	gint response = GTK_RESPONSE_OK;

	/* retweet status & close dialog */
	client = mainwindow_create_twittter_client(arg->mainwindow, TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME);

	if(!twitter_client_retweet(client, arg->account, arg->guid, &err))
	{
		response = GTK_RESPONSE_CANCEL;

		dialog = gtk_message_dialog_new(GTK_WINDOW(arg->mainwindow),
		                                GTK_DIALOG_MODAL,
		                                GTK_MESSAGE_WARNING,
		                                GTK_BUTTONS_OK,
		                                _("Couldn't retweet status, please try again later."));
		g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, dialog);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_object_unref(client);

	gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(arg->dialog), response);

	return NULL;
}

static void
_status_tab_retweet_multiple_account_ok_clicked(GtkWidget *button, _RetweetArg *arg)
{
	GThread *thread;
	GError *err = NULL;

	g_assert(arg != NULL);
	g_assert(GTK_IS_WIDGET(arg->dialog));
	g_assert(GTK_IS_WINDOW(arg->mainwindow));

	/* disable dialog & start thread to retweet the status */
	gtk_helpers_set_widget_busy(arg->dialog, TRUE);
	arg->account = select_account_dialog_get_account(arg->dialog);

	g_debug("Retweeting status \"%s\" using account \"%s\"", arg->guid, arg->account);
	thread = g_thread_create((GThreadFunc)_status_tab_retweet_worker, arg, FALSE, &err);

	if(!thread)
	{
		if(err)
		{
			g_error("%s", err->message);
			g_error_free(err);
		}
		else
		{
			g_error("Couldn't create worker.");
		}
	}	
}

static void
_status_tab_retweet_multiple_account(GtkWidget *mainwindow, gchar **accounts, gint length, const gchar *guid)
{
	GtkWidget *dialog;
	GtkWidget *button;
	_RetweetArg *arg = (_RetweetArg *)g_slice_alloc(sizeof(_RetweetArg));

	/* let user select an account to retweet the status */
	dialog = select_account_dialog_create(mainwindow, accounts, length, _("Retweet"), _("Please specify an user account to retweet the selected status:"));

	arg->dialog = dialog;
	arg->mainwindow = mainwindow;
	g_strlcpy(arg->guid, guid, 32);
	arg->account = NULL;

	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_start(GTK_BOX(select_account_dialog_get_action_area(dialog)), button, FALSE, FALSE, 0);
	gtk_widget_set_visible(button, TRUE);
	g_signal_connect(button, "clicked", (GCallback)_status_tab_retweet_multiple_account_ok_clicked, arg);
	select_account_dialog_add_button(dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	/* run dialog */
	if(select_account_dialog_run(dialog) == GTK_RESPONSE_OK)
	{
		mainwindow_sync_gui(mainwindow);
	}

	/* cleanup */
	if(GTK_IS_WIDGET(dialog))
	{
		gtk_widget_destroy(dialog);
	}

	g_free(arg->account);
	g_slice_free1(sizeof(_RetweetArg), arg);
}

static void
_status_tab_retweet_single_account(GtkWidget *mainwindow, const gchar *account, const gchar *guid)
{
	GtkWidget *dialog;
	gint response;
	GtkWidget *message_dialog;

	dialog = retweet_dialog_create(mainwindow, account, guid);

	if((response = gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog))) == GTK_RESPONSE_YES)
	{
		mainwindow_sync_gui(mainwindow);
	}
	else if(response == GTK_RESPONSE_CANCEL)
	{
		message_dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow),
		                                        GTK_DIALOG_MODAL,
		                                        GTK_MESSAGE_WARNING,
		                                        GTK_BUTTONS_OK,
		                                        _("Couldn't retweet status, please try again later."));
		g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, message_dialog);
	}

	if(GTK_IS_DELETABLE_DIALOG(dialog))
	{
		gtk_widget_destroy(dialog);
	}
}

static void
_status_tab_retweet_button_clicked(GtkTwitterStatus *status, const gchar *guid, _StatusTab *tab)
{
	GtkWidget *mainwindow;
	gchar **accounts = NULL;
	gchar *author = NULL;
	gint length = 0;

	g_debug("Retweeting status \"%s\"", guid);
	mainwindow = tabbar_get_mainwindow(tab->tabbar);

	/* ask user if he wants to retweet the status if tab has an assigned owner */
	if(tab->owner)
	{
		_status_tab_retweet_single_account(mainwindow, tab->owner, guid);
	}
	else
	{
		g_debug("Tab has no owner, detecting account to retweet status");

		/* copy account list */
		g_object_get(G_OBJECT(status), "username", &author, NULL);
		length = _status_tab_copy_accounts(tab, author, &accounts);
		g_free(author);

		/* let user select an account if account list contains more than one account */
		if(length == 1)
		{
			g_debug("Account list contains only a single user");
			_status_tab_retweet_single_account(mainwindow, tab->owner, guid);
		}
		else
		{
			g_debug("Account list contains multiple users");
			_status_tab_retweet_multiple_account(mainwindow, accounts, length, guid);
		}
	}
	
	g_strfreev(accounts);
}

/*
 *	friendship dialog:
 */
static gboolean
_status_tab_update_friendship(GtkWidget *dialog, gboolean checked, TwitterDbHandle *handle, TwitterClient *client, const gchar *username, gint *count)
{
	gchar *account;
	GList *users;
	GList *iter;
	GError *err = NULL;
	gboolean result = TRUE;

	gdk_threads_enter();
	iter = users = gtk_user_list_dialog_get_users(GTK_USER_LIST_DIALOG(dialog), checked);
	gdk_threads_leave();

	if(iter)
	{
		while(iter && result)
		{
			account = (gchar *)iter->data;
			g_debug("Testing friendship: \"%s\" %s> \"%s\"", account, checked ? "=" : "!=", username);

			if(twitterdb_is_follower(handle, account, username, &err) != checked && !err)
			{
				if(checked)
				{
					g_debug("Adding friendship: \"%s\" => \"%s\"", account, username);
					result = twitter_client_add_friend(client, account, username, &err);
				}
				else
				{
					g_debug("Removing friendship: \"%s\" => \"%s\"", account, username);
					result = twitter_client_remove_friend(client, account, username, &err);
				}

				if(result)
				{
					++(*count);
				}
			}

			if(err)
			{
				result = FALSE;
				g_warning("%s", err->message);
				g_error_free(err);
				err = NULL;
			}

			iter = iter->next;
		}
	}

	/* cleanup */
	g_list_foreach(users, (GFunc)g_free, NULL);
	g_list_free(users);

	return result;
}

static gpointer
_status_tab_friendship_apply_worker(_StatusTabFriendshipWorkerArg *arg)
{
	TwitterDbHandle *handle;
	TwitterClient *client;
	GError *err = NULL;
	GtkWidget *message_dialog;
	GtkWidget *parent;
	gint count = 0;
	gint response = GTK_RESPONSE_NONE;
	gboolean success = FALSE;

	g_debug("Starting worker: %s", __func__);

	/* get parent window */
	parent = tabbar_get_mainwindow(arg->tab->tabbar);

	/* update friendship information */
	if((handle = twitterdb_get_handle(&err)))
	{
		if((client = mainwindow_create_twittter_client(tabbar_get_mainwindow(arg->tab->tabbar), TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
		{
			/* update friendship information */
			if((success = _status_tab_update_friendship(arg->dialog, FALSE, handle, client, arg->username, &count)))
			{
				success = _status_tab_update_friendship(arg->dialog, TRUE, handle, client, arg->username, &count);
			}

			/* close dialog */
			gdk_threads_enter();
			if(count)
			{
				response = success ? GTK_RESPONSE_APPLY : GTK_RESPONSE_CANCEL;
			}

			gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(arg->dialog), response);
			gdk_threads_leave();

			/* destroy client */
			g_object_unref(client);
		}

		if(!success)
		{
			/* show failure message */
			message_dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
								GTK_DIALOG_MODAL,
								GTK_MESSAGE_WARNING,
								GTK_BUTTONS_OK,
								_("Couldn't update friendship information, please try again later."));
			g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, message_dialog);
		}

		/* destroy database connection */
		twitterdb_close_handle(handle);
	}
	else
	{
		g_warning("Couldn't create TwitterClient instance.");
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
		err = NULL;
	}

	g_debug("Leaving worker: %s", __func__);

	return NULL;
}

static void
_status_tab_friendship_apply(GtkWidget *button, _StatusTabFriendshipWorkerArg *arg)
{
	GThread *thread;
	GError *err = NULL;

	/* disable window & start apply worker*/
	gtk_helpers_set_widget_busy(arg->dialog, TRUE);

	if(!(thread = g_thread_create((GThreadFunc)_status_tab_friendship_apply_worker, arg, FALSE, &err)))
	{
		if(err)
		{
			g_warning("%s\n", err->message);
			g_error_free(err);
		}
	}
}

static gboolean
_status_tab_friendship_worker(_StatusTabFriendshipWorkerArg *arg)
{
	GtkWidget *dialog;
	GtkWidget *parent;
	GtkWidget *button;
	TwitterDbHandle *handle;
	gint i = 0;
	TwitterUser user;
	GError *err = NULL;

	gdk_threads_enter();

	/* create dialog */
	parent = tabbar_get_mainwindow(arg->tab->tabbar);
	dialog = edit_members_dialog_create(parent, _("Edit friendship"));
	g_object_set(G_OBJECT(dialog),
	             "checkbox-column-visible",
	             TRUE,
	             "checkbox-column-activatable",
	             TRUE,
	             "checkbox-column-title",
	             _("Followed by"),
	             "username-column-title",
	             _("Username"),
	             "has-separator",
	             FALSE,
	             "show-user_count",
	             FALSE,
	             NULL);
	gtk_widget_set_size_request(dialog, 350, 250);
	arg->dialog = dialog;

	/* add buttons */
	button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_box_pack_start(GTK_BOX(gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog))), button, FALSE, FALSE, 0);
	gtk_widget_set_visible(button, TRUE);
	g_signal_connect(button, "clicked", (GCallback)_status_tab_friendship_apply, arg);
	gtk_deletable_dialog_add_button(GTK_DELETABLE_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gdk_threads_leave();

	/* populate list */
	if((handle = twitterdb_get_handle(&err)))
	{
		g_mutex_lock(arg->tab->accountlist.mutex);

		while(arg->tab->accountlist.accounts[i])
		{
			if(twitterdb_get_user_by_name(handle, arg->tab->accountlist.accounts[i], &user, &err))
			{
				/* don't append user to list if username equals account name */
				if(g_ascii_strcasecmp(user.screen_name, arg->username))
				{
					edit_members_dialog_add_user(dialog, user, twitterdb_is_follower(handle, arg->tab->accountlist.accounts[i], arg->username, &err));
				}

				if(err)
				{
					g_warning("%s", err->message);
					g_error_free(err);
					err = NULL;
				}
			}
			else
			{
				g_warning("Couldn't get user details of user \"%s\"", arg->tab->accountlist.accounts[i]);

				if(err)
				{
					g_warning("%s", err->message);
					g_error_free(err);
					err = NULL;
				}
			}

			++i;
		}

		g_mutex_unlock(arg->tab->accountlist.mutex);

		twitterdb_close_handle(handle);
	}

	/* display error message */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	/* run dialog */
	gdk_threads_enter();

	if(gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog)) == GTK_RESPONSE_APPLY)
	{
		mainwindow_sync_timelines(parent);
	}

	edit_members_dialog_destroy(dialog);

	gdk_threads_leave();

	/* free memory */
	g_slice_free1(sizeof(_StatusTabFriendshipWorkerArg), arg);

	return FALSE;
}

static void
_status_tab_edit_friendship_button_clicked(GtkTwitterStatus *status, const gchar *username, _StatusTab *tab)
{
	_StatusTabFriendshipWorkerArg *arg = (_StatusTabFriendshipWorkerArg *)g_slice_alloc(sizeof(_StatusTabFriendshipWorkerArg));

	arg->tab = tab;
	arg->dialog = NULL;
	g_strlcpy(arg->username, username, 64);

	g_idle_add((GSourceFunc)_status_tab_friendship_worker, arg);
}

/*
 *	friendship tooltip:
 */
static gboolean
_status_tab_edit_friendship_button_query_tooltip(GtkTwitterStatus *status, GtkTooltip *tooltip, _StatusTab *tab)
{
	TwitterDbHandle *handle;
	GError *err = NULL;
	gchar *username;
	gint i = 0;
	GString *text = NULL;
	gint followers = 0;
	gboolean first = TRUE;
	static gchar last_toolip[512] = { 0 };
	static GQuark last_user = 0;
	static glong last_query = 0;
	glong diff = -1;
	gboolean result = FALSE;

	g_object_get(G_OBJECT(status), "username", &username, NULL);
	diff = _status_tab_get_tooltip_diff(username, &last_user, &last_query);

	/* display tooltip */
	if(diff < 0 || diff > 2)
	{
		memset(last_toolip, 0, 512);

		if((handle = twitterdb_get_handle(&err)))
		{
			text = g_string_sized_new(128);

			g_mutex_lock(tab->accountlist.mutex);

			if(tab->accountlist.accounts)
			{
				g_string_printf(text, "<b>Followed by:</b> ");

				while(tab->accountlist.accounts[i])
				{
					if(twitterdb_is_follower(handle, tab->accountlist.accounts[i], username, &err))
					{
						if(first)
						{
							first = FALSE;
						}
						else
						{
							text = g_string_append(text, ", ");
						}

						g_string_append_printf(text, "<i>%s</i>", tab->accountlist.accounts[i]);
						++followers;
					}
					else if(err)
					{
						g_warning("%s", err->message);
						g_error_free(err);
						err = NULL;
					}

					++i;
				}
			}

			g_mutex_unlock(tab->accountlist.mutex);

			twitterdb_close_handle(handle);
		}

		if(followers > 0)
		{
			gtk_tooltip_set_markup(tooltip, text->str);
			g_strlcpy(last_toolip, text->str, 512);
			result = TRUE;
		}
	}
	else if(last_toolip[0])
	{
		gtk_tooltip_set_markup(tooltip, last_toolip);
		result = TRUE;
	}

	/* cleanup */
	g_free(username);

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	if(text)
	{
		g_string_free(text, TRUE);
	}

	return result;
}


/*
 *	list membership tooltip:
 */
static gchar *
_status_tab_build_tooltip_list_name(const gchar *account, const gchar *listname)
{
	gchar *escaped_account = NULL;
	gchar *escaped_listname = NULL;
	gchar *result = NULL;

	if((escaped_account = g_markup_escape_text(account, -1)))
	{
		if((escaped_listname = g_markup_escape_text(listname, -1)))
		{
			result = g_strdup_printf("<i>%s@</i><b>%s</b>", escaped_account, escaped_listname);
		}
	}

	g_free(escaped_account);
	g_free(escaped_listname);

	return result;
}

static gboolean
_status_tab_edit_lists_button_query_tooltip(GtkTwitterStatus *status, GtkTooltip *tooltip, _StatusTab *tab)
{
	TwitterDbHandle *handle;
	GError *err = NULL;
	gint count;
	gchar *username;
	gchar **accounts = NULL;
	gchar **lists = NULL;
	GString *text = NULL;
	gchar *item;
	gboolean first = TRUE;
	static gchar last_toolip[512] = { 0 };
	static GQuark last_user = 0;
	static glong last_query = 0;
	glong diff = -1;
	gboolean result = FALSE;

	g_object_get(G_OBJECT(status), "username", &username, NULL);
	diff = _status_tab_get_tooltip_diff(username, &last_user, &last_query);

	/* display tooltip */
	if(diff < 0 || diff > 2)
	{
		memset(last_toolip, 0, 512);

		if((handle = twitterdb_get_handle(&err)))
		{
			if((count = twitterdb_get_list_membership(handle, username, &accounts, &lists, &err)) > 0)
			{
				text = g_string_sized_new(128);
				result = TRUE;

				if(count)
				{
					for(gint i = 0; i < count; ++i)
					{
						if(first)
						{
							first = FALSE;
						}
						else
						{
							text = g_string_append(text, "\n");
						}

						item = _status_tab_build_tooltip_list_name(accounts[i], lists[i]);
						text = g_string_append(text, item);
						g_free(item);

						gtk_tooltip_set_markup(tooltip, text->str);
						memcpy(last_toolip, text->str, 512);

						g_free(accounts[i]);
						g_free(lists[i]);
					}
				}

				g_free(accounts);
				g_free(lists);
			}

			twitterdb_close_handle(handle);
		}
	}
	else if(last_toolip[0])
	{
		gtk_tooltip_set_markup(tooltip, last_toolip);
		result = TRUE;
	}

	/* cleanup */
	g_free(username);

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	if(text)
	{
		g_string_free(text, TRUE);
	}

	return result;
}

/*
 *	edit list membership:
 */
static void
_status_tab_edit_lists_button_clicked(GtkTwitterStatus *status, const gchar *username, _StatusTab *tab)
{
	GtkWidget *dialog;
	GtkWidget *parent;
	gint response;

	parent = tabbar_get_mainwindow(tab->tabbar);

	g_debug("Editing list membership: \"%s\"", username);

	dialog = edit_list_membership_dialog_create(parent, username);
	response = gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if(response == GTK_RESPONSE_APPLY)
	{
		mainwindow_sync_lists(tabbar_get_mainwindow(tab->tabbar));
	}
}

/*
 *	edit list members:
 */
static void
_status_tab_add_user_to_dialog(TwitterUser user, GtkWidget *dialog)
{
	gdk_threads_enter();
	edit_members_dialog_add_user(dialog, user, TRUE);
	gdk_threads_leave();
}

static gpointer
_status_tab_edit_list_members_apply_worker(_StatusTabListWorkerArg *arg)
{
	GList *users;
	GList *iter;
	gchar *username;
	TwitterClient *client;
	GError *err = NULL;
	GtkWidget *message_dialog;
	GtkWidget *parent;
	gboolean success = FALSE;

	g_debug("Starting worker: %s", __func__);

	/* get parent window */
	parent = tabbar_get_mainwindow(arg->tab->tabbar);

	/* try to update membership */
	if((client = mainwindow_create_twittter_client(tabbar_get_mainwindow(arg->tab->tabbar), TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME)))
	{
		gdk_threads_enter();
		users = gtk_user_list_dialog_get_users(GTK_USER_LIST_DIALOG(arg->dialog), FALSE);
		gdk_threads_leave();

		iter = users;
		success = TRUE;

		while(iter && success)
		{
			username = (gchar *)iter->data;
			g_debug("Removing user \"%s\" from list \"%s@%s\"", username, arg->username, arg->listname);

			if(!(success = twitter_client_remove_user_from_list(client, arg->username, arg->listname, username, &err)))
			{
				if(err)
				{
					g_warning("%s", err->message);
					g_error_free(err);
					err = NULL;
				}
			}

			iter = iter->next;
		}

		g_list_foreach(users, (GFunc)g_free, NULL);
		g_list_free(users);

		/* close dialog */
		gdk_threads_enter();
		gtk_deletable_dialog_response(GTK_DELETABLE_DIALOG(arg->dialog), GTK_RESPONSE_APPLY);
		gdk_threads_leave();

		/* free memory */
		g_object_unref(client);
	}

	if(!success)
	{
		/* show failure message */
		message_dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
							GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							GTK_BUTTONS_OK,
							_("Couldn't update list members, please try again later."));
		g_idle_add((GSourceFunc)gtk_helpers_run_and_destroy_dialog_worker, message_dialog);
	}


	g_debug("Leaving worker: %s", __func__);

	return NULL;
}

static void
_status_tab_edit_list_members_apply(GtkWidget *dialog, _StatusTabListWorkerArg *arg)
{
	GThread *thread;
	GError *err = NULL;

	/* disable window & start appy worker */
	gtk_helpers_set_widget_busy(arg->dialog, TRUE);

	if(!(thread = g_thread_create((GThreadFunc)_status_tab_edit_list_members_apply_worker, arg, FALSE, &err)))
	{
		if(err)
		{
			g_warning("%s\n", err->message);
			g_error_free(err);
		}
	}
}

static gboolean
_status_tab_edit_list_members_worker(_StatusTabListWorkerArg *arg)
{
	GtkWidget *dialog;
	GtkWidget *parent;
	TwitterDbHandle *handle;
	GtkWidget *button;
	GError *err = NULL;

	gdk_threads_enter();

	/* create dialog */
	parent = tabbar_get_mainwindow(arg->tab->tabbar);
	dialog = edit_members_dialog_create(parent, _("List members"));
	g_object_set(G_OBJECT(dialog),
	             "checkbox-column-visible",
	             TRUE,
	             "checkbox-column-activatable",
	             TRUE,
	             "checkbox-column-title",
	             _("Member"),
	             "username-column-title",
	             _("Username"),
	             "has-separator",
	             FALSE,
	             NULL);
	arg->dialog = dialog;

	/* add buttons */
	button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_box_pack_start(GTK_BOX(gtk_deletable_dialog_get_action_area(GTK_DELETABLE_DIALOG(dialog))), button, FALSE, FALSE, 0);
	gtk_widget_set_visible(button, TRUE);
	g_signal_connect(button, "clicked", (GCallback)_status_tab_edit_list_members_apply, arg);
	gtk_deletable_dialog_add_button(GTK_DELETABLE_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	gdk_threads_leave();

	/* populate list */
	if((handle = twitterdb_get_handle(&err)))
	{
		twitterdb_foreach_list_member(handle, arg->username, arg->listname, (TwitterProcessListMemberFunc)_status_tab_add_user_to_dialog, (gpointer)dialog, NULL, &err);
		twitterdb_close_handle(handle);
	}	

	/* display error message */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	/* run dialog */
	gdk_threads_enter();

	if(gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog)) == GTK_RESPONSE_APPLY)
	{
		mainwindow_sync_lists(tabbar_get_mainwindow(arg->tab->tabbar));
	}

	edit_members_dialog_destroy(dialog);

	gdk_threads_leave();

	/* free memory */
	g_slice_free1(sizeof(_StatusTabListWorkerArg), arg);

	return FALSE;
}

static void
_status_tab_edit_list_members_button_clicked(GtkWidget *button, _StatusTab *tab)
{
	gchar **pieces;
	gchar *tab_id;
	_StatusTabListWorkerArg *arg;

	tab_id = tab_get_id((Tab *)tab);

	if((pieces = g_strsplit(tab_id, "@", 2)))
	{
		g_debug("List name: \"%s\"@\"%s\"", pieces[0], pieces[1]);

		arg = (_StatusTabListWorkerArg *)g_slice_alloc(sizeof(_StatusTabListWorkerArg));
		arg->tab = tab;
		arg->dialog = NULL;
		g_strlcpy(arg->username, pieces[0], 64);
		g_strlcpy(arg->listname, pieces[1], 64);
		g_strfreev(pieces);

		g_idle_add((GSourceFunc)_status_tab_edit_list_members_worker, arg);
	}

	g_free(tab_id);
}

/*
 *	list preferences:
 */
static void
_status_tab_list_preferences_button_clicked(GtkWidget *widget, _StatusTab *tab)
{
	gchar **pieces;
	GtkWidget *dialog;
	GtkWidget *parent;
	gchar *tab_id;

	parent = tabbar_get_mainwindow(tab->tabbar);
	tab_id = tab_get_id((Tab *)tab);

	if((pieces = g_strsplit(tab_id, "@", 2)))
	{
		dialog = list_preferences_dialog_create(parent, _("List preferences"), pieces[0], pieces[1]);
		gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_strfreev(pieces);
	}

	g_free(tab_id);
}

/*
 *	remove list:
 */
static void
_status_tab_remove_list_button_clicked(GtkWidget *widget, _StatusTab *tab)
{
	GtkWidget *dialog;
	gchar *tab_id;
	gchar **pieces;

	tab_id = tab_get_id((Tab *)tab);

	if((pieces = g_strsplit(tab_id, "@", 2)))
	{
		dialog = remove_list_dialog_create(tabbar_get_mainwindow(tab->tabbar), pieces[0], pieces[1]);
		gtk_deletable_dialog_run(GTK_DELETABLE_DIALOG(dialog));
		g_strfreev(pieces);

		if(GTK_IS_DELETABLE_DIALOG(dialog))
		{
			gtk_widget_destroy(dialog);
		}
	}

	g_free(tab_id);
}

/*
 *	scrolling:
 */
static gboolean
_status_tab_key_press(GtkWidget *widget, GdkEventKey *event, _StatusTab *tab)
{
	TabFuncs *funcs;
	gboolean result = TRUE;

	funcs = ((Tab *)tab)->funcs;

	if(event->keyval == GDK_Down)
	{
		funcs->scroll(widget, TRUE);
	}
	else if(event->keyval == GDK_Up)
	{
		funcs->scroll(widget, FALSE);
	}
	else
	{
		result = FALSE;
	}

	return result;
}

static void
_status_tab_grab_focus(GtkTwitterStatus *status, _StatusTab *tab)
{
	GtkAllocation alloc;
	gint height;
	GtkWidget *bar;
	gint bar_pos;

	gtk_widget_get_allocation(tab->page, &alloc);
	height = alloc.height;

	gtk_widget_get_allocation(GTK_WIDGET(status), &alloc);
	height -= alloc.height;

	bar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(tab->scrolled));
	bar_pos = gtk_range_get_value(GTK_RANGE(bar));

	if(alloc.y > bar_pos + height)
	{
		gtk_range_set_value(GTK_RANGE(bar), alloc.y);
	}
	else if(alloc.y < bar_pos)
	{
		gtk_range_set_value(GTK_RANGE(bar), alloc.y);
	}
}

/*
 *	add tweets:
 */
static void
_status_tab_add_tweet(TwitterStatus status, TwitterUser user, _StatusTab *tab)
{
	GtkWidget *widget;
	GList *children;
	GList *iter;
	gint position = 0;
	gboolean exists = FALSE;
	gchar group[128];
	gboolean owner = FALSE;
	TabTypeId type_id;
	gboolean show_extra_buttons = TRUE;

	gdk_threads_enter();

	/* convert timestamp (if necessary) */
	if(!status.timestamp && status.created_at[0])
	{
		status.timestamp = (gint)twitter_timestamp_to_unix_timestamp(status.created_at);
	}
		
	/* check if status does exist */
	if((iter = children = gtk_container_get_children(GTK_CONTAINER(tab->vbox))))
	{
		while(iter)
		{

			if(!strcmp(status.id, gtk_twitter_status_get_guid(GTK_TWITTER_STATUS(iter->data))))
			{
				exists = TRUE;
				break;
			}

			iter = iter->next;
		}
	}

	if(!exists)
	{
		/* find position */
		if((iter = children))
		{
			while(iter)
			{
				if(status.timestamp > gtk_twitter_status_get_timestamp(GTK_TWITTER_STATUS(iter->data)))
				{
					break;
				}

				++position;
				iter = iter->next;
			}
		}

		/* insert status */
		if(!exists)
		{
			/* create widget */
			widget = gtk_twitter_status_new();

			if(tab->owner && !g_strcasecmp(user.screen_name, tab->owner))
			{
				g_mutex_lock(tab->accountlist.mutex);

				if((owner = _status_tab_account_list_contains(tab->accountlist.accounts, user.screen_name)))
				{
					show_extra_buttons = FALSE;

					if((type_id = tabbar_get_current_type(tab->tabbar)) == TAB_TYPE_ID_SEARCH || type_id == TAB_TYPE_ID_USER_TIMELINE)
					{
						if(g_strv_length(tab->accountlist.accounts) > 1)
						{
							show_extra_buttons = TRUE;
						}
					}
				}
	
				g_mutex_unlock(tab->accountlist.mutex);
			}

			g_object_set(G_OBJECT(widget),
			             "guid", status.id,
			             "username", user.screen_name,
			             "realname", user.name,
			             "timestamp", status.timestamp,
			             "status", status.text,
			             "show-reply-button", show_extra_buttons,
				     "show-edit-lists-button", TRUE,
			             "edit-lists-button-has-tooltip", TRUE,
			             "show-edit-friendship-button", show_extra_buttons,
			             "edit-friendship-button-has-tooltip", TRUE,
				     "show-retweet_button", show_extra_buttons,
				     "show-delete-button", FALSE,
				     "show-replies-button", status.prev_status[0] ? TRUE : FALSE,
				     "selectable", TRUE,
				     "background-color", tab->background_color,
				     NULL);

			/* insert status */
			gtk_box_pack_start(GTK_BOX(tab->vbox), widget, FALSE, FALSE, 2);
			gtk_box_reorder_child(GTK_BOX(tab->vbox), widget, position);
			gtk_widget_show_all(widget);

			/* register signals */
			g_signal_connect(G_OBJECT(widget), "url-activated", (GCallback)_status_tab_url_activated, tab);
			g_signal_connect(G_OBJECT(widget), "username-activated", (GCallback)_status_tab_username_activated, tab);
			g_signal_connect(G_OBJECT(widget), "edit-lists-button-query-tooltip", (GCallback)_status_tab_edit_lists_button_query_tooltip, tab);
			g_signal_connect(G_OBJECT(widget), "edit-lists-button-clicked", (GCallback)_status_tab_edit_lists_button_clicked, tab);
			g_signal_connect(G_OBJECT(widget), "edit-friendship-button-clicked", (GCallback)_status_tab_edit_friendship_button_clicked, tab);
			g_signal_connect(G_OBJECT(widget), "edit-friendship-button-query-tooltip", (GCallback)_status_tab_edit_friendship_button_query_tooltip, tab);
			g_signal_connect(G_OBJECT(widget), "reply-button-clicked", (GCallback)_status_tab_reply_button_clicked, tab);
			g_signal_connect(G_OBJECT(widget), "retweet-button-clicked", (GCallback)_status_tab_retweet_button_clicked, tab);
			g_signal_connect(G_OBJECT(widget), "replies-button-clicked", (GCallback)_status_tab_replies_button_clicked, tab);
			g_signal_connect(G_OBJECT(widget), "grab-focus", (GCallback)_status_tab_grab_focus, tab);

			/* load pixmap */
			sprintf(group, "statustab-%d", tab->tab_id);
			mainwindow_load_pixbuf(tabbar_get_mainwindow(tab->tabbar), group, user.image, (PixbufLoaderCallback)pixbuf_helpers_set_gtktwitterstatus_callback, widget, NULL);
		}
	}

	gdk_flush();
	gdk_threads_leave();

	if(children)
	{
		g_list_free(children);
	}

	if(!exists)
	{
		g_usleep(50000);
	}
}

static void
_status_tab_populate_list(TwitterClient *client, _StatusTab *tab, GError **err)
{
	gchar *tab_id;
	gchar **pieces;

	tab_id = tab_get_id((Tab *)tab);

	if((pieces = g_strsplit(tab_id, "@", 2)))
	{
		twitter_client_process_list(client, pieces[0], pieces[1], (TwitterProcessStatusFunc)_status_tab_add_tweet, tab, tab->cancellable, err);
		g_strfreev(pieces);
	}

	g_free(tab_id);
}

static void
_status_tab_populate_search_query(TwitterClient *client, _StatusTab *tab, GError **err)
{
	gchar *tab_id;
	gchar *account;

	g_mutex_lock(tab->accountlist.mutex);
	account = g_strdup(tab->accountlist.accounts[0]);
	g_mutex_unlock(tab->accountlist.mutex);

	tab_id = tab_get_id((Tab *)tab);
	twitter_client_search(client, account, tab_id, (TwitterProcessStatusFunc)_status_tab_add_tweet, tab, tab->cancellable, err);
	g_free(tab_id);
}

static void
_status_tab_update_color(_StatusTab *tab)
{
	GList *iter;
	GList *children;
	GtkWidget *widget;

	if((iter = children = gtk_container_get_children(GTK_CONTAINER(tab->vbox))))
	{
		while(iter)
		{
			widget = GTK_WIDGET(iter->data);

			gdk_threads_enter();
			g_object_set(G_OBJECT(widget), "background-color", tab->background_color, NULL);
			gdk_threads_leave();

			g_usleep(75000);
			iter = iter->next;
		}
	}

	if(children)
	{
		g_list_free(children);
	}
}

static void
_status_tab_populate(_StatusTab *tab)
{
	gchar *tab_id;
	TabTypeId type;
	TwitterClient *client;
	GError *err = NULL;

	/* get id & type from tab */
	tab_id = tab_get_id((Tab *)tab);
	type = ((Tab *)tab)->type_id;

	/* create TwitterClient */
	if(type == TAB_TYPE_ID_SEARCH)
	{
		client = mainwindow_create_twittter_client(tabbar_get_mainwindow(tab->tabbar), TWITTER_CLIENT_DEFAULT_CACHE_LIFETIME);
	}
	else
	{
		client = mainwindow_create_twittter_client(tabbar_get_mainwindow(tab->tabbar), TWITTER_CLIENT_SEARCH_CACHE_LIFETIME);
	}

	/* insert tweets */
	switch(type)
	{
		case TAB_TYPE_ID_PUBLIC_TIMELINE:
			twitter_client_process_public_timeline(client, tab_id, (TwitterProcessStatusFunc)_status_tab_add_tweet, tab, tab->cancellable, &err);
			break;
	
		case TAB_TYPE_ID_REPLIES:
			twitter_client_process_replies(client, tab_id, (TwitterProcessStatusFunc)_status_tab_add_tweet, tab, tab->cancellable, &err);
			break;

		case TAB_TYPE_ID_USER_TIMELINE:
			twitter_client_process_usertimeline(client, tab_id, (TwitterProcessStatusFunc)_status_tab_add_tweet, tab, tab->cancellable, &err);
			break;

		case TAB_TYPE_ID_LIST:
			_status_tab_populate_list(client, tab, &err);
			break;

		case TAB_TYPE_ID_SEARCH:
			_status_tab_populate_search_query(client, tab, &err);
			break;

		default:
			g_warning("Unknown tab id: %d", type);
	}

	/* change background color of old tweets if necessary*/
	if(tab->background_changed)
	{
		tab->background_changed = FALSE;
		_status_tab_update_color(tab);
	}

	/* cleanup */
	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_free(tab_id);
	g_object_unref(client);
}

/*
 *	background worker (each tab has its own thread which updates the timeline):
 */
static gboolean
_status_tab_is_waiting(GtkWidget *widget)
{
	_StatusTab *meta = g_object_get_data(G_OBJECT(widget), "meta");
	gboolean waiting;

	g_mutex_lock(meta->status.mutex);
	waiting = meta->status.waiting;
	g_mutex_unlock(meta->status.mutex);

	return waiting;
}

static void
_status_tab_wait_for_worker(GtkWidget *widget)
{
	while(!_status_tab_is_waiting(widget))
	{
		g_usleep(100);
	}
}

static void
_status_tab_send_signal(GtkWidget *widget, gint signal)
{
	_StatusTab *meta;

	/* check if thread is waiting for signal */
	if(!_status_tab_is_waiting(widget))
	{
		return;
	}

	/* thread is waiting => set signal code & signal condition */
	meta = g_object_get_data(G_OBJECT(widget), "meta");

	g_static_mutex_lock(&meta->worker.mutex);
	meta->worker.signal = signal;
	g_cond_signal(meta->worker.condition);
	g_static_mutex_unlock(&meta->worker.mutex);
}

static gpointer
_status_tab_worker(gpointer data)
{
	_StatusTab *meta = g_object_get_data(G_OBJECT(data), "meta");
	gint signal;
	GTimeVal time;
	gboolean running = TRUE;

	g_get_current_time(&time);
	g_time_val_add(&time, 100);

	while(running)
	{
		/* set status to "waiting" */
		g_mutex_lock(meta->status.mutex);
		meta->status.waiting = TRUE;
		g_mutex_unlock(meta->status.mutex);
	
		/* sleep for a short while & read signal */
		g_cond_timed_wait(meta->worker.condition, g_static_mutex_get_mutex(&meta->worker.mutex), &time);
		signal = meta->worker.signal;
		meta->worker.signal = STATUS_TAB_SIGNAL_IDLE;

		/* update status */
		g_mutex_lock(meta->status.mutex);
		meta->status.waiting = FALSE;
		g_mutex_unlock(meta->status.mutex);

		/* unlock condition mutex */
		g_static_mutex_unlock(&meta->worker.mutex);
	
		/* signal handler */
		if(signal == STATUS_TAB_SIGNAL_REFRESH)
		{
			/* set busy status */
			gdk_threads_enter();
			tabbar_set_page_busy(meta->tabbar, ((Tab *)meta)->widget, TRUE);
			gtk_widget_set_sensitive(meta->vbox, FALSE);
			gdk_threads_leave();

			/* populate data */
			_status_tab_populate(meta);

			/* unset busy status */
			gdk_threads_enter();
			gtk_widget_set_sensitive(meta->vbox, TRUE);
			tabbar_set_page_busy(meta->tabbar, ((Tab *)meta)->widget, FALSE);
			gdk_threads_leave();
		}
		else if(signal == STATUS_TAB_SIGNAL_DESTROY)
		{
			g_debug("%s: tab received kill signal", __func__);
			running = FALSE;
		}

		g_get_current_time(&time);
		g_time_val_add(&time, 5000000);
	}

	return NULL;
}

/*
 *	action area:
 */
static void
_status_tab_show_action_area(GtkWidget *widget, gboolean visible)
{
	_StatusTab *meta = g_object_get_data(G_OBJECT(widget), "meta");
	GtkWidget *parent;

	parent = gtk_widget_get_parent(meta->action_area);

	if(visible)
	{
		gtk_widget_show_all(parent);
	}
	else
	{
		gtk_widget_hide(parent);
	}
}

/*
 *	initialization:
 */
static GtkWidget *
_status_tab_create_action_button(const gchar *icon, gboolean from_stock, const gchar *text)
{
	GtkWidget *hbox;
	gchar *filename;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *button;

	hbox = gtk_hbox_new(FALSE, 5);

	if(from_stock)
	{
		image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_SMALL_TOOLBAR);
	}
	else
	{
		filename = pathbuilder_build_icon_path("16x16", icon);
		image = gtk_image_new_from_file(filename);
		g_free(filename);
	}

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE, 0);

	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), hbox);

	gtk_widget_show_all(button);

	return button;
}

static void
_status_tab_init(GtkWidget *widget)
{
	_StatusTab *tab = g_object_get_data(G_OBJECT(widget), "meta");
	TabTypeId type;
	gboolean visible = TRUE;
	GtkWidget *button;

	type = ((Tab *)tab)->type_id;

	/* insert action widgets */
	switch(type)
	{
		case TAB_TYPE_ID_LIST:
			button = _status_tab_create_action_button("users.png", FALSE, _("List members"));
			g_signal_connect(G_OBJECT(button), "clicked", (GCallback)_status_tab_edit_list_members_button_clicked, tab);
			gtk_box_pack_start(GTK_BOX(tab->action_area), button, FALSE, FALSE, 2);

			button = _status_tab_create_action_button(GTK_STOCK_EDIT, TRUE, _("List preferences"));
			g_signal_connect(G_OBJECT(button), "clicked", (GCallback)_status_tab_list_preferences_button_clicked, tab);
			gtk_box_pack_start(GTK_BOX(tab->action_area), button, FALSE, FALSE, 2);

			button = _status_tab_create_action_button(GTK_STOCK_DELETE, TRUE, _("Remove list"));
			g_signal_connect(G_OBJECT(button), "clicked", (GCallback)_status_tab_remove_list_button_clicked, tab);
			gtk_box_pack_start(GTK_BOX(tab->action_area), button, FALSE, FALSE, 2);
			break;

		default:
			visible = FALSE;
	}

	_status_tab_show_action_area(widget, visible);
}

/*
 *	interface:
 */
static void
_status_tab_destroyed(GtkWidget *widget)
{
	_StatusTab *meta = g_object_get_data(G_OBJECT(widget), "meta");
	gchar group[128];

	/* remove callbacks from pixbuf loader */
	sprintf(group, "statustab-%d", meta->tab_id);
	g_debug("Removing callbacks from pixbuf loader (group=\"%s\")", group);
	mainwindow_remove_pixbuf_group(tabbar_get_mainwindow(meta->tabbar), group);

	/* cancel running operations */
	g_debug("Cancelling operations");
	g_cancellable_cancel(meta->cancellable);

	/* wait until worker thread is waiting for condition again */
	_status_tab_wait_for_worker(widget);

	/* send destroy message to background worker */
	g_debug("%s: sending destroy signal to worker: type_id=%d, id=\"%s\"", __func__, ((Tab *)meta)->type_id, ((Tab *)meta)->id.id);
	_status_tab_send_signal(widget, STATUS_TAB_SIGNAL_DESTROY);

	/* join thread */
	g_debug("%s: joining background worker: type_id=%d, id=\"%s\"", __func__, ((Tab *)meta)->type_id, ((Tab *)meta)->id.id);
	g_thread_join(meta->worker.thread);

	/* cleanup */
	g_debug("%s: freeing meta information: type_id=%d, id=\"%s\"", __func__, ((Tab *)meta)->type_id, ((Tab *)meta)->id.id);

	g_debug("Freeing worker mutex");
	g_static_mutex_free(&meta->worker.mutex);
	g_debug("Freeing condition");
	g_cond_free(meta->worker.condition);
	g_debug("Freeing status mutex");
	g_mutex_free(meta->status.mutex);
	g_debug("Destroying cancellable");
	g_object_unref(meta->cancellable);
	g_debug("Freeing accountlist");
	g_strfreev(meta->accountlist.accounts);
	g_mutex_free(meta->accountlist.mutex);
	g_free(meta->background_color);

	g_free(((Tab *)meta)->id.id);
	g_mutex_free(((Tab *)meta)->id.mutex);
	g_free(meta->owner);

	g_slice_free1(sizeof(_StatusTab), meta);
}

static void
_status_tab_refresh(GtkWidget *widget)
{
	gchar *tab_id;
	_StatusTab *meta = g_object_get_data(G_OBJECT(widget), "meta");
	GtkWidget *window;
	Config *config;
	Section *section;
	Value *value;
	const gchar *spec;

	if(!meta->initialized)
	{
		meta->initialized = TRUE;
		_status_tab_init(widget);
	}

	tab_id = tab_get_id((Tab *)meta);
	window = tabbar_get_mainwindow(meta->tabbar);

	/* get background color */
	config = mainwindow_lock_config(window);
	section = config_get_root(config);

	if((section = section_find_first_child(section, "View")))
	{
		if((value = section_find_first_value(section, "tweet-background-color")) && VALUE_IS_STRING(value))
		{
			spec = value_get_string(value);

			if(!meta->background_color || g_strcmp0(meta->background_color, spec))
			{
				if(meta->background_color)
				{
					g_free(meta->background_color);
					meta->background_changed = TRUE;
				}

				meta->background_color = g_strdup(value_get_string(value));
			}
		}
	}

	mainwindow_unlock_config(window);

	/* get account list */
	g_mutex_lock(meta->accountlist.mutex);
	g_strfreev(meta->accountlist.accounts);
	meta->accountlist.accounts = _status_tab_get_accounts(mainwindow_lock_config(window));
	mainwindow_unlock_config(window);
	g_mutex_unlock(meta->accountlist.mutex);

	g_debug("Refreshing status tab: type_id=%d, id=\"%s\"", ((Tab *)meta)->type_id, tab_id);
	_status_tab_send_signal(widget, STATUS_TAB_SIGNAL_REFRESH);

	g_free(tab_id);
}

static void
_status_tab_set_busy(GtkWidget *widget, gboolean busy)
{
	_StatusTab *tab = g_object_get_data(G_OBJECT(widget), "meta");
	TabTypeId type;
	MainWindowSyncStatus status;

	type = ((Tab *)tab)->type_id;
	status = mainwindow_get_current_sync_status(tabbar_get_mainwindow(tab->tabbar));

	/* check if page displays a list & if mainwindow is synchronizing list information */
	if(type == TAB_TYPE_ID_LIST) 
	{
		if(status == MAINWINDOW_SYNC_STATUS_SYNC_LISTS)
		{
			gtk_widget_set_visible(tab->action_area, busy ? FALSE : TRUE);
		}
		else
		{
			gtk_widget_set_visible(tab->action_area, TRUE);
		}
	}
}

static void
_status_tab_scroll(GtkWidget *widget, gboolean down)
{
	_StatusTab *tab = (_StatusTab *)g_object_get_data(G_OBJECT(widget), "meta");
	GtkWidget *scrollbar;
	gdouble value;

	scrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(tab->scrolled));

	if(down)
	{
		value = gtk_range_get_value(GTK_RANGE(scrollbar)) + 50;
	}
	else
	{
		value = gtk_range_get_value(GTK_RANGE(scrollbar)) - 50;
	}

	gtk_range_set_value(GTK_RANGE(scrollbar), value);
}

static TabFuncs status_tab_funcs =
{
	_status_tab_destroyed,
	_status_tab_refresh,
	_status_tab_set_busy,
	_status_tab_scroll
};

static gchar *
_status_tab_get_owner_from_id(const gchar *id, TabTypeId type_id, Config *config)
{
	gchar **pieces;
	gchar *owner = NULL;
	gchar **accounts;

	if(type_id == TAB_TYPE_ID_LIST)
	{
		if((pieces = g_strsplit(id, "@", 2)))
		{
			owner = g_strdup(pieces[0]);
			g_strfreev(pieces);
		}
	}
	else if(type_id != TAB_TYPE_ID_SEARCH)
	{
		owner = g_strdup(id);
	}

	if(owner)
	{
		accounts = _status_tab_get_accounts(config);

		if(!_status_tab_account_list_contains(accounts, owner))
		{
			g_free(owner);
			owner = NULL;
		}

		g_strfreev(accounts);
	}

	return owner;
}

GtkWidget *
status_tab_create(GtkWidget *tabbar, TabTypeId type_id, const gchar *id)
{
	GtkWidget *widget;
	GtkWidget *align;
	GtkWidget *action_area;
	GtkWidget *scrolled;
	GtkWidget *hbox;
	GtkWidget *vbox;
	_StatusTab *meta;
	static guint tab_id = 0;
	GtkWidget *mainwindow;
	Config *config;
	GError *err = NULL;

	/* create widget */
	g_debug("Creating new status tab");

	widget = gtk_vbox_new(FALSE, 5);

	/* action box */
	align = gtk_alignment_new(1, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(widget), align, FALSE, TRUE, 5);

	action_area = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(align), action_area);

	/* status list */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(widget), scrolled, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 2);

	/* create meta information */
	g_debug("Creating tab meta data");
	meta = (_StatusTab *)g_slice_alloc(sizeof(_StatusTab));

	((Tab *)meta)->widget = widget;
	((Tab *)meta)->funcs = &status_tab_funcs;
	((Tab *)meta)->type_id = type_id;
	((Tab *)meta)->id.id = g_strdup(id);
	((Tab *)meta)->id.mutex = g_mutex_new();

	if(tab_id == G_MAXUINT)
	{
		tab_id = 0;
	}

	meta->initialized = FALSE;
	meta->tab_id = tab_id++;
	meta->page = widget;
	meta->vbox = vbox;
	meta->action_area = action_area;
	meta->tabbar = tabbar;
	meta->scrolled = scrolled;
	meta->cancellable = g_cancellable_new();
	meta->worker.thread = NULL;
	meta->worker.condition = NULL;
	meta->worker.signal = STATUS_TAB_SIGNAL_IDLE;
	meta->status.mutex = g_mutex_new();
	meta->status.waiting = FALSE;
	meta->accountlist.accounts = NULL;
	meta->accountlist.mutex = g_mutex_new();
	meta->background_color = NULL;
	meta->background_changed = FALSE;

	mainwindow = tabbar_get_mainwindow(tabbar);
	config = mainwindow_lock_config(mainwindow);
	meta->owner = _status_tab_get_owner_from_id(id, type_id, config);
	mainwindow_unlock_config(mainwindow);

	g_object_set_data(G_OBJECT(widget), "meta", (gpointer)meta);

	/* start background worker */
	meta->worker.condition = g_cond_new();
	g_static_mutex_init(&meta->worker.mutex);

	meta->worker.thread = g_thread_create(_status_tab_worker, widget, TRUE, &err);

	if(!meta->worker.thread)
	{
		g_error("%s", err->message);
	}

	/* wait until worker thread is waiting for signals */
	_status_tab_wait_for_worker(widget);

	g_signal_connect(G_OBJECT(widget), "key-press-event", G_CALLBACK(_status_tab_key_press), meta);

	gtk_widget_show_all(widget);

	return widget;
}

gchar *
status_tab_get_owner(GtkWidget *page)
{
	_StatusTab *meta = g_object_get_data(G_OBJECT(page), "meta");

	if(meta->owner)
	{
		return g_strdup(meta->owner);
	}

	return NULL;
}

/**
 * @}
 */


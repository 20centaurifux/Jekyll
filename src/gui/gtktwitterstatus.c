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
 * \file gtktwitterstatus.c
 * \brief A GTK widget displaying a twitter status.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 28. December 2011
 */

#include <string.h>
#include <time.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include "gtktwitterstatus.h"
#include "gtklinklabel.h"
#include "marshal.h"
#include "../libsexy/sexy-url-label.h"

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Widgets
 * 	@{
 */

static void gtk_twitter_status_class_init(GtkTwitterStatusClass *klass);
static void gtk_twitter_status_init(GtkTwitterStatus *cpu);
static GObject *gtk_twitter_status_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params);
static void gtk_twitter_status_finalize(GObject *object);
static void gtk_twitter_status_set_property(GObject *object, guint property_id, const GValue *config, GParamSpec *pspec);
static void gtk_twitter_status_get_property(GObject *object, guint property_id, GValue *config, GParamSpec *pspec);
static void gtk_twitter_status_show(GtkWidget *widget);

enum
{
	PROP_0,
	PROP_GUID,
	PROP_PIXBUF,
	PROP_USERNAME,
	PROP_REALNAME,
	PROP_STATUS,
	PROP_TIMESTAMP,
	PROP_BACKGROUND_COLOR,
	PROP_SHOW_REPLY_BUTTON,
	PROP_SHOW_LISTS_BUTTON,
	PROP_SHOW_FRIENDSHIP_BUTTON,
	PROP_SHOW_RETWEET_BUTTON,
	PROP_FRIENDSHIP_BUTTON_HAS_TOOLIP,
	PROP_LISTS_BUTTON_HAS_TOOLIP,
	PROP_SHOW_DELETE_BUTTON,
	PROP_SELECTABLE
};

enum
{
	GTK_TWITTER_STATUS_ACTION_REPLY,
	GTK_TWITTER_STATUS_ACTION_EDIT_LISTS,
	GTK_TWITTER_STATUS_ACTION_EDIT_FRIENDSHIP,
	GTK_TWITTER_STATUS_ACTION_RETWEET,
	GTK_TWITTER_STATUS_ACTION_DELETE,
	GTK_TWITTER_STATUS_ACTION_SHOW_USER
};

/**
 * \struct _GtkTwitterStatusEventArg
 * \brief holds event data
 */
typedef struct
{
	/*! A GtkTwitterStatus instance. */
	GtkWidget *widget;
	/*! Id specifying the action event. */
	gint action;
} _GtkTwitterStatusEventArg;

/*! Creates a GtkTwitterStatus event argument. */
#define BUILD_GTK_TWITTER_STATUS_ARG(i) priv->args[i].widget = GTK_WIDGET(object); priv->args[i].action = i

/**
 * \struct _GtkTwitterStatusPrivate
 * \brief Private _GtkTwitterStatus data.
 */
struct _GtkTwitterStatusPrivate
{
	/*! An event box holding the main box. */
	GtkWidget *ebox;
	/*! A box holding all child widgets. */
	GtkWidget *hbox;
	/*! Guid of the status. */
	gchar *guid;
	/**
	 * \struct _image
	 * \brief Holds the image of the user.
	 *
	 * \var image
	 * \brief Image of the user.
	 */
	struct _image
	{
		/*! Displays the image. */
		GtkWidget *widget;
	} image;
	/**
	 * \struct _username
	 * \brief Holds username and related widget.
	 *
	 * \var username
	 * \brief Username and related widget.
	 */
	struct _username
	{
		/*! Value of the username. */
		gchar *value;
		/*! Displays the username. */
		GtkWidget *widget;
	} username;
	/**
	 * \struct _realname
	 * \brief Holds realname and related widget.
	 *
	 * \var realname
	 * \brief Realname and related widget.
	 */
	struct _realname
	{
		/*! Value of the realname. */
		gchar *value;
		/*! Displays the realname. */
		GtkWidget *widget;
	} realname;
	/**
	 * \struct _status
	 * \brief Holds status and related widget.
	 *
	 * \var status
	 * \brief Status and related widget.
	 */
	struct _status
	{
		/*! Value of the status. */
		gchar *value;
		/*! Enable text selection. */
		gboolean selectable;
		/*! Displays the status. */
		GtkWidget *widget;
	} status;
	/**
	 * \struct _timestamp
	 * \brief Holds timestamp and related widget.
	 *
	 * \var timestamp
	 * \brief Timestamp and related widget.
	 */
	struct _timestamp
	{
		/*! Value of the timestamp. */
		gint value;
		/*! Displays the timestamp. */
		GtkWidget *widget;
	} timestamp;
	/**
	 * \struct _background
	 * \brief Holds background color.
	 *
	 * \var background
	 * \brief Holds background color.
	 */
	struct _background
	{
		/*! The string specifying the color. */
		gchar *spec;
		/*! GdkColor. */
		GdkColor color;
	} background;
	/**
	 * \struct _reply_button
	 * \brief Holds reply button visibility and widget.
	 *
	 * \var reply_button
	 * \brief Reply button visibility and widget.
	 */
	struct _reply_button
	{
		/*! A button. */
		GtkWidget *widget;
		/*! Visibility of the button. */
		gboolean visible;
	} reply_button;
	/**
	 * \struct _edit_lists_button
	 * \brief Holds edit lists button visibility and widget.
	 *
	 * \var edit_lists_button
	 * \brief Edit lists button visibility and widget.
	 */
	struct _edit_lists_button
	{
		/*! A button. */
		GtkWidget *widget;
		/*! Visibility of the button. */
		gboolean visible;
		/*! TRUE if button has a tooltip. */
		gboolean tooltip;
	} edit_lists_button;
	/**
	 * \struct _edit_friendship_button
	 * \brief Holds edit friendship button visibility and widget.
	 *
	 * \var edit_friendship_button
	 * \brief Edit friendship button visibility and widget.
	 */
	struct _edit_friendship_button
	{
		/*! A button. */
		GtkWidget *widget;
		/*! Visibility of the button. */
		gboolean visible;
		/*! TRUE if button has a tooltip. */
		gboolean tooltip;
	} edit_friendship_button;
	/**
	 * \struct _retweet_button
	 * \brief Holds retweet button visibility and widget.
	 *
	 * \var retweet_button
	 * \brief Retweet button visibility and widget.
	 */
	struct _retweet_button
	{
		/*! A button. */
		GtkWidget *widget;
		/*! Visibility of the button. */
		gboolean visible;
	} retweet_button;
	/**
	 * \struct _delete_button
	 * \brief Holds delete button visibility and widget.
	 *
	 * \var delete_button
	 * \brief Delete button visibility and widget.
	 */
	struct _delete_button
	{
		/*! A button. */
		GtkWidget *widget;
		/*! Visibility of the button. */
		gboolean visible;
	} delete_button;
	/*! Event parameters. */
	_GtkTwitterStatusEventArg args[6];
};

/*! Define GtkTwitterStatus. */
G_DEFINE_TYPE(GtkTwitterStatus, gtk_twitter_status, GTK_TYPE_VBOX);

/*
 *	helpers:
 */
static void
_gtk_twitter_status_set_label_text(GtkWidget *label, const gchar *text)
{
	if(GTK_IS_LABEL(label))
	{
		gtk_label_set_text(GTK_LABEL(label), text);
	}
	else if(GTK_IS_LINK_LABEL(label))
	{
		gtk_link_label_set_text(GTK_LINK_LABEL(label), text);
	}
}

enum
{
	POS_TEXT,
	POS_LINK,
	POS_HASHTAG,
	POS_USER,
	POS_ANCHOR
};

static GString *
_gtk_twitter_status_append_text_to_buffer(GString *buffer, const gchar *text, gint offset, gint length)
{
	gint i = offset;
	gint end = offset + length - 1;

	while(i <= end)
	{
		if(text[i] == '&' && (i == end || (i != end && text[i + 1] == ' ')))
		{
			buffer = g_string_append(buffer, "&amp;");
		}
		else
		{
			buffer = g_string_append_c(buffer, text[i]);
		}

		++i;
	}

	return buffer;
}

static GString *
_gtk_twitter_status_append_link_to_buffer(GString *buffer, const gchar *text, gint offset, gint length)
{
	gchar url[280] = { 0 };
	gint pos = 0;
	gchar *escaped;

	memcpy(url, text + offset, length);

	while(url[pos] != '/' && url[pos + 1] != '/')
	{
		++pos;
	}
	pos += 3;

	if((escaped = g_markup_escape_text(url, -1)))
	{
		buffer = g_string_append(buffer, "<a href=\"");
		buffer = g_string_append_len(buffer, url, pos);
		buffer = g_string_append_uri_escaped(buffer, url + pos, "/", TRUE);
		buffer = g_string_append(buffer, "\">");
		buffer = g_string_append(buffer, escaped);
		buffer = g_string_append(buffer, "</a>");
		g_free(escaped);
	}
	else
	{
		g_warning("Couldn't escape string: \"%s\"", url);
	}

	return buffer;
}

static GString *
_gtk_twitter_status_append_hashtag_to_buffer(GString *buffer, const gchar *text, gint offset, gint length)
{
	gchar *hash;

	if((hash = g_markup_escape_text(text + offset, length)))
	{
		buffer = g_string_append(buffer, "<a href=\"http://twitter.com/#!/search?q=%23");
		buffer = g_string_append_uri_escaped(buffer, hash + 1, NULL, length - 1);
		buffer = g_string_append(buffer, "\">");
		buffer = g_string_append_len(buffer, hash, length);
		buffer = g_string_append(buffer, "</a>");
		g_free(hash);
	}
	else
	{
		g_warning("%s: couldn't escape string", __func__);
	}

	return buffer;
}

static GString *
_gtk_twitter_status_append_user_to_buffer(GString *buffer, const gchar *text, gint offset, gint length)
{
	gchar *user;
	gint sep = length;

	while(text[offset + sep - 1] == ':')
	{
		--sep;
	}

	if((user = g_strndup(text + offset, sep)))
	{
		buffer = g_string_append(buffer, "<a href=\"http://www.twitter.com/#!/");
		buffer = g_string_append_uri_escaped(buffer, user + 1, NULL, sep - 1);
		buffer = g_string_append(buffer, "\">");
		buffer = g_string_append_len(buffer, user, sep);
		buffer = g_string_append(buffer, "</a>");

		if(length > sep)
		{
			buffer = g_string_append(buffer, text + offset + sep);
		}

		g_free(user);
	}
	else
	{
		g_warning("%s: couldn't escape string", __func__);
	}

	return buffer;
}

static gboolean
_gtk_twitter_status_strprefix(const gchar *text, gint offset, const gchar *string)
{
	gint len0 = strlen(text + offset);
	gint len1 = strlen(string);
	gint i = 0;

	if(len0 < len1)
	{
		return FALSE;
	}

	for(i = 0; i < len1 - 1; ++i)
	{
		if(g_ascii_tolower(text[i + offset]) != g_ascii_tolower(string[i]))
		{
			return FALSE;
		}
	}

	return text[i + offset] == string[i];
}

static gint
_gtk_twitter_status_replace_anchor(GString *buffer, const gchar *text, gint offset)
{
	static const gchar *escape_chars[] =
	{
		"quot;", "amp;", "lt;", "gt;", "apos;", NULL

	};
	gint i = 0;

	while(escape_chars[i])
	{
		if(_gtk_twitter_status_strprefix(text, offset, escape_chars[i]))
		{
			g_string_append_printf(buffer, "&%s", escape_chars[i]);
			return offset + strlen(escape_chars[i]);
		}

		++i;
	}

	buffer = g_string_append(buffer, "&amp;");

	return offset;
}

static void
_gtk_twitter_status_set_status_text(GtkWidget *label, const gchar *text)
{
	gint length;
	gint offx;
	gint offy;
	gint prefix_length = 0;
	gint type = POS_TEXT;
	GString *buffer = g_string_sized_new(256);

	length = strlen(text);
	offx = offy = 0;

	while(text[offy])
	{
		if(type == POS_TEXT)
		{
			if(offy == 0 || g_ascii_isspace(text[offy - 1]))
			{
				/* check if the beginning of a link, hashtag or user can be found at the current position */
				if((offy < length - 7) &&      /* http:// */
				   (text[offy] == 'h' || text[offy] == 'H') &&
				   (text[offy + 1] == 't' || text[offy + 1] == 'T') &&
				   (text[offy + 2] == 't' || text[offy + 2] == 'T') &&
				   (text[offy + 3] == 'p' || text[offy + 3] == 'P') &&
				    text[offy + 4] == ':' && text[offy + 5] == '/' && text[offy + 6] == '/')
				{
					type = POS_LINK;
					prefix_length = 7;
				}
				else if((offy < length - 8) && /* https:// */
					(text[offy] == 'h' || text[offy] == 'H') &&
					(text[offy + 1] == 't' || text[offy + 1] == 'T') &&
					(text[offy + 2] == 't' || text[offy + 2] == 'T') &&
					(text[offy + 3] == 'p' || text[offy + 3] == 'P') &&
					(text[offy + 4] == 's' || text[offy + 4] == 'S') &&
					 text[offy + 5] == ':' && text[offy + 6] == '/' && text[offy + 7] == '/')
				{
					type = POS_LINK;
					prefix_length = 8;
				}
				else if((offy < length - 6) && /* ftp:// */
					(text[offy] == 'f' || text[offy] == 'F') &&
					(text[offy + 1] == 't' || text[offy + 1] == 'T') &&
					(text[offy + 2] == 'p' || text[offy + 2] == 'P') &&
					 text[offy + 3] == ':' && text[offy + 4] == '/' && text[offy + 5] == '/')
				{
					prefix_length = 6;
					type = POS_LINK;
				}
				else if((offy < length - 7) && /* ftps:// */
					(text[offy] == 'f' || text[offy] == 'F') &&
					(text[offy + 1] == 't' || text[offy + 1] == 'T') &&
					(text[offy + 2] == 'p' || text[offy + 2] == 'P') &&
					(text[offy + 3] == 's' || text[offy + 3] == 'S') &&
					 text[offy + 4] == ':' && text[offy + 5] == '/' && text[offy + 6] == '/')
				{
					type = POS_LINK;
					prefix_length = 7;
				}
				else if(text[offy] == '#')     /* hashtag (#) */
				{
					type = POS_HASHTAG;
					prefix_length = 1;
				}
				else if(text[offy] == '@')     /* user (@) */
				{
					type = POS_USER;
					prefix_length = 1;
				}
			}

			if(text[offy] == '&')
			{
				type = POS_ANCHOR;
				prefix_length = 1;
			}

			/* check if type has been changed */
			if(type == POS_TEXT)
			{
				++offy;
			}
			else
			{
				/* append text to buffer & update offsets */
				buffer = _gtk_twitter_status_append_text_to_buffer(buffer, text, offx, offy - offx);
				offx = offy;
				offy += prefix_length;
			}
		}
		else
		{
			/* check if we've found the end of the link/hashtag at the current position */
			if(type != POS_ANCHOR && (text[offy] == ' ' || (type == POS_USER && text[offy] == ':') || (type == POS_HASHTAG && (g_unichar_iscntrl(g_utf8_get_char_validated(text+ offy, 1)) || text[offy] == ':'))))
			{
				/* append link/hashtag to buffer */
				switch(type)
				{
					case POS_LINK:
						buffer = _gtk_twitter_status_append_link_to_buffer(buffer, text, offx, offy - offx);
						break;

					case POS_HASHTAG:
						buffer = _gtk_twitter_status_append_hashtag_to_buffer(buffer, text, offx, offy - offx);
						break;

					case POS_USER:
						buffer = _gtk_twitter_status_append_user_to_buffer(buffer, text, offx, offy - offx);
						break;

					default:
						g_warning("%s: invalid type (%d)", __func__, type);
				}

				offx = offy;
				type = POS_TEXT;
			}
			else if(type == POS_ANCHOR)
			{
				offy = _gtk_twitter_status_replace_anchor(buffer, text, offy);
				offx = offy;
				type = POS_TEXT;
			}
			else
			{
				++offy;
			}
		}
	}

	/* "flush" buffer */
	if(offx < length)
	{
		switch(type)
		{
			case POS_TEXT:
				buffer = _gtk_twitter_status_append_text_to_buffer(buffer, text, offx, offy - offx);
				break;
			
			case POS_LINK:
				buffer = _gtk_twitter_status_append_link_to_buffer(buffer, text, offx, offy - offx);
				break;
		
			case POS_HASHTAG:
				buffer = _gtk_twitter_status_append_hashtag_to_buffer(buffer, text, offx, offy - offx);
				break;

			case POS_USER:
				buffer = _gtk_twitter_status_append_user_to_buffer(buffer, text, offx, offy - offx);
				break;

			case POS_ANCHOR:
				_gtk_twitter_status_replace_anchor(buffer, text, offy);
				break;

			default:
				g_warning("%s: invalid type (%d)", __func__, type);

		}
	}

	sexy_url_label_set_markup(SEXY_URL_LABEL(label), buffer->str);
	g_string_free(buffer, TRUE);
}

static void
_gtk_twitter_status_set_timestamp(GtkWidget *label, gint64 timestamp)
{
	time_t time = (time_t)timestamp;

	if(time > 0)
	{
		_gtk_twitter_status_set_label_text(label, ctime(&time));
	}
	else
	{
		_gtk_twitter_status_set_label_text(label, NULL);
	}
}

static void
_gtk_twitter_status_update_button_visibility(GtkWidget *twitter_status)
{
	GtkTwitterStatusPrivate *priv = GTK_TWITTER_STATUS(twitter_status)->priv;

	gtk_widget_set_visible(priv->reply_button.widget, priv->reply_button.visible);
	gtk_widget_set_visible(priv->edit_lists_button.widget, priv->edit_lists_button.visible);
	gtk_widget_set_visible(priv->edit_friendship_button.widget, priv->edit_friendship_button.visible);
	gtk_widget_set_visible(priv->retweet_button.widget, priv->retweet_button.visible);
	gtk_widget_set_visible(priv->delete_button.widget, priv->delete_button.visible);
}

static void
_gtk_twitter_status_enable_tooltip(GtkWidget *widget, gboolean enabled)
{
	g_object_set(G_OBJECT(widget), "has-tooltip", enabled, NULL);
}

static PangoAttrList *
_gtk_twitter_status_create_link_label_attributes(void)
{
	PangoAttrList *attrs;
	PangoAttribute *attr;

	attrs = pango_attr_list_new();

	attr = pango_attr_scale_new(PANGO_SCALE_SMALL);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_foreground_new(0, 0, 0xFFFF);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_underline_new(PANGO_UNDERLINE_NONE);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_style_new(PANGO_STYLE_NORMAL);

	return attrs;
}

/*
 *	event handler:
 */
static void
_gtk_twitter_status_link_label_clicked(GtkLinkLabel *label, _GtkTwitterStatusEventArg *arg)
{
	GtkTwitterStatusPrivate *priv;

	priv = GTK_TWITTER_STATUS(arg->widget)->priv;

	switch(arg->action)
	{
		case GTK_TWITTER_STATUS_ACTION_REPLY:
			g_signal_emit_by_name(arg->widget, "reply-button-clicked", priv->guid);
			break;

		case GTK_TWITTER_STATUS_ACTION_EDIT_LISTS:
			g_signal_emit_by_name(arg->widget, "edit-lists-button-clicked", priv->username.value);
			break;

		case GTK_TWITTER_STATUS_ACTION_EDIT_FRIENDSHIP:
			g_signal_emit_by_name(arg->widget, "edit-friendship-button-clicked", priv->username.value);
			break;

		case GTK_TWITTER_STATUS_ACTION_RETWEET:
			g_signal_emit_by_name(arg->widget, "retweet-button-clicked", priv->guid);
			break;

		case GTK_TWITTER_STATUS_ACTION_DELETE:
			g_signal_emit_by_name(arg->widget, "delete-button-clicked", priv->guid);
			break;

		case GTK_TWITTER_STATUS_ACTION_SHOW_USER:
			g_signal_emit_by_name(arg->widget, "username-activated", priv->username.value);
			break;

		default:
			g_warning("%s: invalid action id", __func__);
	}
}

static gboolean
_gtk_twitter_status_link_label_query_tooltip(GtkLinkLabel *label, gint x, gint y, gboolean keyboad_mode, GtkTooltip *tooltip, _GtkTwitterStatusEventArg *arg)
{
	gboolean result = FALSE;

	if(!gtk_widget_is_sensitive(GTK_WIDGET(label)))
	{
		return FALSE;
	}

	switch(arg->action)
	{
		case GTK_TWITTER_STATUS_ACTION_EDIT_LISTS:
			g_signal_emit_by_name(arg->widget, "edit-lists-button-query-tooltip", tooltip, &result);
			break;

		case GTK_TWITTER_STATUS_ACTION_EDIT_FRIENDSHIP:
			g_signal_emit_by_name(arg->widget, "edit-friendship-button-query-tooltip", tooltip, &result);
			break;

		default:
			g_warning("%s: invalid action id", __func__);
	}

	return result;
}

static void
_gtk_twitter_status_url_activated(SexyUrlLabel *label, const gchar *url, gpointer user_data)
{
	g_signal_emit_by_name((GtkWidget *)user_data, "url-activated", url);
}

static void
_gtk_twitter_status_status_label_grab_focus(GtkWidget *widget, GtkTwitterStatus *status)
{
	g_signal_emit_by_name(status, "grab-focus");
}

/*
 *	color:
 */
static void
_gtk_twitter_status_set_background(GtkTwitterStatus *twitter_status, gchar *spec)
{
	GdkColor color;
	GtkTwitterStatusPrivate *priv = GTK_TWITTER_STATUS(twitter_status)->priv;
	GtkWidget *w = priv->ebox;
	gint states[] = { GTK_STATE_NORMAL, GTK_STATE_ACTIVE, GTK_STATE_PRELIGHT, GTK_STATE_SELECTED, GTK_STATE_INSENSITIVE };
	gint i = 0;

	if(gdk_color_parse(spec, &color))
	{
		for(i = 0; i < 5; ++i)
		{
			gtk_widget_modify_bg(GTK_WIDGET(w), states[i], &color);
			gtk_widget_modify_bg(GTK_WIDGET(priv->username.widget), states[i], &color);
			gtk_widget_modify_bg(GTK_WIDGET(priv->edit_friendship_button.widget), states[i], &color);
			gtk_widget_modify_bg(GTK_WIDGET(priv->reply_button.widget), states[i], &color);
			gtk_widget_modify_bg(GTK_WIDGET(priv->retweet_button.widget), states[i], &color);
			gtk_widget_modify_bg(GTK_WIDGET(priv->edit_lists_button.widget), states[i], &color);
			gtk_widget_modify_bg(GTK_WIDGET(priv->delete_button.widget), states[i], &color);
		}
	}
	else
	{
		g_warning("%s: couldn't parse color \"%s\"", __func__, spec);
	}
}

/*
 *	implementation:
 */
static const gchar *
_gtk_twitter_status_get_guid(GtkTwitterStatus *twitter_status)
{
	return twitter_status->priv->guid;
}

static gint
_gtk_twitter_status_get_timestamp(GtkTwitterStatus *twitter_status)
{
	return twitter_status->priv->timestamp.value;
}

static void
gtk_twitter_status_class_init(GtkTwitterStatusClass *klass)
{
	GtkWidgetClass *widget_class;
	GObjectClass *object_class;

	g_type_class_add_private(klass, sizeof(GtkTwitterStatusPrivate));

	widget_class = (GtkWidgetClass *)klass;
	object_class = (GObjectClass *)klass;

	widget_class->show = gtk_twitter_status_show;

	object_class->constructor= gtk_twitter_status_constructor;
	object_class->finalize = gtk_twitter_status_finalize;
	object_class->set_property = gtk_twitter_status_set_property;
	object_class->get_property = gtk_twitter_status_get_property;

	klass->get_guid = _gtk_twitter_status_get_guid;
	klass->get_timestamp= _gtk_twitter_status_get_timestamp;

	/* install properties */
	g_object_class_install_property(object_class,
		                        PROP_PIXBUF,
		                        g_param_spec_object("pixbuf", NULL, NULL, GDK_TYPE_PIXBUF, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_GUID,
	                                g_param_spec_string("guid", NULL, NULL, NULL, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_USERNAME,
	                                g_param_spec_string("username", NULL, NULL, NULL, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_REALNAME,
	                                g_param_spec_string("realname", NULL, NULL, NULL, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_STATUS,
	                                g_param_spec_string("status", NULL, NULL, NULL, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_TIMESTAMP,
	                                g_param_spec_int("timestamp", NULL, NULL, 0, G_MAXINT, 0, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_BACKGROUND_COLOR,
	                                g_param_spec_string("background-color", NULL, NULL, NULL, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SHOW_REPLY_BUTTON,
	                                g_param_spec_boolean("show-reply-button", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SHOW_LISTS_BUTTON,
	                                g_param_spec_boolean("show-edit-lists-button", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_LISTS_BUTTON_HAS_TOOLIP,
	                                g_param_spec_boolean("edit-lists-button-has-tooltip", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SHOW_FRIENDSHIP_BUTTON,
	                                g_param_spec_boolean("show-edit-friendship-button", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_FRIENDSHIP_BUTTON_HAS_TOOLIP,
	                                g_param_spec_boolean("edit-friendship-button-has-tooltip", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SHOW_RETWEET_BUTTON,
	                                g_param_spec_boolean("show-retweet-button", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SHOW_DELETE_BUTTON,
	                                g_param_spec_boolean("show-delete-button", NULL, NULL, FALSE, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SELECTABLE,
	                                g_param_spec_boolean("selectable", NULL, NULL, FALSE, G_PARAM_READWRITE));

	/* install signals */
	g_signal_new("reply-button-clicked", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new("edit-lists-button-clicked", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new("edit-lists-button-query-tooltip", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_BOOLEAN__OBJECT, G_TYPE_BOOLEAN, 1, G_TYPE_OBJECT);
	g_signal_new("delete-button-clicked", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new("username-activated", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new("url-activated", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new("edit-friendship-button-clicked", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new("edit-friendship-button-query-tooltip", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_BOOLEAN__OBJECT, G_TYPE_BOOLEAN, 1, G_TYPE_OBJECT);
	g_signal_new("retweet-button-clicked", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
gtk_twitter_status_init(GtkTwitterStatus *twitter_status)
{
	twitter_status->priv = G_TYPE_INSTANCE_GET_PRIVATE(twitter_status, GTK_TYPE_TWITTER_STATUS, GtkTwitterStatusPrivate);
	memset(twitter_status->priv, 0, sizeof(GtkTwitterStatusPrivate));
	twitter_status->priv->reply_button.visible = FALSE;
	twitter_status->priv->edit_friendship_button.visible = TRUE;
	twitter_status->priv->edit_lists_button.visible = TRUE;
	twitter_status->priv->delete_button.visible = FALSE;
}

static GObject*
gtk_twitter_status_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params)
{
	GObject *object;
	GtkTwitterStatusPrivate *priv;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *alignment;
	PangoAttrList *attrs;
	PangoAttribute *attr;

	object = G_OBJECT_CLASS(gtk_twitter_status_parent_class)->constructor(type, n_construct_properties, construct_params);

	gtk_widget_push_composite_child();

	priv = GTK_TWITTER_STATUS(object)->priv;

	/* mainbox */
	priv->hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(priv->hbox), 2);

	/* image */
	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(priv->hbox), alignment, FALSE, FALSE, 0);

	priv->image.widget = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(alignment), priv->image.widget);

	/* username */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->hbox), vbox, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	priv->username.widget = gtk_link_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), priv->username.widget, FALSE, FALSE, 0);

	attrs = pango_attr_list_new();
	attr = pango_attr_scale_new(PANGO_SCALE_LARGE);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
	pango_attr_list_insert(attrs, attr);
	gtk_link_label_set_attributes(GTK_LINK_LABEL(priv->username.widget), attrs);
	pango_attr_list_unref(attrs);

	/* realname */
	priv->realname.widget = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), priv->realname.widget, FALSE, FALSE, 0);

	attrs = pango_attr_list_new();
	attr = pango_attr_scale_new(PANGO_SCALE_LARGE);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
	pango_attr_list_insert(attrs, attr);
	gtk_label_set_attributes(GTK_LABEL(priv->realname.widget), attrs);
	pango_attr_list_unref(attrs);

	/* status */
	priv->status.widget = sexy_url_label_new();
	gtk_label_set_line_wrap(GTK_LABEL(priv->status.widget), TRUE);
	gtk_label_set_line_wrap_mode(GTK_LABEL(priv->status.widget), PANGO_WRAP_WORD);
	gtk_label_set_selectable(GTK_LABEL(priv->status.widget), TRUE);
	priv->status.selectable = TRUE;
	gtk_misc_set_alignment(GTK_MISC(priv->status.widget), 0, 0);

	gtk_box_pack_start(GTK_BOX(vbox), priv->status.widget, TRUE, TRUE, 5);

	attrs = pango_attr_list_new();
	attr = pango_attr_scale_new(PANGO_SCALE_MEDIUM);
	pango_attr_list_insert(attrs, attr);
	gtk_label_set_attributes(GTK_LABEL(priv->status.widget), attrs);
	pango_attr_list_unref(attrs);

	g_signal_connect(G_OBJECT(priv->status.widget), "grab-focus", (GCallback)_gtk_twitter_status_status_label_grab_focus, object);

	/* timestamp */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	priv->timestamp.widget = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), priv->timestamp.widget, FALSE, FALSE, 0);

	attrs = pango_attr_list_new();
	attr = pango_attr_scale_new(PANGO_SCALE_SMALL);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
	pango_attr_list_insert(attrs, attr);
	attr = pango_attr_foreground_new(0x7777, 0x7777, 0x9999);
	pango_attr_list_insert(attrs, attr);
	gtk_label_set_attributes(GTK_LABEL(priv->timestamp.widget), attrs);
	pango_attr_list_unref(attrs);

	/* edit friendship button */
	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

	priv->edit_friendship_button.widget = gtk_link_label_new(_("Friendship"));
	g_object_set(G_OBJECT(priv->edit_friendship_button.widget), "xmargin", 2, NULL);
	gtk_container_add(GTK_CONTAINER(alignment), priv->edit_friendship_button.widget);

	attrs = _gtk_twitter_status_create_link_label_attributes();
	gtk_link_label_set_attributes(GTK_LINK_LABEL(priv->edit_friendship_button.widget), attrs);
	pango_attr_list_unref(attrs);

	/* reply button */
	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

	priv->reply_button.widget = gtk_link_label_new(_("Reply"));
	g_object_set(G_OBJECT(priv->reply_button.widget), "xmargin", 2, NULL);
	gtk_container_add(GTK_CONTAINER(alignment), priv->reply_button.widget);

	attrs = _gtk_twitter_status_create_link_label_attributes();
	gtk_link_label_set_attributes(GTK_LINK_LABEL(priv->reply_button.widget), attrs);
	pango_attr_list_unref(attrs);

	/* retweet button */
	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

	priv->retweet_button.widget = gtk_link_label_new(_("Retweet"));
	g_object_set(G_OBJECT(priv->retweet_button.widget), "xmargin", 2, NULL);
	gtk_container_add(GTK_CONTAINER(alignment), priv->retweet_button.widget);

	attrs = _gtk_twitter_status_create_link_label_attributes();
	gtk_link_label_set_attributes(GTK_LINK_LABEL(priv->retweet_button.widget), attrs);
	pango_attr_list_unref(attrs);

	/* edit lists button */
	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

	priv->edit_lists_button.widget = gtk_link_label_new(_("Lists"));
	g_object_set(G_OBJECT(priv->edit_lists_button.widget), "xmargin", 2, NULL);
	gtk_container_add(GTK_CONTAINER(alignment), priv->edit_lists_button.widget);

	attrs = _gtk_twitter_status_create_link_label_attributes();
	gtk_link_label_set_attributes(GTK_LINK_LABEL(priv->edit_lists_button.widget), attrs);
	pango_attr_list_unref(attrs);

	/* delete button */
	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

	priv->delete_button.widget = gtk_link_label_new(_("Delete"));
	g_object_set(G_OBJECT(priv->delete_button.widget), "xmargin", 2, NULL);
	gtk_container_add(GTK_CONTAINER(alignment), priv->delete_button.widget);

	attrs = _gtk_twitter_status_create_link_label_attributes();
	gtk_link_label_set_attributes(GTK_LINK_LABEL(priv->delete_button.widget), attrs);
	pango_attr_list_unref(attrs);

	/* pack widgets */
	priv->ebox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(priv->ebox), priv->hbox);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(frame), priv->ebox);

	gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);

	/* connect signals */
	BUILD_GTK_TWITTER_STATUS_ARG(GTK_TWITTER_STATUS_ACTION_REPLY);
	BUILD_GTK_TWITTER_STATUS_ARG(GTK_TWITTER_STATUS_ACTION_EDIT_LISTS);
	BUILD_GTK_TWITTER_STATUS_ARG(GTK_TWITTER_STATUS_ACTION_EDIT_FRIENDSHIP);
	BUILD_GTK_TWITTER_STATUS_ARG(GTK_TWITTER_STATUS_ACTION_RETWEET);
	BUILD_GTK_TWITTER_STATUS_ARG(GTK_TWITTER_STATUS_ACTION_DELETE);
	BUILD_GTK_TWITTER_STATUS_ARG(GTK_TWITTER_STATUS_ACTION_SHOW_USER);

	g_signal_connect(priv->reply_button.widget, "clicked", (GCallback)_gtk_twitter_status_link_label_clicked, &priv->args[GTK_TWITTER_STATUS_ACTION_REPLY]);
	g_signal_connect(priv->edit_lists_button.widget, "clicked", (GCallback)_gtk_twitter_status_link_label_clicked, &priv->args[GTK_TWITTER_STATUS_ACTION_EDIT_LISTS]);
	g_signal_connect(priv->edit_lists_button.widget, "query-tooltip", (GCallback)_gtk_twitter_status_link_label_query_tooltip, &priv->args[GTK_TWITTER_STATUS_ACTION_EDIT_LISTS]);
	g_signal_connect(priv->edit_friendship_button.widget, "clicked", (GCallback)_gtk_twitter_status_link_label_clicked, &priv->args[GTK_TWITTER_STATUS_ACTION_EDIT_FRIENDSHIP]);
	g_signal_connect(priv->edit_friendship_button.widget, "query-tooltip", (GCallback)_gtk_twitter_status_link_label_query_tooltip, &priv->args[GTK_TWITTER_STATUS_ACTION_EDIT_FRIENDSHIP]);
	g_signal_connect(priv->retweet_button.widget, "clicked", (GCallback)_gtk_twitter_status_link_label_clicked, &priv->args[GTK_TWITTER_STATUS_ACTION_RETWEET]);
	g_signal_connect(priv->delete_button.widget, "clicked", (GCallback)_gtk_twitter_status_link_label_clicked, &priv->args[GTK_TWITTER_STATUS_ACTION_DELETE]);
	g_signal_connect(priv->username.widget, "clicked", (GCallback)_gtk_twitter_status_link_label_clicked, &priv->args[GTK_TWITTER_STATUS_ACTION_SHOW_USER]);
	g_signal_connect(priv->status.widget, "url_activated", (GCallback)_gtk_twitter_status_url_activated, object);

	gtk_widget_push_composite_child();

	return object;
}

static void
gtk_twitter_status_finalize(GObject *object)
{
	GtkTwitterStatusPrivate *priv;

	priv = GTK_TWITTER_STATUS(object)->priv;

	g_free(priv->guid);
	g_free(priv->background.spec);
	g_free(priv->username.value);
	g_free(priv->realname.value);
	g_free(priv->status.value);

	G_OBJECT_CLASS(gtk_twitter_status_parent_class)->finalize(object);
}

static void
gtk_twitter_status_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GtkTwitterStatus *self = (GtkTwitterStatus*)object;

	switch(property_id)
	{
		case PROP_PIXBUF:
			gtk_image_set_from_pixbuf(GTK_IMAGE(self->priv->image.widget), (GdkPixbuf *)g_value_get_object(value));
			break;

		case PROP_GUID:
			g_free(self->priv->guid);
			self->priv->guid = g_value_dup_string(value);
			break;

		case PROP_USERNAME:
			g_free(self->priv->username.value);
			self->priv->username.value = g_value_dup_string(value);
			_gtk_twitter_status_set_label_text(self->priv->username.widget, self->priv->username.value);
			break;

		case PROP_REALNAME:
			g_free(self->priv->realname.value);
			self->priv->realname.value = g_value_dup_string(value);
			_gtk_twitter_status_set_label_text(self->priv->realname.widget, self->priv->realname.value);
			break;

		case PROP_STATUS:
			g_free(self->priv->status.value);
			self->priv->status.value = g_value_dup_string(value);
			_gtk_twitter_status_set_status_text(self->priv->status.widget, self->priv->status.value);
			break;

		case PROP_BACKGROUND_COLOR:
			g_free(self->priv->background.spec);
			self->priv->background.spec = g_value_dup_string(value);
			_gtk_twitter_status_set_background(self, self->priv->background.spec);
			break;

		case PROP_TIMESTAMP:
			self->priv->timestamp.value = g_value_get_int(value);
			_gtk_twitter_status_set_timestamp(self->priv->timestamp.widget, self->priv->timestamp.value);
			break;

		case PROP_SHOW_REPLY_BUTTON:
			self->priv->reply_button.visible = g_value_get_boolean(value);
			_gtk_twitter_status_update_button_visibility(GTK_WIDGET(self));
			break;

		case PROP_SHOW_LISTS_BUTTON:
			self->priv->edit_lists_button.visible = g_value_get_boolean(value);
			_gtk_twitter_status_update_button_visibility(GTK_WIDGET(self));
			break;

		case PROP_LISTS_BUTTON_HAS_TOOLIP:
			self->priv->edit_lists_button.tooltip = g_value_get_boolean(value);
			_gtk_twitter_status_enable_tooltip(self->priv->edit_lists_button.widget, self->priv->edit_lists_button.tooltip);
			break;

		case PROP_SHOW_FRIENDSHIP_BUTTON:
			self->priv->edit_friendship_button.visible = g_value_get_boolean(value);
			_gtk_twitter_status_update_button_visibility(GTK_WIDGET(self));
			break;

		case PROP_FRIENDSHIP_BUTTON_HAS_TOOLIP:
			self->priv->edit_friendship_button.tooltip = g_value_get_boolean(value);
			_gtk_twitter_status_enable_tooltip(self->priv->edit_friendship_button.widget, self->priv->edit_friendship_button.tooltip);
			break;

		case PROP_SHOW_RETWEET_BUTTON:
			self->priv->retweet_button.visible = g_value_get_boolean(value);
			_gtk_twitter_status_update_button_visibility(GTK_WIDGET(self));
			break;

		case PROP_SHOW_DELETE_BUTTON:
			self->priv->delete_button.visible = g_value_get_boolean(value);
			_gtk_twitter_status_update_button_visibility(GTK_WIDGET(self));
			break;

		case PROP_SELECTABLE:
			self->priv->status.selectable = g_value_get_boolean(value);
			gtk_label_set_selectable(GTK_LABEL(self->priv->status.widget), self->priv->status.selectable);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
gtk_twitter_status_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GtkTwitterStatus *self = (GtkTwitterStatus*)object;

	switch(property_id)
	{
		case PROP_PIXBUF:
			g_value_set_object(value, gtk_image_get_pixbuf(GTK_IMAGE(self->priv->image.widget)));
			break;

		case PROP_GUID:
			g_value_set_string(value, self->priv->guid);
			break;

		case PROP_USERNAME:
			g_value_set_string(value, self->priv->username.value);
			break;

		case PROP_REALNAME:
			g_value_set_string(value, self->priv->realname.value);
			break;

		case PROP_STATUS:
			g_value_set_string(value, self->priv->status.value);
			break;
	
		case PROP_TIMESTAMP:
			g_value_set_int(value, self->priv->timestamp.value);
			break;

		case PROP_BACKGROUND_COLOR:
			g_value_set_string(value, self->priv->background.spec);
			break;
	
		case PROP_SHOW_REPLY_BUTTON:
			g_value_set_boolean(value, self->priv->reply_button.visible);
			break;

		case PROP_SHOW_LISTS_BUTTON:
			g_value_set_boolean(value, self->priv->edit_lists_button.visible);
			break;

		case PROP_LISTS_BUTTON_HAS_TOOLIP:
			g_value_set_boolean(value, self->priv->edit_lists_button.tooltip);
			break;

		case PROP_SHOW_FRIENDSHIP_BUTTON:
			g_value_set_boolean(value, self->priv->edit_friendship_button.visible);
			break;

		case PROP_FRIENDSHIP_BUTTON_HAS_TOOLIP:
			g_value_set_boolean(value, self->priv->edit_friendship_button.tooltip);
			break;

		case PROP_SHOW_RETWEET_BUTTON:
			g_value_set_boolean(value, self->priv->retweet_button.visible);
			_gtk_twitter_status_update_button_visibility(GTK_WIDGET(self));
			break;

		case PROP_SHOW_DELETE_BUTTON:
			g_value_set_boolean(value, self->priv->delete_button.visible);
			_gtk_twitter_status_update_button_visibility(GTK_WIDGET(self));
			break;

		case PROP_SELECTABLE:
			g_value_set_boolean(value, self->priv->status.selectable);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
gtk_twitter_status_show(GtkWidget *widget)
{
	/* call parent method */
	GTK_WIDGET_CLASS(gtk_twitter_status_parent_class)->show(widget);

	/* update button visibility */
	_gtk_twitter_status_update_button_visibility(widget);
}

/*
 *	public:
 */
GtkWidget *
gtk_twitter_status_new(void)
{
	return GTK_WIDGET(g_object_new(gtk_twitter_status_get_type(), NULL));
}

const gchar *
gtk_twitter_status_get_guid(GtkTwitterStatus *twitter_status)
{
	return GTK_TWITTER_STATUS_GET_CLASS(twitter_status)->get_guid(twitter_status);
}

gint
gtk_twitter_status_get_timestamp(GtkTwitterStatus *twitter_status)
{
	return GTK_TWITTER_STATUS_GET_CLASS(twitter_status)->get_timestamp(twitter_status);
}

/**
 * @}
 * @}
 */


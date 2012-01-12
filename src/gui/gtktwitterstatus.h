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
 * \file gtktwitterstatus.h
 * \brief A GTK widget displaying a twitter status.
 * \author sebastian fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 12. January 2012
 */

#ifndef __GTK_TWITTER_STATUS_H__
#define __GTK_TWITTER_STATUS_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Widgets
 * 	@{
 */

/*! Get GType. */
#define GTK_TYPE_TWITTER_STATUS            (gtk_twitter_status_get_type())
/*! Cast to GtkTwitterStatus. */
#define GTK_TWITTER_STATUS(obj)            G_TYPE_CHECK_INSTANCE_CAST(obj, gtk_twitter_status_get_type(), GtkTwitterStatus)
/*! Cast to GtkTwitterStatusClass. */
#define GTK_TWITTER_STATUS_CLASS(klass)    G_TYPE_CHECK_CLASS_CAST(klass, gtk_twitter_status_get_type(), GtkTwitterStatusClass)
/*! Check if instance is GtkTwitterStatus. */
#define GTK_IS_TWITTER_STATUS(obj)         G_TYPE_CHECK_INSTANCE_TYPE(obj, gtk_twitter_status_get_type())
/*! Get GtkTwitterStatusClass from GtkTwitterStatus. */
#define GTK_TWITTER_STATUS_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS((inst), GTK_TWITTER_STATUS_0CLASS, GtkTwitterStatusClass))

/*! A type definition for _GtkTwitterStatusPrivate*/
typedef struct _GtkTwitterStatusPrivate GtkTwitterStatusPrivate;

/*! A type definition for _GtkTwitterStatus*/
typedef struct _GtkTwitterStatus GtkTwitterStatus;

/*! A type definition for _GtkTwitterStatusClass*/
typedef struct _GtkTwitterStatusClass GtkTwitterStatusClass;

/**
 * \struct _GtkTwitterStatus
 * \brief A widget displaying a Twitter status.
 */
struct _GtkTwitterStatus
{
	/*! The parent instance. */
	GtkVBox parent_object;
	/*! Private data. */
	GtkTwitterStatusPrivate *priv;
};

/**
 * \struct _GtkTwitterStatusClass
 * \brief The _GtkTwitterStatus class structure.
 *
 * A widget for displaying a Twitter status. This class has the following properties:
 * - \b pixbuf: image of the user (GdkPixbuf, rw)\n
 * - \b username: name of the user account (string, rw)\n
 * - \b realname: real name of the user account (string, rw)\n
 * - \b status: text of the status (string, rw)\n
 * - \b timestamp: timestamp (integer, rw)\n
 * - \b show-reply-button: show reply button (boolean, rw)\n
 * - \b show-edit-lists-button: show edit lists button (boolean, rw)\n
 * - \b edit-lists-button-has-tooltip: activate list button tooltip (boolean, rw)\n
 * - \b show-edit-friendship-button: show edit friendship button (boolean, rw)\n
 * - \b show-retweet-button: show retweet button (boolean, rw)\n
 * - \b edit-friendship-button-has-tooltip: activate friendship button tooltip (boolean, rw)\n
 * - \b show-delete-button: show delete button (boolean, rw)\n
 * - \b show-history-button: show history button (boolean, rw)\n
 * - \b selectable: enable text selection (boolean, rw)\n
 * - \b background-color: the background-color (string, rw)\n
 *
 * The class has the following signals:
 * - \b reply-button-clicked: void user_function(GtkTwitterStatus *status, gchar *status_guid, gpointer user_data)
 * - \b edit-lists-button-clicked: void user_function(GtkTwitterStatus *status, gchar *username, gpointer user_data)
 * - \b edit-lists-button-query-tooltip: gboolean user_function(GtkTwitterStatus *status, GtkTooltip *tooltip, gpointer user_data)
 * - \b edit-friendship-button-clicked: void user_function(GtkTwitterStatus *status, gchar *username, gpointer user_data)
 * - \b edit-friendship-button-query-tooltip: gboolean user_function(GtkTwitterStatus *status, GtkTooltip *tooltip, gpointer user_data)
 * - \b retweet-button-clicked: void user_function(GtkTwitterStatus *status, gchar *guid, gpointer user_data)
 * - \b delete-button-clicked: void user_function(GtkTwitterStatus *status, gchar *status_guid, gpointer user_data)
 * - \b history-button-clicked: void user_function(GtkTwitterStatus *status, gchar *status_guid, gpointer user_data)
 * - \b username-clicked: void user_function(GtkTwitterStatus *status, gchar *username, gpointer user_data)
 * - \b url-activated: void user_function(GtkTwitterStatus *status, gchar *url, gpointer user_data)
 */
struct _GtkTwitterStatusClass
{
	/*! The parent class. */
	GtkVBoxClass parent_class;

	/**
	 * \param twitter_status a GtkTwitterStatus instance
	 * \return the guid of the status
	 *
	 * Gets the guid of the status.
	 */
	const gchar *(* get_guid)(GtkTwitterStatus *twitter_status);

	/**
	 * \param twitter_status a GtkTwitterStatus instance
	 * \return the timestamp of the status
	 *
	 * Gets the timestamp of the status.
	 */
	gint (* get_timestamp)(GtkTwitterStatus *twitter_status);
};

/**
 * \return a GtkType
 *
 * Gets the type of the GtkTwitterStatus.
 */
GType gtk_twitter_status_get_type(void);

/**
 * \return a new GtkTwitterStatus widget
 *
 * Creates a new GtkTwitterStatus widget.
 */
GtkWidget * gtk_twitter_status_new();

/*! See _GtkTwitterStatusClass::get_guid() for further information. */
const gchar *gtk_twitter_status_get_guid(GtkTwitterStatus *twitter_status);
/*! See _GtkTwitterStatus::get_timestamp() for further information. */
gint gtk_twitter_status_get_timestamp(GtkTwitterStatus *twitter_status);

/**
 * @}
 * @}
 */
#endif


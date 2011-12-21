/***************************************************************************
    begin........: January 2011
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
 * \file listener.h
 * \brief A listener allow only one instance of the program.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 1. February 2011
 */

#include <glib.h>

#ifndef __LISTENER_H__
#define __LISTENER_H__

/**
 * @addtogroup Core
 * @{
 */

/*! Listener initialization exit codes. */
typedef enum
{
	/*! Initialization failed. */
	LISTENER_RESULT_ABORT,
	/*! An instance is already running. */
	LISTENER_RESULT_FOUND_INSTANCE,
	/*! Listener has been started successfully. */
	LISTENER_RESULT_OK
} ListenerInitResult;

/*! Message codes. */
typedef enum
{
	/*! A ping. */
	LISTENER_MESSAGE_PING,
	/*! Activate the running instance. */
	LISTENER_MESSAGE_ACTIVATE_INSTANCE
} ListenerMessage;

/*! Type of listener callback functions. */
typedef void (* ListenerCallback)(gint code, const gchar *text, gpointer user_data);

/**
 * \return a ListenerInitResult code
 *
 * Starts the listener.
 */
ListenerInitResult listener_init(void);

/**
 * \param code message code
 * \param text message text
 * \return number of sent bytes or -1 on failure
 *
 * Sends a message to the running listener.
 */
gssize listener_send_message(gint32 code, const gchar *text);

/**
 * \param callback function invoked when the listener receives a message
 * \param user_data user data
 * \param free_func a function to free the user data
 * \return id id of the callback
 *
 * Adds a message handler.
 */
gint listener_add_callback(ListenerCallback callback, gpointer user_data, GFreeFunc free_func);

/**
 * \param id id of the callback to remove
 *
 * Removes a message handler.
 */
void listener_remove_callback(gint id);

/**
 * Shuts down the listener.
 */
void listener_shutdown(void);

/**
 * @}
 */
#endif


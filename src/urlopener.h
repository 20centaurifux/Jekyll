/***************************************************************************
    begin........: June 2010
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
 * \file urlopener.h
 * \brief Opening urls.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 23. June 2010
 */

#ifndef __URLOPENER_H__
#define __URLOPENER_H__

#include <glib.h>

/**
 * @addtogroup Core
 * @{
 */

/*! Typedef macro for the _UrlOpener structure. */
typedef struct _UrlOpener UrlOpener;

/*! Typedef macro for the _UrlOpenerFuncs structure. */
typedef struct _UrlOpenerFuncs UrlOpenerFuncs;

/**
 * \struct _UrlOpener
 * \brief opening urls
 * 
 * Base structure for UrlOpener implementations.
 */
struct _UrlOpener
{
	/*! Pointer to UrlOpener functions. */
	UrlOpenerFuncs *funcs;
};

/**
 * \struct _UrlOpenerFuncs
 * \brief UrlOpener functions.
 */
struct _UrlOpenerFuncs
{
	/*! A function to open URLs. */
	gboolean (* open)(UrlOpener *opener, const gchar *url);
	/*! A function freeing allocated resources. */
	void (* free)(UrlOpener *opener);
};

/**
 * \return a new UrlOpener or NULL on failure
 *
 * Tries to create a default UrlOpener.
 */
UrlOpener *url_opener_new_default(void);

/**
 * \param executable path to an executable file
 * \param arguments optional arguments
 * \return a new UrlOpener
 *
 * Creates an UrlOpener from an executable file.
 */
UrlOpener *url_opener_new_executable(const gchar *executable, const gchar *arguments);

/**
 * \return a new UrlOpener
 *
 * Creates an UrlOpener on windows operating systems.
 */
UrlOpener *url_opener_new_win32(void);

/**
 * \param opener an UrlOpener
 * \param url url to open
 * \return TRUE on success
 *
 * Opens the specified url.
 */
gboolean url_opener_launch(UrlOpener *opener, const gchar *url);

/**
 * \param opener an UrlOpener
 *
 * Frees an UrlOpener structure.
 */
void url_opener_free(UrlOpener *opener);

/**
 * @}
 */
#endif


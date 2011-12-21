/***************************************************************************
    begin........: August 2010
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
 * \file first_sync_dialog.h
 * \brief Dialog shown when doing the initial synchronization.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 2. September 2011
 */
#ifndef __FIRT_SYNC_DIALOG_H__
#define __FIRT_SYNC_DIALOG_H__

#include <gtk/gtk.h>

#include "../configuration.h"

/**
 * \param parent parent window
 * \param username user to synchronize
 * \param access_key OAuth access key
 * \param access_secret OAuth access secret
 * \return TRUE on success
 *
 * Performs the initial synchronization.
 */
gboolean first_sync_dialog_run(GtkWidget *parent, const gchar *username, const gchar *access_key, gchar *access_secret);

#endif

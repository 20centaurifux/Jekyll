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
 * \file about_dialog.h
 * \brief An about dialog.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 5. October 2010
 */

#include <gtk/gtk.h>

#ifndef __ABOUT_H__
#define __ABOUT_H__

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param parent parent window
 * \return a GTK dialog
 *
 * Creates an about dialog.
 */
GtkWidget *about_dialog_create(GtkWidget *parent);

/**
 * @}
 */
#endif


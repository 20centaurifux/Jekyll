/***************************************************************************
    begin........: January 2012
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
 * \file pixbuf_helpers.h
 * \brief PixbufLoader helper functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 17. January 2012
 */

#ifndef __PIXBUF_HELPERS_H__
#define __PIXBUF_HELPERS_H__

#include "gtktwitterstatus.h"
#include "../pixbufloader.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param GdkPixbuf pixbuf to set
 * \param status a GtkTwitterStatus
 *
 * PixbufLoader callback function to set GtkTwitterStatus pixbuf;
 */
void pixbuf_helpers_set_gtktwitterstatus_callback(GdkPixbuf *pixbuf, GtkTwitterStatus *status);

/**
 * @}
 */
#endif



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
 * \file gui.h
 * \brief GUI initialization.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 25. May 2010
 */
#ifndef __GUI_H__
#define __GUI_H__

#include <glib.h>

#include "../configuration.h"
#include "../cache.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param config a Config instance
 * \param cache a Cache instance
 *
 * Starts the GUI.
 */
void gui_start(Config *config, Cache *cache);

/**
 * @}
 */
#endif


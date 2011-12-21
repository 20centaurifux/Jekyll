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
 * \file wizard.h
 * \brief Startup wizard.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 15. May 2010
 */

#ifndef __WIZARD_H__
#define __WIZARD_H__

#include <glib.h>
#include "../configuration.h"

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Wizard
 * 	@{
 */

/**
 * \param config a Config instance for storing configuration data
 * \return TRUE if the wizard has been finished successfully
 *
 * Starts the wizard.
 */
gboolean wizard_start(Config *config);

/**
 * @}
 * @}
 */
#endif


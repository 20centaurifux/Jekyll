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
 * \file helpers.h
 * \brief Helper functions.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 21. October 2010
 */

#ifndef __HELPERS_H__
#define __HELPERS_H__

#include "urlopener.h"
#include "configuration.h"

/**
 * @addtogroup Core 
 * @{
 */

/**
 * \param config configuration data
 * \return a new allocated UrlOpener or NULL on failure
 *
 * Creates an UrlOpener instance from the given configuration data.
 */
UrlOpener *helpers_create_urlopener_from_config(Config *config);

/**
 * @}
 */
#endif


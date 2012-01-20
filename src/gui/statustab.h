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
 * \file statustab.h
 * \brief A tab containing twitter statuses.
 * \author sebastian fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 19. January 2012
 */

#ifndef __STATUS_TAB_H__
#define __STATUS_TAB_H__

#include "tabbar.h"

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param tabbar the tabbar
 * \param identifier id of the status tab
 * \param id id of the tab
 * \return a new status tab
 *
 * Creates a new status tab.
 */
GtkWidget *status_tab_create(GtkWidget *tabbar, TabTypeId identifier, const gchar *id);

/**
 * \param tab the tab page
 * \return a new allocated string or NULL
 *
 * Gets the tab owner.
 */
gchar *status_tab_get_owner(GtkWidget *page);

/**
 * @}
 */
#endif


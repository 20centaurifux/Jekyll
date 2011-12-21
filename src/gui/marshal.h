/***************************************************************************
    begin........: March 2011
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
 * \file marshal.h
 * \brief Closures.
 * \author sebastian fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 08. March 2011
 */

#ifndef __MARSHAL_H__
#define __MARSHAL_H__

#include <glib-object.h>

/**
 * @addtogroup Gui
 * @{
 */

void g_cclosure_marshal_BOOLEAN__OBJECT(GClosure *closure, GValue *return_value, guint n_param_values,
                                        const GValue *param_values, gpointer invocation_hint, gpointer marshal_data);

#endif

/**
 * @}
 */

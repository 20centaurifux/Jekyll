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
 * \file marshal.c
 * \brief Closures.
 * \author sebastian fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 08. March 2011
 */

#include "marshal.h"

/*! Copies a value from the values vector. */
#define g_marshal_value_peek_object(v) g_value_get_object (v)

/**
 * @addtogroup Gui
 * @{
 */

/**
 * \param closure a GClosure
 * \param return_value return value of the closure
 * \param n_param_values number of parameters
 * \param param_values values
 * \param invocation_hint
 * \param marshal_data
 */
void
g_cclosure_marshal_BOOLEAN__OBJECT(GClosure *closure, GValue *return_value, guint n_param_values,
                                   const GValue *param_values, gpointer invocation_hint, gpointer marshal_data)
{
	typedef gboolean(*GMarshalFunc_BOOLEAN__OBJECT)(gpointer data1, gpointer arg_1, gpointer data2);

	register GMarshalFunc_BOOLEAN__OBJECT callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;
	gboolean v_return;

	g_return_if_fail(return_value != NULL);
	g_return_if_fail(n_param_values == 2);

	if(G_CCLOSURE_SWAP_DATA(closure))
	{
		data1 = closure->data;
		data2 = g_value_peek_pointer(param_values + 0);
	}
	else
	{
		data1 = g_value_peek_pointer(param_values + 0);
		data2 = closure->data;
	}

	callback = (GMarshalFunc_BOOLEAN__OBJECT)(marshal_data ? marshal_data : cc->callback);

	v_return = callback(data1, g_marshal_value_peek_object(param_values + 1), data2);

	g_value_set_boolean(return_value, v_return);
}

/**
 * @}
 */


/***************************************************************************
    begin........: April 2009
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
 * \file value.c
 * \brief Implementation of the _Value class.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 10. May 2010
 */

#include "value.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Config
 * 	@{
 */

enum
{
	PROP_0,
	PROP_NAME,
	PROP_DATATYPE
};

static GObjectClass *parent_class = NULL;

static void value_class_init(ValueClass *klass);
static void value_finalize(GObject *obj);
static void _value_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void _value_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static const gchar *_value_get_name(Value *value);
static gint32 _value_get_int32(Value *self);
static void _value_set_int32(Value *self, gint32 value);
static gfloat _value_get_float(Value *self);
static void _value_set_float(Value *self, gfloat value);
static gboolean _value_get_bool(Value *self);
static void _value_set_bool(Value *self, gboolean value);
static const gchar *_value_get_string(Value *self);
static void _value_set_string(Value *self, const gchar *format, va_list ap);
static void _value_set_array(Value *self, gconstpointer, guint len);
static gconstpointer _value_get_array(Value *self, guint *len);

/*
 *	initialize/finalization:
 */
GType
value_get_type(void)
{
	static GType value_type = 0;

	g_type_init();

	if(!value_type)
	{
		static const GTypeInfo value_type_info =
		{
			sizeof(ValueClass),
			NULL,
			NULL,
			(GClassInitFunc)value_class_init,
			NULL,
			NULL,
			sizeof(Value),
			0,
			NULL
		};
		value_type = g_type_register_static(G_TYPE_OBJECT, "Value", &value_type_info, (GTypeFlags)0);
	}

	return value_type;
}

static void
value_class_init(ValueClass *klass)
{
	GObjectClass *object_class;

	parent_class = (GObjectClass *)g_type_class_peek_parent(klass);
	object_class = (GObjectClass *)klass;

	/* overwrite funtions */
	object_class->finalize = &value_finalize;
	object_class->set_property = &_value_set_property;
	object_class->get_property = &_value_get_property;

	/* class methods */
	klass->get_name = &_value_get_name;
	klass->get_int32 = &_value_get_int32;
	klass->set_int32 = &_value_set_int32;
	klass->get_float = &_value_get_float;
	klass->set_float = &_value_set_float;
	klass->get_bool = &_value_get_bool;
	klass->set_bool = &_value_set_bool;
	klass->get_string = &_value_get_string;
	klass->set_string = &_value_set_string;
	klass->get_array = &_value_get_array;
	klass->set_array = &_value_set_array;

	/* install properties */
	g_object_class_install_property(object_class,
		                        PROP_NAME,
		                        g_param_spec_string("name", NULL, NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	
	g_object_class_install_property(object_class,
		                        PROP_DATATYPE,
					g_param_spec_int("datatype", NULL, NULL, 0, 7, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
value_initialize_datatype(Value *value)
{
	/* initialize value */
	switch(value->priv.datatype)
	{
		case VALUE_DATATYPE_INT32:
			value_set_int32(value, 0);
			break;

		case VALUE_DATATYPE_FLOAT:
			value_set_float(value, 0);
			break;

		case VALUE_DATATYPE_BOOLEAN:
			value_set_bool(value, FALSE);
			break;

		case VALUE_DATATYPE_STRING:
			value->priv.value.string_value = g_string_new_len(NULL, 128);
			break;

		case VALUE_DATATYPE_INT32_ARRAY:
		case VALUE_DATATYPE_FLOAT_ARRAY:
		case VALUE_DATATYPE_BOOLEAN_ARRAY:
		case VALUE_DATATYPE_STRING_ARRAY:
			value->priv.value.array_value.data = NULL;
			value->priv.value.array_value.len = 0;
			break;

		default:
			g_log(NULL, G_LOG_LEVEL_ERROR, "Invalid datatype: %d", value->priv.datatype);
	}
}

static void
value_finalize(GObject *obj)
{
	Value *self = (Value *)obj;

	G_OBJECT_CLASS(parent_class)->finalize(obj);

	if(self->priv.name)
	{
		g_free(self->priv.name);
	}

	if(VALUE_IS_STRING(self))
	{
		g_string_free(self->priv.value.string_value, TRUE);
	}
	else
	{
		if(VALUE_IS_ARRAY(self))
		{
			if(VALUE_IS_STRING_ARRAY(self))
			{
				for(guint i = 0; i < self->priv.value.array_value.len; ++i)
				{
					g_free(((gchar **)self->priv.value.array_value.data)[i]);
				}
			}
			g_free(self->priv.value.array_value.data);
		}
	}
}

/*
 *	implementation:
 */
static void
_value_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Value *self = (Value *)object;

	switch(property_id)
	{
		case PROP_NAME:
			if(self->priv.name)
			{
				g_free(self->priv.name);
			}
			self->priv.name = g_value_dup_string(value);
			break;

		case PROP_DATATYPE:
			self->priv.datatype = g_value_get_int(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
_value_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Value *self = (Value *)object;

	switch(property_id)
	{
		case PROP_NAME:
			g_value_set_string(value, self->priv.name);
			break;

		case PROP_DATATYPE:
			g_value_set_int(value, self->priv.datatype);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static const gchar *
_value_get_name(Value *self)
{
	return self->priv.name;
}

static gint32
_value_get_int32(Value *self)
{
	g_return_val_if_fail(VALUE_IS_INT32(self), 0);

	return self->priv.value.int32_value;
}

static void
_value_set_int32(Value *self, gint32 value)
{
	g_return_if_fail(VALUE_IS_INT32(self));

	self->priv.value.int32_value = value;
}

static gfloat
_value_get_float(Value *self)
{
	g_return_val_if_fail(VALUE_IS_FLOAT(self), 0);

	return self->priv.value.float_value;
}

static void
_value_set_float(Value *self, gfloat value)
{
	g_return_if_fail(VALUE_IS_FLOAT(self));

	self->priv.value.float_value = value;
}

static gboolean
_value_get_bool(Value *self)
{
	g_return_val_if_fail(VALUE_IS_BOOLEAN(self), FALSE);

	return self->priv.value.bool_value;
}

static void
_value_set_bool(Value *self, gboolean value)
{
	g_return_if_fail(VALUE_IS_BOOLEAN(self));

	self->priv.value.bool_value = value;
}

static const gchar *
_value_get_string(Value *self)
{
	g_return_val_if_fail(VALUE_IS_STRING(self), NULL);

	return (const gchar *)self->priv.value.string_value->str;
}

static void
_value_set_string(Value *self, const gchar *format, va_list ap)
{
	g_return_if_fail(VALUE_IS_STRING(self));

	g_string_vprintf(self->priv.value.string_value, format, ap);
}

static void
_value_set_array(Value *self, gconstpointer data, guint len)
{
	guint i;
	guint size = 0;
	
	g_return_if_fail(VALUE_IS_ARRAY(self));

	/* free memory allocated for string values */
	if(VALUE_IS_STRING_ARRAY(self))
	{
		for(i = 0; i < self->priv.value.array_value.len; ++i)
		{
			g_free(((gchar **)self->priv.value.array_value.data)[i]);
		}
	}

	/* resize array */
	if(len > self->priv.value.array_value.len)
	{
		/* calculate array element size & reallocate memory */
		switch(self->priv.datatype)
		{
			case VALUE_DATATYPE_INT32_ARRAY:
				size = sizeof(gint32);
				break;
			
			case VALUE_DATATYPE_FLOAT_ARRAY:
				size = sizeof(gfloat);
				break;

			case VALUE_DATATYPE_BOOLEAN_ARRAY:
				size = sizeof(gboolean);
				break;

			case VALUE_DATATYPE_STRING_ARRAY:
				size = sizeof(gchar *);
				break;

			default:
				g_log(NULL, G_LOG_LEVEL_ERROR, "Invalid datatype: %d.", self->priv.datatype);
		}

		if(size)
		{
			self->priv.value.array_value.data = g_realloc(self->priv.value.array_value.data, size * len);
			self->priv.value.array_value.len = len;
		}
	}

	/* copy values */
	for(i = 0; i < len; ++i)
	{
		switch(self->priv.datatype)
		{
			case VALUE_DATATYPE_INT32_ARRAY:
				((gint32 *)self->priv.value.array_value.data)[i] = ((gint32 *)data)[i];
				break;

			case VALUE_DATATYPE_FLOAT_ARRAY:
				((gfloat *)self->priv.value.array_value.data)[i] = ((gfloat *)data)[i];
				break;

			case VALUE_DATATYPE_BOOLEAN_ARRAY:
				((gboolean *)self->priv.value.array_value.data)[i] = ((gboolean *)data)[i];
				break;

			case VALUE_DATATYPE_STRING_ARRAY:
				((gchar **)self->priv.value.array_value.data)[i] = g_strdup(((gchar **)data)[i]);
				break;

			default:
				g_log(NULL, G_LOG_LEVEL_ERROR, "Invalid datatype: %d.", self->priv.datatype);

		}		
	}
}

static gconstpointer
_value_get_array(Value *self, guint *len)
{
	g_return_val_if_fail(VALUE_IS_ARRAY(self), NULL);

	*len = self->priv.value.array_value.len;
	return (gconstpointer)self->priv.value.array_value.data;
}

/*
 *	public:
 */
Value *
value_new(const gchar *name, ValueDatatype datatype)
{
	Value *instance = NULL;

	/* check name & datatype */
	g_return_val_if_fail(name != NULL, NULL);
	g_return_val_if_fail(datatype >= 0 && datatype <= 7, NULL);
		
	/* create instance & initialize data */
	instance = (Value *)g_object_new(VALUE_TYPE, "name", name, "datatype", datatype, NULL);
	value_initialize_datatype(instance);

	return instance;
}

void
value_unref(Value *self)
{
	g_object_unref(self);
}

const gchar *
value_get_name(Value *self)
{
	return VALUE_GET_CLASS(self)->get_name(self);
}

gint32
value_get_int32(Value *self)
{
	return VALUE_GET_CLASS(self)->get_int32(self);
}

void
value_set_int32(Value *self, gint32 value)
{
	VALUE_GET_CLASS(self)->set_int32(self, value);
}

gfloat
value_get_float(Value *self)
{
	return VALUE_GET_CLASS(self)->get_float(self);
}

void
value_set_float(Value *self, gfloat value)
{
	VALUE_GET_CLASS(self)->set_float(self, value);
}

gboolean
value_get_bool(Value *self)
{
	return VALUE_GET_CLASS(self)->get_bool(self);
}

void
value_set_bool(Value *self, gboolean value)
{
	VALUE_GET_CLASS(self)->set_bool(self, value);
}

const gchar *
value_get_string(Value *self)
{
	return VALUE_GET_CLASS(self)->get_string(self);
}


void
value_set_string(Value *self, const gchar *format, ...)
{
	va_list ap;

	va_start(ap, format);
	VALUE_GET_CLASS(self)->set_string(self, format, ap);
	va_end(ap);
}


void
value_set_array(Value *self, gconstpointer data, guint len)
{
	VALUE_GET_CLASS(self)->set_array(self, data, len);
}

gconstpointer
value_get_array(Value *self, guint *len)
{
	return VALUE_GET_CLASS(self)->get_array(self, len);
}

/**
 * @}
 * @}
 */


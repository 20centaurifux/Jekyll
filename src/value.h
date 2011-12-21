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
 * \file value.h
 * \brief Declarations of the _Value class.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 10. May 2010
 */

#ifndef __VALUE_H__
#define __VALUE_H__

#include <glib.h>
#include <glib-object.h>
#include <stdarg.h>

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Config
 * 	@{
 */

/*! Gets GType. */
#define VALUE_TYPE           (value_get_type ())
/*! Cast to Value. */
#define VALUE_OBJECT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VALUE_TYPE, Value))
/*! Get ValueClass from Value. */
#define VALUE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  VALUE_TYPE, ValueClass))

/*! Typedef macro for the Value class structure. */
typedef struct _Value Value;

/*! Typedef macro for the Value instance structure. */
typedef struct _ValueClass ValueClass;

/*!
 * \enum ValueDatatype
 * \brief Supported datatypes.
 */
typedef enum
{
	/*! Integer. */
	VALUE_DATATYPE_INT32,
	/*! Float. */
	VALUE_DATATYPE_FLOAT,
	/*! Boolean. */
	VALUE_DATATYPE_BOOLEAN,
	/*! String. */
	VALUE_DATATYPE_STRING,
	/*! Integer array. */
	VALUE_DATATYPE_INT32_ARRAY,
	/*! Float array. */
	VALUE_DATATYPE_FLOAT_ARRAY,
	/*! Boolean array. */
	VALUE_DATATYPE_BOOLEAN_ARRAY,
	/*! String array. */
	VALUE_DATATYPE_STRING_ARRAY
} ValueDatatype;

/*! Checks if a given Value is an integer. */
#define VALUE_IS_INT32(v)         v->priv.datatype == VALUE_DATATYPE_INT32
/*! Checks in a given Value is a float. */
#define VALUE_IS_FLOAT(v)         v->priv.datatype == VALUE_DATATYPE_FLOAT
/*! Checks if a given value is a boolean. */
#define VALUE_IS_BOOLEAN(v)       v->priv.datatype == VALUE_DATATYPE_BOOLEAN
/*! Checks is a given value is a string. */
#define VALUE_IS_STRING(v)        v->priv.datatype == VALUE_DATATYPE_STRING
/*! Checks if a given value is an integer array. */
#define VALUE_IS_INT32_ARRAY(v)   v->priv.datatype == VALUE_DATATYPE_INT32_ARRAY
/*! Checks in a given Value is a float array. */
#define VALUE_IS_FLOAT_ARRAY(v)   v->priv.datatype == VALUE_DATATYPE_FLOAT_ARRAY
/*! Checks in a given Value is a boolean array. */
#define VALUE_IS_BOOLEAN_ARRAY(v) v->priv.datatype == VALUE_DATATYPE_BOOLEAN
/*! Checks in a given Value is a string array. */
#define VALUE_IS_STRING_ARRAY(v)  v->priv.datatype == VALUE_DATATYPE_STRING_ARRAY
/*! Checks in a given Value is an array. */
#define VALUE_IS_ARRAY(v)         (v->priv.datatype >= VALUE_DATATYPE_INT32_ARRAY && v->priv.datatype <= VALUE_DATATYPE_STRING_ARRAY)

/**
 * \struct _Value
 * \brief A class holding a key and a value.
 *
 * The Value class holds a key and a value.
 */
struct _Value
{
	/*! Base class. */
	GObject parent;
	/**
	 * \struct _value_priv
	 * \brief Holds private _Value data.
	 *
	 * \var priv
	 * \brief private data.
	 */
	struct _value_priv
	{
		/*! Name of the value. */
		gchar *name;
		/*! Datatype of the value. */
		ValueDatatype datatype;
		/**
		 * \union _value
		 * \brief Holds stored value.
		 *
		 * \var value
		 * \brief Stored value.
		 */
		union _value
		{
			/*! An integer value. */
			gint32 int32_value;
			/*! A float value. */
			gfloat float_value;
			/*! A boolean value. */
			gboolean bool_value;
			/*! A string value. */
			GString *string_value;
			/**
			 * \struct _array_value
			 * \brief Holds array values and datatype.
			 *
			 * \var array_value
			 * \brief Array values and datatype.
			 */
			struct _array_value
			{
				/*! Array data. */
				gpointer data;
				/*! Length of the array. */
				guint len;
			} array_value;
		} value;
	} priv;
};

/**
 * \struct _ValueClass
 * \brief The _Value class structure.
 *
 * The _Value class has the following properties:
 * - \b name: name of the value (string, rw)\n
 * - \b datatype: datatype of the value (integer, rw)\n
 */
struct _ValueClass
{
	/*! Parent class. */
	GObjectClass parent_class;

	/**
	 * \param self Value instance
	 * \return the name of the value
	 *
	 * Gets the name of a Value.
	 */
	const gchar *(*get_name)(Value *self);

	/**
	 * \param self Value instance
	 * \return the value
	 */
	gint32 (*get_int32)(Value *self);

	/**
	 * \param self Value instance
	 * \param value value to write
	 */
	void (*set_int32)(Value *self, gint32 value);

	/**
	 * \param self Value instance
	 * \return the value
	 */
	gfloat (*get_float)(Value *self);

	/**
	 * \param self Value instance
	 * \param value value to write
	 */
	void (*set_float)(Value *self, gfloat value);

	/**
	 * \param self Value instance
	 * \return the value
	 */
	gboolean (*get_bool)(Value *self);

	/**
	 * \param self Value instance
	 * \param value value to write
	 */
	void (*set_bool)(Value *self, gboolean value);

	/**
	 * \param self Value instance
	 * \return the value
	 */
	const gchar *(*get_string)(Value *self);

	/**
	 * \param self Value instance
	 * \param value value to write
	 */
	void (*set_string)(Value *self, const gchar *format, va_list ap);

	/**
	 * \param self Value instance
	 * \param data a pointer to the elements of the array
	 * \param len number of elements to copy
	 */
	void (*set_array)(Value *self, gconstpointer data, guint len);

	/**
	 * \param self Value instance
	 * \param len contains the length of the array
	 * \return a pointer to the array
	 */
	gconstpointer (*get_array)(Value *self, guint *len);
};

/*! Registers the type in the type system. */
GType value_get_type(void);

/**
 * \param name name of the value
 * \param datatype datatype of the value
 * \return a Value object or NULL on failure
 */
Value *value_new(const gchar *name, ValueDatatype datatype);

/**
 * \param self Value instance
 */
void value_unref(Value *self);

/*! See _ValueClass::get_name for further information. */
const gchar *value_get_name(Value *self);
/*! See _ValueClass::get_int32 for further information. */
gint32 value_get_int32(Value *self);
/*! See _ValueClass::set_int32 for further information. */
void value_set_int32(Value *self, gint32 value);
/*! See _ValueClass::get_float for further information. */
gfloat value_get_float(Value *self);
/*! See _ValueClass::set_float for further information. */
void value_set_float(Value *self, gfloat value);
/*! See _ValueClass::get_bool for further information. */
gboolean value_get_bool(Value *self);
/*! See _ValueClass::set_bool for further information. */
void value_set_bool(Value *self, gboolean value);
/*! See _ValueClass::get_string for further information. */
const gchar *value_get_string(Value *self);
/*! See _ValueClass::set_string for further information. */
void value_set_string(Value *self, const gchar *format, ...);
/*! See _ValueClass::set_array for further information. */
void value_set_array(Value *self, gconstpointer data, guint len);
/*! See _ValueClass::get_array for further information. */
gconstpointer value_get_array(Value *self, guint *len);

/**
 * @}
 * @}
 */
#endif


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
 * \file section.c
 * \brief Implementation of the _Section class.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 15. October 2010
 */

#include <string.h>

#include "section.h"

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
};

static GObjectClass *parent_class = NULL;

static void section_class_init(SectionClass *klass);
static void section_constructor(Section *section);
static void section_finalize(GObject *obj);
static void section_set_property(GObject *object, guint property_id, const GValue *section, GParamSpec *pspec);
static void section_get_property(GObject *object, guint property_id, GValue *section, GParamSpec *pspec);
static const gchar *_section_get_name(Section *section);
static Value *_section_append_value(Section *self, const gchar *name, ValueDatatype datatype);
static void _section_remove_value(Section *self, Value *value);
static guint _section_n_values(Section *self);
static Value *_section_nth_value(Section *self, guint n);
static void _section_foreach_value(Section *self, ForEachValueFunc func, gpointer user_data);
static Value *_section_find_first_value(Section *self, const gchar *name);
static Section *_section_append_child(Section *self, const gchar *name);
static void _section_remove_child(Section *self, Section *child);
static guint _section_n_children(Section *self);
static Section *_section_nth_child(Section *self, guint n);
static void _section_foreach_child(Section *self, ForEachSectionFunc func, gpointer user_data);
static Section *_section_find_first_child(Section *self, const gchar *name);
static GList *_section_select_children(Section *self, const gchar *path, Section **section);

/*
 *	initialization/finalization:
 */
GType
section_get_type(void)
{
	static GType section_type = 0;

	g_type_init();

	if(!section_type)
	{
		static const GTypeInfo section_type_info =
		{
			sizeof(SectionClass),
			NULL,
			NULL,
			(GClassInitFunc)section_class_init,
			NULL,
			NULL,
			sizeof(Section),
			0,
			(GInstanceInitFunc)section_constructor
		};
		section_type = g_type_register_static(G_TYPE_OBJECT, "Section", &section_type_info, (GTypeFlags)0);
	}

	return section_type;
}

static void
section_class_init(SectionClass *klass)
{
	GObjectClass *object_class;

	parent_class = (GObjectClass *)g_type_class_peek_parent(klass);
	object_class = (GObjectClass *)klass;

	/* overwrite funtions */
	object_class->finalize = &section_finalize;
	object_class->set_property = &section_set_property;
	object_class->get_property = &section_get_property;

	/* class methods */
	klass->get_name = &_section_get_name;
	klass->append_value = &_section_append_value;
	klass->remove_value = &_section_remove_value;
	klass->n_values = &_section_n_values;
	klass->nth_value = &_section_nth_value;
	klass->foreach_value = &_section_foreach_value;
	klass->find_first_value = &_section_find_first_value;
	klass->append_child = &_section_append_child;
	klass->remove_child = &_section_remove_child;
	klass->n_children = &_section_n_children;
	klass->nth_child = &_section_nth_child;
	klass->foreach_child = &_section_foreach_child;
	klass->find_first_child = &_section_find_first_child;
	klass->select_children = &_section_select_children;

	/* install properties */
	g_object_class_install_property(object_class,
		                        PROP_NAME,
		                        g_param_spec_string("name", NULL, NULL, NULL, G_PARAM_READWRITE));
}

static void
section_constructor(Section *section)
{
	section->priv.name = NULL;
	section->priv.values = NULL;
	section->priv.children = NULL;
}

static void
section_finalize(GObject *obj)
{
	Section *self = (Section *)obj;
	GList *item;

	G_OBJECT_CLASS(parent_class)->finalize(obj);

	if(self->priv.name)
	{
		g_free(self->priv.name);
	}

	/* destroy lists */
	item = g_list_first(self->priv.values);
	while(item)
	{
		value_unref((Value *)item->data);
		item = g_list_next(item);
	}
	g_list_free(self->priv.values);

	item = g_list_first(self->priv.children);
	while(item)
	{
		section_unref((Section *)item->data);
		item = g_list_next(item);
	}
	g_list_free(self->priv.children);
}

static void
section_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Section *self = (Section *)object;

	switch(property_id)
	{
		case PROP_NAME:
			if(self->priv.name)
			{
				g_free(self->priv.name);
			}
			self->priv.name = g_value_dup_string(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
section_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Section *self = (Section *)object;

	switch(property_id)
	{
		case PROP_NAME:
			g_value_set_string(value, self->priv.name);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

/*
 *	implementation:
 */
static const gchar *
_section_get_name(Section *self)
{
	return self->priv.name;
}

static Value *
_section_append_value(Section *self, const gchar *name, ValueDatatype datatype)
{
	Value *value = NULL;

	value = value_new(name, datatype);
	self->priv.values = g_list_append(self->priv.values, (gpointer)value);

	return value;
}

static void
_section_remove_value(Section *self, Value *value)
{
	self->priv.values = g_list_remove(self->priv.values, (gpointer)value);
}

static guint
_section_n_values(Section *self)
{
	return g_list_length(self->priv.values);
}

static Value *
_section_nth_value(Section *self, guint n)
{
	g_return_val_if_fail(n < section_n_values(self), NULL);

	return (Value *)g_list_nth_data(self->priv.values, n);
}

static void
_section_foreach_value(Section *self, ForEachValueFunc func, gpointer user_data)
{
	GList *item;

	g_return_if_fail(func != NULL);

	item = g_list_first(self->priv.values);
	while(item)
	{
		func((Value *)item->data, user_data);
		item = g_list_next(item);
	}
}

static Value *
_section_find_first_value(Section *self, const gchar *name)
{
	GList *item = g_list_first(self->priv.values);

	while(item)
	{
		if(!strcmp(name, value_get_name((Value *)item->data)))
		{
			return (Value *)item->data;
		}
		item = g_list_next(item);
	}

	return NULL;
}

static Section *
_section_append_child(Section *self, const gchar *name)
{
	Section *section = NULL;

	section = section_new(name);
	self->priv.children = g_list_append(self->priv.children, (gpointer)section);

	return section;
}

static void
_section_remove_child(Section *self, Section *child)
{
	self->priv.children = g_list_remove(self->priv.children, (gpointer)child);
}

static guint
_section_n_children(Section *self)
{
	return g_list_length(self->priv.children);
}

Section *
_section_nth_child(Section *self, guint n)
{
	g_return_val_if_fail(n < section_n_children(self), NULL);

	return (Section *)g_list_nth_data(self->priv.children, n);
}

static void
_section_foreach_child(Section *self, ForEachSectionFunc func, gpointer user_data)
{
	GList *item;

	g_return_if_fail(func != NULL);

	item = g_list_first(self->priv.children);
	while(item)
	{
		func((Section *)item->data, user_data);
		item = g_list_next(item);
	}
}

static Section *
_section_find_first_child(Section *self, const gchar *name)
{
	GList *item = g_list_first(self->priv.children);

	while(item)
	{
		if(!strcmp(name, section_get_name((Section *)item->data)))
		{
			return (Section *)item->data;
		}
		item = g_list_next(item);
	}

	return NULL;
}

static GList *
_section_select_children(Section *self, const gchar *path, Section **parent)
{
	gchar **vector;
	gint length;
	gint i;
	Section *section = NULL;
	Section *child;
	GList *result = NULL;

	section = self;

	if((vector = g_strsplit(path, "/", -1)))
	{
		length = g_strv_length(vector);

		for(i = 0; i < length - 1 && section; ++i)
		{
			if(strlen(vector[i]))
			{
				section = section_find_first_child(section, vector[i]);

			}
		}

		/* create list */
		if(section)
		{
			for(i = 0; i < section_n_children(section); ++i)
			{
				child = section_nth_child(section, i);

				if(!strcmp(section_get_name(child), vector[length - 1]))
				{
					result = g_list_append(result, child);
				}
			}
		}

		if(parent)
		{
			*parent = section;
		}

		/* free memory */
		g_strfreev(vector);
	}

	return result;
}

/*
 *	public:
 */
Section *
section_new(const gchar *name)
{
	g_return_val_if_fail(name != NULL, NULL);

	return (Section *)g_object_new(SECTION_TYPE, "name", name, NULL);
}

void
section_unref(Section *self)
{
	g_object_unref(self);
}

const gchar *
section_get_name(Section *self)
{
	return SECTION_GET_CLASS(self)->get_name(self);
}

Value *
section_append_value(Section *self, const gchar *name, ValueDatatype datatype)
{
	return SECTION_GET_CLASS(self)->append_value(self, name, datatype);
}

void
section_remove_value(Section *self, Value *value)
{
	SECTION_GET_CLASS(self)->remove_value(self, value);
}

guint
section_n_values(Section *self)
{
	return SECTION_GET_CLASS(self)->n_values(self);
}

Value *
section_nth_value(Section *self, guint n)
{
	return SECTION_GET_CLASS(self)->nth_value(self, n);
}

void
section_foreach_value(Section *self, ForEachValueFunc func, gpointer user_data)
{
	SECTION_GET_CLASS(self)->foreach_value(self, func, user_data);
}

Value *
section_find_first_value(Section *self, const gchar *name)
{
	return SECTION_GET_CLASS(self)->find_first_value(self, name);
}

Section *
section_append_child(Section *self, const gchar *name)
{
	return SECTION_GET_CLASS(self)->append_child(self, name);
}

void
section_remove_child(Section *self, Section *child)
{
	SECTION_GET_CLASS(self)->remove_child(self, child);
}

guint
section_n_children(Section *self)
{
	return SECTION_GET_CLASS(self)->n_children(self);
}

Section *
section_nth_child(Section *self, guint n)
{
	return SECTION_GET_CLASS(self)->nth_child(self, n);
}

void
section_foreach_child(Section *self, ForEachSectionFunc func, gpointer user_data)
{
	return SECTION_GET_CLASS(self)->foreach_child(self, func, user_data);
}

Section *
section_find_first_child(Section *self, const gchar *name)
{
	return SECTION_GET_CLASS(self)->find_first_child(self, name);
}

GList *
section_select_children(Section *self, const gchar *name, Section **section)
{
	return SECTION_GET_CLASS(self)->select_children(self, name, section);
}

/**
 * @}
 * @}
 */


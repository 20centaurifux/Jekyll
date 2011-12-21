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
 * \file section.h
 * \brief Declarations of the _Section class.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 15. October 2010
 */

#ifndef __SECTION_H__
#define __SECTION_H__

#include <glib.h>
#include <glib-object.h>

#include "value.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Config
 * 	@{
 */

/*! Get GType. */
#define SECTION_TYPE           (section_get_type ())
/*! Check if instance is Section. */
#define SECTION_OBJECT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SECTION_TYPE, Section))
/*! Get SectionClass from Section. */
#define SECTION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  SECTION_TYPE, SectionClass))

/*! Typedef macro for the Section class structure. */
typedef struct _Section Section;

/*! Typedef macro for the Section instance structure. */
typedef struct _SectionClass SectionClass;

/*! Specifies the type of the function which is passed to _Section::foreach_value(). */
typedef void (*ForEachValueFunc)(Value *value, gpointer user_data);
/*! Specifies the type of the function which is passed to _Section::foreach_child(). */
typedef void (*ForEachSectionFunc)(Section *section, gpointer user_data);

/**
 * \struct _Section
 * \brief A class holding values and sections.
 *
 * The _Section class holds values and subsections.
 */
struct _Section
{
	/*! Base class. */
	GObject parent;
	/**
	 * \struct _section_priv
	 * \brief Holds private _Section data.
	 *
	 * \var priv
	 * \brief Private data.
	 */
	struct _section_priv
	{
		/*! Name of the section. */
		gchar *name;
		/*! A list holding values. */
		GList *values;
		/*! A list holding subsections. */
		GList *children;
	} priv;
};

/**
 * \struct _SectionClass
 * \brief The _Section class structure.
 *
 * The _Section class has the following properties:
 * - \b name: name of the section (string, rw)\n
 */
struct _SectionClass
{
	/*! Parent class. */
	GObjectClass parent_class;

	/**
	 * \param self Section instance
	 * \return the name of the section
	 *
	 * Gets the name of the section.
	 */
	const gchar *(*get_name)(Section *self);

	/**
	 * \param self Section instance
	 * \param name name of the value to append
	 * \param datatype type of the value to append
	 * \return a new Value object or NULL on failure
	 *
	 * Appends a new value to the section.
	 */
	Value *(*append_value)(Section *self, const gchar *name, ValueDatatype datatype);

	/**
	 * \param self Section instance
	 * \param value Value to remove from the section
	 *
	 * Removes a value from the section.
	 */
	void (*remove_value)(Section *self, Value *value);

	/**
	 * \param self Section instance
	 * \return number of stored values
	 *
	 * Gets the number of child values of a section.
	 */
	guint (*n_values)(Section *self);

	/**
	 * \param self Section instance
	 * \param n the position of the value
	 * \return the Value at the given position
	 *
	 * Returns the value at the given position.
	 */
	Value *(*nth_value)(Section *self, guint n);

	/**
	 * \param self Section instance
	 * \param func function called for each Value of the section
	 * \param user_data user data
	 *
	 * Iterates the values of a section and calls the specified callback function for each element.
	 */
	void (*foreach_value)(Section *self, ForEachValueFunc func, gpointer user_data);

	/**
	 * \param self Section instance
	 * \param name name of the value to search for
	 * \return a pointer to the found value or NULL if it cannot be found
	 *
	 * Searches for the first value with the given name.
	 */
	Value *(*find_first_value)(Section *self, const gchar *name);

	/**
	 * \param self Section instance
	 * \param name name of the child to append
	 * \return a new Section object or NULL on failure
	 *
	 * Appends a new child to a section.
	 */
	Section *(*append_child)(Section *self, const gchar *name);

	/**
	 * \param self Section instance
	 * \param child child to remove from the section
	 *
	 * Removes a child from a section.
	 */
	void (*remove_child)(Section *self, Section *child);

	/**
	 * \param self Section instance
	 * \return number of stored children
	 *
	 * Gets the number of children of the section.
	 */
	guint (*n_children)(Section *self);

	/**
	 * \param self Section instance
	 * \param n the position of the section
	 * \return the Section at the given position
	 *
	 * Returns the section at the given position.
	 */
	Section *(*nth_child)(Section *self, guint n);

	/**
	 * \param self Section instance
	 * \param func function called for each child of the section
	 * \param user_data user data
	 *
	 * Iterates the children of the section and calls the specified callback function for each element.
	 */
	void (*foreach_child)(Section *self, ForEachSectionFunc func, gpointer user_data);

	/**
	 * \param self Section instance
	 * \param name name of the section to search for
	 * \return a pointer to the found section or NULL if it cannot be found
	 *
	 * Searches for the first section with the given name.
	 */
	Section *(*find_first_child)(Section *self, const gchar *name);

	/**
	 * \param self Section instance
	 * \param path path to find
	 * \param parent holds the parent of the found children
	 * \return a list containing sections
	 *
	 * Selects all sections with the specified path.
	 */
	GList *(*select_children)(Section *self, const gchar *path, Section **parent);
};

/*! Registers the type in the type system. */
GType section_get_type(void);

/**
 * \param name name of the section
 * \return a Section object or NULL on failure
 */
Section *section_new(const gchar *name);

/**
 * \param self a Section instance
 */
void section_unref(Section *self);

/*! See _SectionClass::get_name for further information. */
const gchar *section_get_name(Section *self);
/*! See _SectionClass::append_value for further information. */
Value *section_append_value(Section *self, const gchar *name, ValueDatatype datatype);
/*! See _SectionClass::remove_value for further information. */
void section_remove_value(Section *self, Value *value);
/*! See _SectionClass::n_values for further information. */
guint section_n_values(Section *self);
/*! See _SectionClass::get_values for further information. */
Value *section_nth_value(Section *self, guint n);
/*! See _SectionClass::foreach_value for further information. */
void section_foreach_value(Section *self, ForEachValueFunc func, gpointer user_data);
/*! See _SectionClass::find_first_value for further information. */
Value *section_find_first_value(Section *self, const gchar *name);
/*! See _SectionClass::append_child for further information. */
Section *section_append_child(Section *self, const gchar *name);
/*! See _SectionClass::remove_child for further information. */
void section_remove_child(Section *self, Section *child);
/*! See _SectionClass::n_children for further information. */
guint section_n_children(Section *self);
/*! See _SectionClass::get_children for further information. */
Section *section_nth_child(Section *self, guint n);
/*! See _SectionClass::foreach_child for further information. */
void section_foreach_child(Section *self, ForEachSectionFunc func, gpointer user_data);
/*! See _SectionClass::find_first_child for further information. */
Section *section_find_first_child(Section *self, const gchar *name);
/*! See _SectionClass::find_child_by_path for further information. */
GList *section_select_children(Section *self, const gchar *name, Section **parent);

/**
 * @}
 * @}
 */
#endif


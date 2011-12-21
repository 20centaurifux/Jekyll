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
 * \file configuration.h
 * \brief Managing configuration data.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 10. May 2010
 */

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <glib.h>
#include <glib-object.h>

#include "section.h"
#include "value.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Config
 * 	@{
 */

/*! Get GType. */
#define CONFIG_TYPE           (config_get_type ())
/*! Cast to Config. */
#define CONFIG_OBJECT(obj)    (G_TYPE_CHECK_INSTANCE_CAST((obj), CONFIG_TYPE, Config))
/*! Get ConfigClass from Config instance. */
#define CONFIG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  CONFIG_TYPE, ConfigClass))

/*! Typedef macro for the Config class structure. */
typedef struct _Config Config;

/*! Typedef macro for the Config instance structure. */
typedef struct _ConfigClass ConfigClass;

/**
 * \struct _Config
 * \brief Managing configuration data.
 *
 * The Config class provides access to configuration data.
 */
struct _Config
{
	/*! Base class. */
	GObject parent;
	/**
	 * \struct _config_priv
	 * \brief Holds private data of the _Config class.
	 *
	 * \var priv
	 * \brief Private _Config data.
	 */
	struct _config_priv
	{
		/*! Path of the configuration file. */
		gchar *filename;
		/*! The root section. */
		Section *root;
	} priv;
};

/**
 * \struct _ConfigClass
 * \brief The _Config class structure.
 *
 * The _Config class has the following properties:
 * - \b filename: name of the configuration file (string, rw)\n
 * - \b root: the root section (_Section, ro)\n
 */
struct _ConfigClass
{
	/*! Parent class. */
	GObjectClass parent_class;

	/**
	 * \param config Config instance
	 * \return a pointer to the root section
	 *
	 * Gets the root section.
	 */
	Section *(*get_root)(Config *self);

	/**
	 * \param config Config instance
	 * \param filename file to load
	 * \param err structure for storing failure messages
	 * \return TRUE if the file could be loaded successfully
	 *
	 * Loads configuration data from a file.
	 */
	gboolean (*load)(Config *self, const gchar *filename, GError **err);
	
	/**
	 * \param config Config instance
	 * \param err structure for storing failure messages
	 * \return TRUE if the file could be saved successfully
	 *
	 * Saves configuration data to the current file.
	 */
	gboolean (*save)(Config *self, GError **err);
	
	/**
	 * \param config Config instance
	 * \param filename name of the file which will hold the configuration data  
	 * \param err structure for storing failure messages
	 * \return TRUE if the file could be saved successfully
	 *
	 * Saves configuration data to the specified file.
	 */
	gboolean (*save_as)(Config *self, const gchar *filename, GError **err);
	
	/**
	 * \param config Config instance
	 * \return a pointer to a string holding the current filename
	 * 
	 * Gets the current filename.
	 */
	const gchar *(*get_filename)(Config *self);
};

/*! Registers the type in the type system. */
GType config_get_type(void);

/**
 * \return a Config object or NULL on failure
 */
Config *config_new(void);

/**
 * \param self Config instance
 */
void config_unref(Config *self);

/*! See _ConfigClass::get_root for further information. */
Section *config_get_root(Config *self);
/*! See _ConfigClass::load for further information. */
gboolean config_load(Config *self, const gchar *filename, GError **err);
/*! See _ConfigClass::save for further information. */
gboolean config_save(Config *self, GError **err);
/*! See _ConfigClass::save_as for further information. */
gboolean config_save_as(Config *self, const gchar *filename, GError **err);
/*! See _ConfigClass::get_filename for further information. */
const gchar *config_get_filename(Config *self);

/**
 * @}
 * @}
 */
#endif


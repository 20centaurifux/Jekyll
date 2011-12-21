/***************************************************************************
    begin........: April 2010
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
 * \file cache.h
 * \brief Threadsafe caching.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 21. May 2010
 */

#ifndef __CACHE_H__
#define __CACHE_H__

#include <glib-object.h>

/**
 * @addtogroup Core
 * @{
 */

/*! Get GType. */
#define CACHE_TYPE            cache_get_type()
/*! Cast to Cache. */
#define CACHE(inst)           (G_TYPE_CHECK_INSTANCE_CAST((inst), CACHE_TYPE, Cache))
/*! Cast to CacheClass. */
#define CACHE_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST((class), CACHE_TYPE, CacheClass))
/*! Check if instance is Cache. */
#define IS_CACHE(inst)        (G_TYPE_CHECK_INSTANCE_TYPE((inst), CACHETYPE))
/*! Check if class is CacheClass. */
#define IS_CACHE_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE((class), CACHETYPE))
/*! Get CacheClass from cache instance. */
#define CACHE_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS((inst), CACHE_TYPE, CacheClass))

/*! The default cache limit. */
#define DEFAULT_CACHE_LIMIT   4096
/*! The minimum cache size. */
#define MINIMUM_CACHE_LIMIT   10

/*! A type definition for _CachePrivate. */
typedef struct _CachePrivate CachePrivate;

/*! A type definition for _CacheClass. */
typedef struct _CacheClass CacheClass;

/*! A type definition for _Cache. */
typedef struct _Cache Cache;

/*! Defines an infinite lifetime. */
#define CACHE_INFINITE_LIFETIME 0

/**
 * \struct _CacheClass
 * \brief The _Cache class structure.
 *
 * The Cache class structure.
 * It has the following properties:
 * - \b cache-limit: Maximum size of the cache. (integer, rw)
 * - \b enable-swap: Enables swapping. (boolean, rw)
 * - \b swap-directory: Locaton for swap files. (string, rw)
 */
struct _CacheClass
{
	/*! The parent class. */
	GObjectClass parent_class;

	/**
	 * \param cache Cache instance
	 * \return TRUE on success
	 *
	 * Initializes the swap directory.
	 */
	gboolean (* initialize_swap_folder)(Cache *cache);

	/**
	 * \param cache Cache instance
	 * \return TRUE on success
	 *
	 * Clears the swap directory.
	 */
	gboolean (* clear_swap_folder)(Cache *cache);

	/**
	 * \param cache Cache instance
	 * \param key key assigned to the item to save
	 * \param data data to save
	 * \param size size of the data
	 * \param lifetime lifetime of the cache item
	 * \return TRUE on success
	 *
	 * Stores data in the cache.
	 */
	gboolean (* save)(Cache *cache, const gchar * restrict key, const gchar * restrict data, gint size, gint lifetime);

	/**
	 * \param cache Cache instance
	 * \param key key assigned to the item to load
	 * \param data pointer to a location to store the requested data
	 * \return length of the loaded data
	 *
	 * Loads data from the cache.
	 */
	gint (* load)(Cache *cache, const gchar * restrict key, gchar ** restrict data);

	/**
	 * \param cache Cache instance
	 * \param key key assigned to the item to remove
	 *
	 * Removes an item from the cache.
	 */
	void (* remove)(Cache *cache, const gchar *key);
};

/**
 * \struct _Cache
 * \brief Threadsafe caching.
 *
 * Cache provides threadsafe caching. See _CacheClass for more details.
 */
struct _Cache
{
	/*! The parent instance. */
	GObject parent_instance;

	/*! Private data. */
	CachePrivate *priv;
};

/*! See _CacheClass::initialize_swap_folder for further information. */
gboolean cache_initialize_swap_folder(Cache *cache);
/*! See _CacheClass::clear_swap_folder for further information. */
gboolean cache_clear_swap_folder(Cache *cache);
/*! See _CacheClass::save for further information. */
gboolean cache_save(Cache *cache, const gchar * restrict key, const gchar * restrict data, gint size, gint lifetime);
/*! See _CacheClass::load for further information. */
gint cache_load(Cache *cache, const gchar * restrict key, gchar ** restrict data);
/*! See _CacheClass::remove for further information. */
void cache_remove(Cache *cache, const gchar *key);

/**
 * \return a GType
 *
 * Registers the type in the type system.
 */
GType cache_get_type (void)G_GNUC_CONST;

/**
 * \param cache_limit maximum cache size
 * \param enable_swap enable swapping
 * \param swap_directory location for swap files
 * \return a new Cache instance.
 *
 * Creates a new Cache instance.
 */
Cache *cache_new(gint cache_limit, gboolean enable_swap, const gchar *swap_directory);

/**
 * @}
 */
#endif


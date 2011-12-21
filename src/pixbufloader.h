/***************************************************************************
    begin........: February 2011
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
 * \file pixbufloader.h
 * \brief An asynchronous pixbuf loader.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 10. February 2011
 */

#ifndef __PIXBUF_LOADER_H__
#define __PIXBUF_LOADER_H__

#include <glib-object.h>
#include <gdk/gdk.h>

/**
 * @addtogroup Core
 * @{
 */

/*! Gets GType. */
#define PIXBUF_LOADER_TYPE              pixbuf_loader_get_type()
/*! Cast to PixbufLoader. */
#define PIXBUF_LOADER(inst)             (G_TYPE_CHECK_INSTANCE_CAST((inst), PIXBUF_LOADER_TYPE, PixbufLoader))
/*! Cast to PixbufLoaderClass. */
#define PIXBUF_LOADER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST((class), PIXBUF_LOADER_TYPE, PixbufLoaderClass))
/*! Check if instance is PixbufLoader. */
#define IS_PIXBUF_LOADER(inst)          (G_TYPE_CHECK_INSTANCE_TYPE((inst), PIXBUF_LOADERTYPE))
/*! Check if class is PixbufLoaderClass.  */
#define IS_PIXBUF_LOADER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE((class), PIXBUF_LOADERTYPE))
/*! Get PixbufLoaderClass from PixbufLoader. */
#define PIXBUF_LOADER_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS((inst), PIXBUF_LOADER_TYPE, PixbufLoaderClass))

/*! A type definition for _PixbufLoaderPrivate. */
typedef struct _PixbufLoaderPrivate PixbufLoaderPrivate;

/*! A type definition for _PixbufLoaderClass. */
typedef struct _PixbufLoaderClass PixbufLoaderClass;

/*! A type definition for _PixbufLoader. */
typedef struct _PixbufLoader PixbufLoader;

/**
 * \typedef PixbufLoaderCallback
 * \brief A pixbuf handler function.
 */
typedef void (*PixbufLoaderCallback)(GdkPixbuf *pixbuf, gpointer data);

/**
 * \struct _PixbufLoaderClass
 * \brief The _PixbufLoader class structure.
 *
 * The PixbufLoader class structure.
 * It has the following properties:
 * - \b running:  (boolean, ro)\n
 * - \b cache-dir:  (string, rw)
 */
struct _PixbufLoaderClass
{
	/*! The parent class. */
	GObjectClass parent_class;

	/**
	 * \param pixbufloader PixbufLoader instance
	 * \param url Url of an image
	 * \param callback a pixbuf handler
	 * \param group name of the callback group or NULL
	 * \param user_data user data
	 * \param free_func function to free the specified user data
	 * \return unique id of the callback
	 *
	 * Loads a pixbuf from a website and invokes the given callback function on success.
	 */
	guint (* add_callback)(PixbufLoader *pixbufloader, const gchar *url, PixbufLoaderCallback callback, const gchar *group, gpointer user_data, GFreeFunc free_func);

	/**
	 * \param pixbufloader PixbufLoader instance
	 * \param id id of the callback to remove
	 *
	 * Removes a registered callback function.
	 */
	void (* remove_callback)(PixbufLoader *pixbufloader, guint id);

	/**
	 * \param pixbufloader PixbufLoader instance
	 * \param group name of the group to remove
	 *
	 * Removes a complete callback group.
	 */
	void (* remove_group)(PixbufLoader *pixbufloader, const gchar *group);

	/**
	 * \param pixbufloader PixbufLoader instance
	 *
	 * Starts the PixbufLoader.
	 */
	void (* start)(PixbufLoader *pixbufloader);

	/**
	 * \param pixbufloader PixbufLoader instance
	 *
	 * Stops the PixbufLoader.
	 */
	void (* stop)(PixbufLoader *pixbufloader);
};

/**
 * \struct _PixbufLoader
 * \brief An asynchronous pixbuf loader.
 *
 * Asynchronous pixbuf loader. See _PixbufLoaderClass for more details.
 */
struct _PixbufLoader
{
	/*! The parent instance. */
	GObject parent_instance;

	/*! Private data. */
	PixbufLoaderPrivate *priv;
};

/*! See _PixbufLoaderClass::add_callback for further information. */
guint pixbuf_loader_add_callback(PixbufLoader *pixbufloader, const gchar *url, PixbufLoaderCallback callback, const gchar *group, gpointer user_data, GFreeFunc free_func);
/*! See _PixbufLoaderClass::remove_callback for further information. */
void pixbuf_loader_remove_callback(PixbufLoader *pixbufloader, guint id);
/*! See _PixbufLoaderClass::remove_group for further information. */
void pixbuf_loader_remove_group(PixbufLoader *pixbufloader, const gchar *group);
/*! See _PixbufLoaderClass::start for further information. */
void pixbuf_loader_start(PixbufLoader *pixbufloader);
/*! See _PixbufLoaderClass::stop for further information. */
void pixbuf_loader_stop(PixbufLoader *pixbufloader);

/**
 * \return a GType
 *
 * Registers the type in the type system.
 */
GType pixbuf_loader_get_type (void)G_GNUC_CONST;

/**
 * \return a new PixbufLoader instance.
 *
 * Creates a new PixbufLoader instance.
 */
PixbufLoader *pixbuf_loader_new(const gchar *cache_dir);

/**
 * @}
 */
#endif


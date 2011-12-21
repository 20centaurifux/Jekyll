/***************************************************************************
    begin........: January 2011
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
 * \file gtklinklabel.h
 * \brief A GtkLabel behaving like a HTML link.
 * \author sebastian fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. February 2011
 */

#ifndef __LINK_LABEL_H__
#define __LINK_LABEL_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Widgets
 *	@{
 */

/*! Get GType. */
#define GTK_TYPE_LINK_LABEL            (gtk_link_label_get_type())
/*! Cast to GtkLinkLabel. */
#define GTK_LINK_LABEL(obj)            G_TYPE_CHECK_INSTANCE_CAST(obj, gtk_link_label_get_type(), GtkLinkLabel)
/*! Cast to GtkLinkLabelClass. */
#define GTK_LINK_LABEL_CLASS(klass)    G_TYPE_CHECK_CLASS_CAST(klass, gtk_link_label_get_type(), GtkLinkLabelClass)
/*! Check of instance is GtkLinkLabel. */
#define GTK_IS_LINK_LABEL(obj)         G_TYPE_CHECK_INSTANCE_TYPE(obj, gtk_link_label_get_type())
/*! Get GtkLinkLabelClass from GtkLinkLabel instance. */
#define GTK_LINK_LABEL_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS((inst), GTK_, GtkLinkLabelClass))

/*! A type definition for _GtkLinkLabelPrivate*/
typedef struct _GtkLinkLabelPrivate GtkLinkLabelPrivate;

/*! A type definition for _GtkLinkLabel*/
typedef struct _GtkLinkLabel GtkLinkLabel;

/*! A type definition for _GtkLinkLabelClass*/
typedef struct _GtkLinkLabelClass GtkLinkLabelClass;

/**
 * \struct _GtkLinkLabel
 * \brief A widget behaving like a HTML link.
 */
struct _GtkLinkLabel
{
	/*! The parent instance. */
	GtkEventBox parent_object;
	/*! Private data. */
	GtkLinkLabelPrivate *priv;
};

/**
 * \struct _GtkLinkLabelClass
 * \brief The _GtkLinkLabel class structure.
 *
 * A widget bahaving like a HTML link.
 *
 * The class has the following properties:
 * - \b label: the text of the label (String, rw)\n
 * - \b xmargin: the amount of space added to left and right of the inner label widget (integer, rw)\n
 * - \b ymargin: the amount of space added to top and bottom of the inner label widget (integer, rw)\n
 *
 * The class has the following signals:
 * - \b clicked: user_function(GtkLinkLabel *button, gpointer user_data)
 */
struct _GtkLinkLabelClass
{
	/*! The parent class. */
	GtkEventBoxClass parent_class;

	/**
	 * \param link_label a GtkLinkLabel instance
	 * \return the text of the label
	 *
	 * Gets the text of the label.
	 */
	const gchar *(*get_text)(GtkLinkLabel *link_label);

	/**
	 * \param link_label a GtkLinkLabel instance
	 * \param text text to set
	 *
	 * Sets the text of the label.
	 */
	void (*set_text)(GtkLinkLabel *link_label, const gchar *text);

	/**
	 * \param link_label a GtkLinkLabel instance
	 * \param attrs a PangoAttrList
	 *
	 * Sets a PangoAttrList.
	 */
	void (*set_attributes)(GtkLinkLabel *link_label, PangoAttrList *attrs);

	/**
	 * \param link_label a GtkLinkLabel instance
	 * \return the assigned attribute list, or NULL if none was set
	 *
	 * Gets the attribute list that was set on the label using gtk_link_label_set_attributes().
	 */
	PangoAttrList *(*get_attributes)(GtkLinkLabel *link_label);
};

/**
 * \return a GtkType
 *
 * Gets the type of the GtkLinkLabel.
 */
GType gtk_link_label_get_type(void);

/**
 * \param text text of the label
 * \return a new GtkLinkLabel widget
 *
 * Creates a new GtkLinkLabel widget.
 */
GtkWidget *gtk_link_label_new(const gchar *text);

/*! See _GtkLinkLabelClass::get_text() for further information. */
const gchar *gtk_link_label_get_text(GtkLinkLabel *link_label);
/*! See _GtkLinkLabelClass::set_text() for further information. */
void gtk_link_label_set_text(GtkLinkLabel *link_label, const gchar *text);
/*! See _GtkLinkLabelClass::set_attributes() for further information. */
void gtk_link_label_set_attributes(GtkLinkLabel *link_label, PangoAttrList *attrs);
/*! See _GtkLinkLabelClass::get_attributes() for further information. */
PangoAttrList *gtk_link_label_get_attributes(GtkLinkLabel *link_label);

/**
 * @}
 * @}
 */
#endif


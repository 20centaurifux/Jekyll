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
 * \file gtklinklabel.c
 * \brief A GtkLabel behaving like a HTML link.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 20. September 2011
 */

#include <gdk/gdk.h>
#include <string.h>

#include "gtklinklabel.h"

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Widgets
 * 	@{
 */

static void gtk_link_label_class_init(GtkLinkLabelClass *klass);
static void gtk_link_label_init(GtkLinkLabel *cpu);
static GObject *gtk_link_label_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params);
static void gtk_link_label_finalize(GObject *object);
static void gtk_link_label_set_property(GObject *object, guint property_id, const GValue *config, GParamSpec *pspec);
static void gtk_link_label_get_property(GObject *object, guint property_id, GValue *config, GParamSpec *pspec);

enum
{
	PROP_0,
	PROP_LABEL,
	PROP_XMARGIN,
	PROP_YMARGIN
};

/**
 * \struct _GtkLinkLabelPrivate
 * \brief private _GtkLinkLabel data
 */
struct _GtkLinkLabelPrivate
{
	/*! A widget displaying the text. */
	GtkWidget *label;
	/*! Space added to left and right of inner label. */
	gint xmargin;
	/*! Space added to top and bottom of inner label. */
	gint ymargin;
};

/*! Define GtkLinkLabel. */
G_DEFINE_TYPE(GtkLinkLabel, gtk_link_label, GTK_TYPE_EVENT_BOX);

/*
 *	event handler:
 */
static void
_gtk_link_label_mouseover(GtkWidget *widget, gboolean enter)
{
	GtkWidget *label;
	PangoAttrList *attrs;
	PangoAttrIterator *iter;
	PangoAttrInt *attr = NULL;
	gboolean unref = FALSE;
	GdkCursor *cursor = NULL;

	/* get label, parent & assigned pango attributes */
	label = gtk_bin_get_child(GTK_BIN(widget));

	if(!(attrs = gtk_label_get_attributes(GTK_LABEL(label))))
	{
		/* no attributes assigned => create initial list */
		attrs = pango_attr_list_new();
		unref = TRUE;
	}

	/* find underline attribute */
	iter = pango_attr_list_get_iterator(attrs);
	attr = (PangoAttrInt *)pango_attr_iterator_get(iter, PANGO_ATTR_UNDERLINE);
	pango_attr_iterator_destroy(iter);

	/* update text attribute */
	if(enter)
	{
		if(attr)
		{
			/* underline attribute exists => store current value & update attribute */
			g_object_set_data(G_OBJECT(label), "pango-attr-underline", GINT_TO_POINTER(attr->value));
			attr->value = PANGO_UNDERLINE_SINGLE;
		}
		else
		{
			/* underline attribute doesn't exist => create new attribute & add it to list */
			g_object_set_data(G_OBJECT(label), "pango-attr-underline", GINT_TO_POINTER(PANGO_UNDERLINE_NONE));
			pango_attr_list_insert(attrs, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
		}

		/* create cursor */
		cursor = gdk_cursor_new(GDK_HAND1);
	}
	else
	{
		if(attr)
		{
			/* restore old underline value */
			attr->value = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(label), "pango-attr-underline"));
		}

		/* create cursor */
		cursor = gdk_cursor_new(GDK_LEFT_PTR);
	}

	/* set attribute list of the label */
	gtk_label_set_attributes(GTK_LABEL(label), attrs);

	/* remove attribute list reference (if necessary) */
	if(unref)
	{
		pango_attr_list_unref(attrs);
	}

	/* update cursor */
	if(cursor)
	{
		gdk_window_set_cursor(widget->window, cursor);
		gdk_cursor_unref(cursor);
	}
}

static gboolean
_gtk_link_label_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	g_assert(GTK_IS_EVENT_BOX(widget));

	/* check if mouse is entering/leaving the widget */
	if((event->crossing.type == GDK_ENTER_NOTIFY || event->crossing.type == GDK_LEAVE_NOTIFY) && ((GdkEventCrossing *)event)->mode == GDK_CROSSING_NORMAL)
	{
		if(event->crossing.type == GDK_ENTER_NOTIFY)
		{
			_gtk_link_label_mouseover(widget, TRUE);
		}
		else if(event->crossing.type == GDK_LEAVE_NOTIFY)
		{
			_gtk_link_label_mouseover(widget, FALSE);
		}
	}
	else if(event->type == GDK_BUTTON_PRESS && ((GdkEventButton *)event)->button == 1)
	{
		/* emit click signal */
		_gtk_link_label_mouseover(widget, FALSE);
		g_signal_emit_by_name(widget, "clicked");
	}
	else if(event->type == GDK_LEAVE_NOTIFY)
	{
		_gtk_link_label_mouseover(widget, FALSE);
	}

	return FALSE;
}

/*
 *	implementation:
 */
static const gchar *
_gtk_link_label_get_text(GtkLinkLabel *link_label)
{
	return gtk_label_get_text(GTK_LABEL(link_label->priv->label));
}

static void
_gtk_link_label_set_text(GtkLinkLabel *link_label, const gchar *text)
{
	gtk_label_set_text(GTK_LABEL(link_label->priv->label), text);
}

static void
_gtk_link_label_set_attributes(GtkLinkLabel *link_label, PangoAttrList *attrs)
{
	gtk_label_set_attributes(GTK_LABEL(link_label->priv->label), attrs);
}

static PangoAttrList *
_gtk_link_label_get_attributes(GtkLinkLabel *link_label)
{
	return gtk_label_get_attributes(GTK_LABEL(link_label->priv->label));
}

static void
gtk_link_label_class_init(GtkLinkLabelClass *klass)
{
	GObjectClass *object_class;
	GtkLinkLabelClass *label_class = GTK_LINK_LABEL_CLASS(klass);

	g_type_class_add_private(klass, sizeof(GtkLinkLabelPrivate));

	object_class = (GObjectClass *)klass;

	object_class->constructor= gtk_link_label_constructor;
	object_class->finalize = gtk_link_label_finalize;
	object_class->set_property = gtk_link_label_set_property;
	object_class->get_property = gtk_link_label_get_property;

	label_class->get_text = _gtk_link_label_get_text;
	label_class->set_text = _gtk_link_label_set_text;
	label_class->set_attributes = _gtk_link_label_set_attributes;
	label_class->get_attributes = _gtk_link_label_get_attributes;

	/* install properties */
	g_object_class_install_property(object_class,
	                                PROP_LABEL,
	                                g_param_spec_string("label", NULL, NULL, NULL, G_PARAM_READWRITE));

	g_object_class_install_property(object_class, PROP_XMARGIN,
	                                g_param_spec_int("xmargin", NULL, NULL, 0, G_MAXINT, 0, G_PARAM_READWRITE));

	g_object_class_install_property(object_class, PROP_YMARGIN,
	                                g_param_spec_int("ymargin", NULL, NULL, 0, G_MAXINT, 0, G_PARAM_READWRITE));

	/* install signals */
	g_signal_new("clicked", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
gtk_link_label_init(GtkLinkLabel *link_label)
{
	link_label->priv = G_TYPE_INSTANCE_GET_PRIVATE(link_label, GTK_TYPE_LINK_LABEL, GtkLinkLabelPrivate);
	memset(link_label->priv, 0, sizeof(GtkLinkLabelPrivate));
}

static GObject*
gtk_link_label_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_params)
{
	GObject *object;
	GtkLinkLabelPrivate *priv;

	object = G_OBJECT_CLASS(gtk_link_label_parent_class)->constructor(type, n_construct_properties, construct_params);

	gtk_widget_push_composite_child();

	priv = GTK_LINK_LABEL(object)->priv;

	/* create label */
	priv->label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(object), priv->label);

	/* signal handler */
	g_signal_connect(object, "event", (GCallback)_gtk_link_label_event, NULL);

	gtk_widget_push_composite_child();

	return object;
}

static void
gtk_link_label_finalize(GObject *object)
{
	G_OBJECT_CLASS(gtk_link_label_parent_class)->finalize(object);
}

static void
gtk_link_label_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GtkLinkLabel *self = (GtkLinkLabel*)object;

	switch(property_id)
	{
		case PROP_LABEL:
			gtk_label_set_text(GTK_LABEL(self->priv->label), g_value_get_string(value));
			break;

		case PROP_XMARGIN:
			self->priv->xmargin = g_value_get_int(value);
			gtk_misc_set_padding(GTK_MISC(self->priv->label), self->priv->xmargin, self->priv->ymargin);
			break;

		case PROP_YMARGIN:
			self->priv->ymargin = g_value_get_int(value);
			gtk_misc_set_padding(GTK_MISC(self->priv->label), self->priv->xmargin, self->priv->ymargin);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
gtk_link_label_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GtkLinkLabel *self = (GtkLinkLabel*)object;
	GtkLinkLabelPrivate *priv;

	priv = GTK_LINK_LABEL(object)->priv;

	switch(property_id)
	{
		case PROP_LABEL:
			g_value_set_string(value, gtk_label_get_text(GTK_LABEL(self->priv->label)));
			break;

		case PROP_XMARGIN:
			g_value_set_int(value, priv->xmargin);
			break;

		case PROP_YMARGIN:
			g_value_set_int(value, priv->ymargin);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

/*
 *	public:
 */
GtkWidget *
gtk_link_label_new(const gchar *text)
{
	
	return g_object_new(gtk_link_label_get_type(), "label", text, NULL);
}

const gchar *
gtk_link_label_get_text(GtkLinkLabel *link_label)
{
	return GTK_LINK_LABEL_GET_CLASS(link_label)->get_text(link_label);
}

void
gtk_link_label_set_text(GtkLinkLabel *link_label, const gchar *text)
{
	GTK_LINK_LABEL_GET_CLASS(link_label)->set_text(link_label, text);
}

void
gtk_link_label_set_attributes(GtkLinkLabel *link_label, PangoAttrList *attrs)
{
	GTK_LINK_LABEL_GET_CLASS(link_label)->set_attributes(link_label, attrs);
}

PangoAttrList *
gtk_link_label_get_attributes(GtkLinkLabel *link_label)
{
	return GTK_LINK_LABEL_GET_CLASS(link_label)->get_attributes(link_label);
}

/**
 * @}
 * @}
 */


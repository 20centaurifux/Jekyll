/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __GTK_DELETABLE_DIALOG_H__
#define __GTK_DELETABLE_DIALOG_H__

#include <gtk/gtk.h>

/**
 * @addtogroup Gui
 * @{
 * 	@addtogroup Widgets
 * 	@{
 */

#define GTK_TYPE_DELETABLE_DIALOG                  (gtk_deletable_dialog_get_type ())
#define GTK_DELETABLE_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_DELETABLE_DIALOG, GtkDeletableDialog))
#define GTK_DELETABLE_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_DELETABLE_DIALOG, GtkDeletableDialogClass))
#define GTK_IS_DELETABLE_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_DELETABLE_DIALOG))
#define GTK_IS_DELETABLE_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DELETABLE_DIALOG))
#define GTK_DELETABLE_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_DELETABLE_DIALOG, GtkDeletableDialogClass))


typedef struct _GtkDeletableDialog        GtkDeletableDialog;
typedef struct _GtkDeletableDialogClass   GtkDeletableDialogClass;

struct _GtkDeletableDialog
{
  GtkWindow window;

  /*< public >*/
  GtkWidget *GSEAL (vbox);
  GtkWidget *GSEAL (action_area);

  /*< private >*/
  GtkWidget *GSEAL (separator);
};

struct _GtkDeletableDialogClass
{
  GtkWindowClass parent_class;

  void (* response) (GtkDeletableDialog *dialog, gint response_id);

  /* Keybinding signals */

  void (* close)    (GtkDeletableDialog *dialog);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType      gtk_deletable_dialog_get_type (void) G_GNUC_CONST;
GtkWidget* gtk_deletable_dialog_new      (void);

GtkWidget* gtk_deletable_dialog_new_with_buttons (const gchar     *title,
                                        GtkWindow       *parent,
                                        GtkDialogFlags   flags,
                                        const gchar     *first_button_text,
                                        ...);

void       gtk_deletable_dialog_add_action_widget (GtkDeletableDialog   *dialog,
                                         GtkWidget   *child,
                                         gint         response_id);
GtkWidget* gtk_deletable_dialog_add_button        (GtkDeletableDialog   *dialog,
                                         const gchar *button_text,
                                         gint         response_id);
void       gtk_deletable_dialog_add_buttons       (GtkDeletableDialog   *dialog,
                                         const gchar *first_button_text,
                                         ...) G_GNUC_NULL_TERMINATED;

void gtk_deletable_dialog_set_response_sensitive (GtkDeletableDialog *dialog,
                                        gint       response_id,
                                        gboolean   setting);
void gtk_deletable_dialog_set_default_response   (GtkDeletableDialog *dialog,
                                        gint       response_id);
GtkWidget* gtk_deletable_dialog_get_widget_for_response (GtkDeletableDialog *dialog,
                                               gint       response_id);
gint gtk_deletable_dialog_get_response_for_widget (GtkDeletableDialog *dialog,
					 GtkWidget *widget);

#if !defined (GTK_DISABLE_DEPRECATED) || defined (GTK_COMPILATION)
void     gtk_deletable_dialog_set_has_separator (GtkDeletableDialog *dialog,
                                       gboolean   setting);
gboolean gtk_deletable_dialog_get_has_separator (GtkDeletableDialog *dialog);
#endif

gboolean gtk_alternative_dialog_button_order (GdkScreen *screen);
void     gtk_deletable_dialog_set_alternative_button_order (GtkDeletableDialog *dialog,
						  gint       first_response_id,
						  ...);
void     gtk_deletable_dialog_set_alternative_button_order_from_array (GtkDeletableDialog *dialog,
                                                             gint       n_params,
                                                             gint      *new_order);

/* Emit response signal */
void gtk_deletable_dialog_response           (GtkDeletableDialog *dialog,
                                    gint       response_id);

/* Returns response_id */
gint gtk_deletable_dialog_run                (GtkDeletableDialog *dialog);

GtkWidget * gtk_deletable_dialog_get_action_area  (GtkDeletableDialog *dialog);
GtkWidget * gtk_deletable_dialog_get_content_area (GtkDeletableDialog *dialog);

/* For private use only */
void _gtk_deletable_dialog_set_ignore_separator (GtkDeletableDialog *dialog,
				       gboolean   ignore_separator);

#endif

/**
 * @}
 * @}
 */


/* HSV color selector for GTK
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for GTK)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for GTK)
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK at ftp://ftp.gtk.org/pub/gtk/.
 *
 * Updated and adapted for inclusion in ColPicker by Jente Hidskes 2018.
 */

#ifndef __COLPICKER_HSV_H__
#define __COLPICKER_HSV_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define COLPICKER_TYPE_HSV            (colpicker_hsv_get_type ())
#define COLPICKER_HSV(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLPICKER_TYPE_HSV, ColPickerHSV))
#define COLPICKER_HSV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLPICKER_TYPE_HSV, ColPickerHSVClass))
#define COLPICKER_IS_HSV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLPICKER_TYPE_HSV))
#define COLPICKER_IS_HSV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLPICKER_TYPE_HSV))
#define COLPICKER_HSV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), COLPICKER_TYPE_HSV, ColPickerHSVClass))


typedef struct _ColPickerHSV              ColPickerHSV;
typedef struct _ColPickerHSVPrivate       ColPickerHSVPrivate;
typedef struct _ColPickerHSVClass         ColPickerHSVClass;

struct _ColPickerHSV
{
  GtkWidget parent_instance;

  /*< private >*/
  ColPickerHSVPrivate *priv;
};

struct _ColPickerHSVClass
{
  GtkWidgetClass parent_class;

  /* Notification signals */
  void (* changed) (ColPickerHSV       *hsv);

  /* Keybindings */
  void (* move)    (ColPickerHSV       *hsv,
                    GtkDirectionType  type);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType      colpicker_hsv_get_type     (void) G_GNUC_CONST;
GtkWidget* colpicker_hsv_new          (void);
void       colpicker_hsv_set_color    (ColPickerHSV    *hsv,
				     double         h,
				     double         s,
				     double         v);
void       colpicker_hsv_get_color    (ColPickerHSV    *hsv,
				     gdouble       *h,
				     gdouble       *s,
				     gdouble       *v);
void       colpicker_hsv_set_metrics  (ColPickerHSV    *hsv,
				     gint           size,
				     gint           ring_width);
void       colpicker_hsv_get_metrics  (ColPickerHSV    *hsv,
				     gint          *size,
				     gint          *ring_width);
gboolean   colpicker_hsv_is_adjusting (ColPickerHSV    *hsv);

G_END_DECLS

#endif /* __COLPICKER_HSV_H__ */

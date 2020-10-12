/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK at ftp://ftp.gtk.org/pub/gtk/.
 *
 * Updated and adapted for inclusion in ColPicker by Jente Hidskes 2017.
 */

#ifndef __COLPICKER_COLOR_SELECTION_H__
#define __COLPICKER_COLOR_SELECTION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define COLPICKER_TYPE_COLOR_SELECTION            (colpicker_color_selection_get_type ())
#define COLPICKER_COLOR_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLPICKER_TYPE_COLOR_SELECTION, ColPickerColorSelection))
#define COLPICKER_COLOR_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLPICKER_TYPE_COLOR_SELECTION, ColPickerColorSelectionClass))
#define COLPICKER_IS_COLOR_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLPICKER_TYPE_COLOR_SELECTION))
#define COLPICKER_IS_COLOR_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLPICKER_TYPE_COLOR_SELECTION))
#define COLPICKER_COLOR_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), COLPICKER_TYPE_COLOR_SELECTION, ColPickerColorSelectionClass))

typedef struct _ColPickerColorSelection        ColPickerColorSelection;
typedef struct _ColPickerColorSelectionPrivate ColPickerColorSelectionPrivate;
typedef struct _ColPickerColorSelectionClass   ColPickerColorSelectionClass;

/**
 * ColPickerColorSelectionChangePaletteFunc:
 * @colors: (array length=n_colors): Array of colors
 * @n_colors: Number of colors in the array
 */
typedef void (* ColPickerColorSelectionChangePaletteFunc) (const GdkRGBA *colors,
                                                         gint           n_colors);

/**
 * ColPickerColorSelectionChangePaletteWithScreenFunc:
 * @screen:
 * @colors: (array length=n_colors): Array of colors
 * @n_colors: Number of colors in the array
 *
 * Since: 2.2
 */
typedef void (* ColPickerColorSelectionChangePaletteWithScreenFunc) (GdkScreen     *screen,
							           const GdkRGBA *colors,
							           gint           n_colors);

struct _ColPickerColorSelection
{
  GtkBox parent_instance;

  /*< private >*/
  ColPickerColorSelectionPrivate *private_data;
};

/**
 * ColPickerColorSelectionClass:
 * @parent_class: The parent class.
 * @color_changed:
 */
struct _ColPickerColorSelectionClass
{
  GtkBoxClass parent_class;

  void (*color_changed)	(ColPickerColorSelection *color_selection);

  /*< private >*/

  /* Padding for future expansion */
  void (*_colpicker_reserved1) (void);
  void (*_colpicker_reserved2) (void);
  void (*_colpicker_reserved3) (void);
  void (*_colpicker_reserved4) (void);
};


/* ColorSelection */

GType      colpicker_color_selection_get_type                (void) G_GNUC_CONST;
GtkWidget *colpicker_color_selection_new                     (void);
gboolean   colpicker_color_selection_get_has_opacity_control (ColPickerColorSelection *colorsel);
void       colpicker_color_selection_set_has_opacity_control (ColPickerColorSelection *colorsel,
							    gboolean               has_opacity);
gboolean   colpicker_color_selection_get_has_palette         (ColPickerColorSelection *colorsel);
void       colpicker_color_selection_set_has_palette         (ColPickerColorSelection *colorsel,
							    gboolean               has_palette);


void     colpicker_color_selection_set_current_alpha   (ColPickerColorSelection *colorsel,
						      guint16                alpha);
guint16  colpicker_color_selection_get_current_alpha   (ColPickerColorSelection *colorsel);
void     colpicker_color_selection_set_previous_alpha  (ColPickerColorSelection *colorsel,
						      guint16                alpha);
guint16  colpicker_color_selection_get_previous_alpha  (ColPickerColorSelection *colorsel);

void     colpicker_color_selection_set_current_rgba    (ColPickerColorSelection *colorsel,
                                                      const GdkRGBA         *rgba);
void     colpicker_color_selection_get_current_rgba    (ColPickerColorSelection *colorsel,
                                                      GdkRGBA               *rgba);
void     colpicker_color_selection_set_previous_rgba   (ColPickerColorSelection *colorsel,
                                                      const GdkRGBA         *rgba);
void     colpicker_color_selection_get_previous_rgba   (ColPickerColorSelection *colorsel,
                                                      GdkRGBA               *rgba);

gboolean colpicker_color_selection_is_adjusting        (ColPickerColorSelection *colorsel);

gboolean colpicker_color_selection_palette_from_string (const gchar      *str,
                                                      GdkRGBA         **colors,
                                                      gint             *n_colors);
gchar*   colpicker_color_selection_palette_to_string   (const GdkRGBA    *colors,
                                                      gint              n_colors);

ColPickerColorSelectionChangePaletteWithScreenFunc colpicker_color_selection_set_change_palette_with_screen_hook (ColPickerColorSelectionChangePaletteWithScreenFunc func);

void     colpicker_color_selection_set_current_color   (ColPickerColorSelection *colorsel,
                                                      const GdkRGBA         *color);
void     colpicker_color_selection_get_current_color   (ColPickerColorSelection *colorsel,
                                                      GdkRGBA               *color);
void     colpicker_color_selection_set_previous_color  (ColPickerColorSelection *colorsel,
                                                      const GdkRGBA         *color);
void     colpicker_color_selection_get_previous_color  (ColPickerColorSelection *colorsel,
                                                      GdkRGBA               *color);

G_END_DECLS

#endif /* __COLPICKER_COLOR_SELECTION_H__ */

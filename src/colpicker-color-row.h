/* ColPickerColorRow
 *
 * Copyright (C) 2018 Jente Hidskes
 *
 * Author: Jente Hidskes <hjdskes@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __COLPICKER_COLOR_ROW_H__
#define __COLPICKER_COLOR_ROW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _ColPickerColorRow        ColPickerColorRow;
typedef struct _ColPickerColorRowClass   ColPickerColorRowClass;
typedef struct _ColPickerColorRowPrivate ColPickerColorRowPrivate;

#define COLPICKER_TYPE_COLOR_ROW            (colpicker_color_row_get_type ())
#define COLPICKER_COLOR_ROW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), COLPICKER_TYPE_COLOR_ROW, ColPickerColorRow))
#define COLPICKER_COLOR_ROW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  COLPICKER_TYPE_COLOR_ROW, ColPickerColorRowClass))
#define COLPICKER_IS_COLOR_ROW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), COLPICKER_TYPE_COLOR_ROW))
#define COLPICKER_IS_COLOR_ROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  COLPICKER_TYPE_COLOR_ROW))
#define COLPICKER_COLOR_ROW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  COLPICKER_TYPE_COLOR_ROW, ColPickerColorRowClass))

struct _ColPickerColorRow {
	GtkListBoxRow base_instance;
};

struct _ColPickerColorRowClass {
	GtkListBoxRowClass parent_class;
};

GType            colpicker_color_row_get_type (void) G_GNUC_CONST;

ColPickerColorRow *colpicker_color_row_new (const gchar *key, const gchar *hex);

G_END_DECLS

#endif /* __COLPICKER_COLOR_ROW_H__ */

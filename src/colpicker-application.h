/* ColPickerApplication
 *
 * Copyright (C) 2015-2016 Jente Hidskes
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

#ifndef __COLPICKER_APPLICATION_H__
#define __COLPICKER_APPLICATION_H__

#include <gtk/gtk.h>
#include <glib.h>

G_BEGIN_DECLS

typedef struct _ColPickerApplication        ColPickerApplication;
typedef struct _ColPickerApplicationClass   ColPickerApplicationClass;
typedef struct _ColPickerApplicationPrivate ColPickerApplicationPrivate;

#define COLPICKER_TYPE_APPLICATION            (colpicker_application_get_type ())
#define COLPICKER_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), COLPICKER_TYPE_APPLICATION, ColPickerApplication))
#define COLPICKER_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  COLPICKER_TYPE_APPLICATION, ColPickerApplicationClass))
#define COLPICKER_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), COLPICKER_TYPE_APPLICATION))
#define COLPICKER_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  COLPICKER_TYPE_APPLICATION))
#define COLPICKER_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  COLPICKER_TYPE_APPLICATION, ColPickerApplicationClass))

struct _ColPickerApplication {
	GtkApplication base_instance;
};

struct _ColPickerApplicationClass {
	GtkApplicationClass parent_class;
};

GType               colpicker_application_get_type (void) G_GNUC_CONST;

ColPickerApplication *colpicker_application_new (void);

G_END_DECLS

#endif /* __COLPICKER_APPLICATION_H__ */


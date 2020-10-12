/* ColPickerWindow
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

#ifndef __COLPICKER_WINDOW_H__
#define __COLPICKER_WINDOW_H__

#include <gtk/gtk.h>
#include <glib.h>

#include "colpicker-application.h"
#include "colpicker-color-store.h"

G_BEGIN_DECLS

typedef struct _ColPickerWindow        ColPickerWindow;
typedef struct _ColPickerWindowClass   ColPickerWindowClass;
typedef struct _ColPickerWindowPrivate ColPickerWindowPrivate;

#define COLPICKER_TYPE_WINDOW            (colpicker_window_get_type ())
#define COLPICKER_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), COLPICKER_TYPE_WINDOW, ColPickerWindow))
#define COLPICKER_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  COLPICKER_TYPE_WINDOW, ColPickerWindowClass))
#define COLPICKER_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), COLPICKER_TYPE_WINDOW))
#define COLPICKER_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  COLPICKER_TYPE_WINDOW))
#define COLPICKER_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  COLPICKER_TYPE_WINDOW, ColPickerWindowClass))

struct _ColPickerWindow {
	GtkApplicationWindow base_instance;
};

struct _ColPickerWindowClass {
	GtkApplicationWindowClass parent_class;
};

GType          colpicker_window_get_type (void) G_GNUC_CONST;

ColPickerWindow *colpicker_window_new (ColPickerApplication *application, ColPickerColorStore *store);
void           colpicker_window_destroy (ColPickerWindow *window, gpointer user_data);

G_END_DECLS

#endif /* __COLPICKER_WINDOW_H__ */


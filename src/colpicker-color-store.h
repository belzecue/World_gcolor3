/* ColPickerColorStore
 *
 * Copyright (C) 2016 Jente Hidskes
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

#ifndef __COLPICKER_COLOR_STORE_H__
#define __COLPICKER_COLOR_STORE_H__

#include <gtk/gtk.h>
#include <glib.h>

G_BEGIN_DECLS

typedef struct _ColPickerColorStore        ColPickerColorStore;
typedef struct _ColPickerColorStoreClass   ColPickerColorStoreClass;
typedef struct _ColPickerColorStorePrivate ColPickerColorStorePrivate;

typedef void (*ColPickerColorStoreForeachFunc) (const gchar *key,
					      const gchar *hex,
					      gpointer     user_data);

#define COLPICKER_TYPE_COLOR_STORE            (colpicker_color_store_get_type ())
#define COLPICKER_COLOR_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), COLPICKER_TYPE_COLOR_STORE, ColPickerColorStore))
#define COLPICKER_COLOR_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  COLPICKER_TYPE_COLOR_STORE, ColPickerColorStoreClass))
#define COLPICKER_IS_COLOR_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), COLPICKER_TYPE_COLOR_STORE))
#define COLPICKER_IS_COLOR_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  COLPICKER_TYPE_COLOR_STORE))
#define COLPICKER_COLOR_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  COLPICKER_TYPE_COLOR_STORE, ColPickerColorStoreClass))

struct _ColPickerColorStore {
	GObject base_instance;
};

struct _ColPickerColorStoreClass {
	GObjectClass parent_class;
};

GType              colpicker_color_store_get_type (void);

ColPickerColorStore *colpicker_color_store_new (void);

gboolean           colpicker_color_store_add_color (ColPickerColorStore *store,
						  const gchar       *key,
						  const gchar       *hex);

gboolean           colpicker_color_store_remove_color (ColPickerColorStore *store,
						     const gchar       *key);

gboolean           colpicker_color_store_rename_color (ColPickerColorStore *store,
						     const gchar *old_name,
						     const gchar *new_name);

void               colpicker_color_store_foreach (ColPickerColorStore           *store,
						ColPickerColorStoreForeachFunc func,
						gpointer                     user_data);

gboolean           colpicker_color_store_empty (ColPickerColorStore *store);

G_END_DECLS

#endif /* __COLPICKER_COLOR_STORE_H__ */


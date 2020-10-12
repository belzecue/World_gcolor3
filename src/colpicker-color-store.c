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

#include "config.h"

#include <errno.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "colpicker-color-item.h"
#include "colpicker-color-store.h"
#include "colpicker-marshalers.h"

struct _ColPickerColorStorePrivate {
	GKeyFile *colors;
};

static void colpicker_color_store_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ColPickerColorStore, colpicker_color_store, G_TYPE_OBJECT,
			 G_ADD_PRIVATE (ColPickerColorStore)
			 G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, colpicker_color_store_iface_init)
			)

static inline gchar *
get_user_file (void)
{
	return g_build_filename (g_get_user_config_dir (), "colpicker", "config.ini", NULL);
}

static inline gchar *
get_user_dir (void)
{
	return g_build_filename (g_get_user_config_dir (), "colpicker", NULL);
}

static gboolean
ensure_user_dir (void)
{
	gchar *dir;
	int ret;

	dir = get_user_dir ();
	ret = g_mkdir_with_parents (dir, 0700);
	g_free (dir);

	if (ret < 0) {
		g_warning ("Error creating config directory: %s", g_strerror (errno));
		return FALSE;
	}
	return TRUE;
}

static GType
colpicker_color_store_get_item_type (UNUSED GListModel *list)
{
	return G_TYPE_VARIANT;
}

static guint
colpicker_color_store_get_n_items (GListModel *list)
{
	ColPickerColorStorePrivate *priv;
	GError *error = NULL;
	gsize n_items;
	gchar **keys;

	priv = colpicker_color_store_get_instance_private (COLPICKER_COLOR_STORE (list));

	if (!(keys = g_key_file_get_keys (priv->colors, "Colors", &n_items, &error))) {
		g_warning ("Cannot count number of items: %s\n", error->message);
		g_clear_error (&error);
		return 0;
	}
	g_strfreev (keys);
	return n_items;
}

static gpointer
colpicker_color_store_get_item (GListModel *list, guint position)
{
	ColPickerColorStorePrivate *priv;
	ColPickerColorItem *item;
	GError *error = NULL;
	gsize n_items;
	gchar **keys;
	gchar *value;

	priv = colpicker_color_store_get_instance_private (COLPICKER_COLOR_STORE (list));

	if (!(keys = g_key_file_get_keys (priv->colors, "Colors", &n_items, &error))) {
		g_warning ("Cannot read colors: %s\n", error->message);
		g_clear_error (&error);
		// Returning NULL violates an assumption of the interface,
		// where get_item should never return NULL for an
		// index that is smaller than the length of the
		// list. Hence, we return a default value of black.
		return g_variant_new ("(ss)", "Black", "#000000");
	}

	if (position > n_items) {
		return NULL;
	}

	if (!(value = g_key_file_get_string (priv->colors, "Colors", keys[position], &error))) {
		g_warning ("Cannot access color at position %d: %s\n", position, error->message);
		g_clear_error (&error);
		return g_variant_new ("(ss)", "Black", "#000000");
	}

	item = colpicker_color_item_new (keys[position], value);
	g_free (value);
	g_strfreev (keys);
	return item;
}

static void
colpicker_color_store_dispose (GObject *object)
{
	ColPickerColorStorePrivate *priv;
	GError *error = NULL;
	gchar *file;

	priv = colpicker_color_store_get_instance_private (COLPICKER_COLOR_STORE (object));

	// TODO: possibly only write to disk if contents changed?
	file = get_user_file ();
	if ((ensure_user_dir ()) && !(g_key_file_save_to_file (priv->colors, file, &error))) {
		g_warning ("Error writing file: %s", error->message);
		g_clear_error (&error);
	}
	g_free (file);
	g_key_file_free (priv->colors);

	G_OBJECT_CLASS (colpicker_color_store_parent_class)->dispose (object);
}

static void
colpicker_color_store_class_init (ColPickerColorStoreClass *colpicker_color_store_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (colpicker_color_store_class);

	object_class->dispose = colpicker_color_store_dispose;
}

static void
colpicker_color_store_iface_init (GListModelInterface *iface)
{
	iface->get_item_type = colpicker_color_store_get_item_type;
	iface->get_n_items = colpicker_color_store_get_n_items;
	iface->get_item = colpicker_color_store_get_item;
}

static void
colpicker_color_store_init (ColPickerColorStore *store)
{
	ColPickerColorStorePrivate *priv;
	gchar *file;
	GError *error = NULL;

	priv = colpicker_color_store_get_instance_private (store);

	priv->colors = g_key_file_new ();
	file = get_user_file ();
	if (!(g_key_file_load_from_file (priv->colors,
					 file,
					 G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
					 &error))) {
		g_warning ("Error opening file: %s. Colors likely won't be saved, unless this is the first run",
			   error->message);
		g_clear_error (&error);
	}

	g_free (file);
}

ColPickerColorStore *
colpicker_color_store_new ()
{
	return g_object_new (COLPICKER_TYPE_COLOR_STORE, NULL);
}

gboolean
colpicker_color_store_add_color (ColPickerColorStore *store, const gchar *key, const gchar *hex)
{
	ColPickerColorStorePrivate *priv;
	GError *error = NULL;
	gchar **keys;
	gsize length;

	g_return_val_if_fail (COLPICKER_IS_COLOR_STORE (store), FALSE);
	g_return_val_if_fail (key != NULL && hex != NULL, FALSE);

	priv = colpicker_color_store_get_instance_private (store);

	if (g_key_file_has_key (priv->colors, "Colors", key, NULL)) {
		g_warning ("There is already a color named `%s`", key);
		return FALSE;
	}

	g_key_file_set_string (priv->colors, "Colors", key, hex);

	if (!(keys = g_key_file_get_keys (priv->colors, "Colors", &length, &error))) {
		g_warning ("Cannot locate index of addition: %s. UI won't be updated\n", error->message);
		g_clear_error (&error);
		return TRUE;
	}

	for (guint i = 0; i < length; i++) {
		if (strcmp (keys[i], key) != 0) {
			continue;
		}
		g_list_model_items_changed (G_LIST_MODEL (store), i, 0, 1);
		break;
	}

	g_strfreev (keys);
	return TRUE;
}

gboolean
colpicker_color_store_remove_color (ColPickerColorStore *store, const gchar *key)
{
	ColPickerColorStorePrivate *priv;
	GError *error = NULL;
	gchar **keys;
	gsize length;
	guint i;

	g_return_val_if_fail (COLPICKER_IS_COLOR_STORE (store), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	priv = colpicker_color_store_get_instance_private (store);

	if (!(keys = g_key_file_get_keys (priv->colors, "Colors", &length, &error))) {
		g_warning ("Cannot locate index of removal: %s. UI won't be updated\n", error->message);
		g_clear_error (&error);
		return TRUE;
	}

	for (i = 0; i < length; i++) {
		if (strcmp (keys[i], key) != 0) {
			continue;
		}
		break;
	}
	g_strfreev (keys);

	if (!(g_key_file_remove_key (priv->colors, "Colors", key, &error))) {
		g_warning ("Error deleting key: %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	g_list_model_items_changed (G_LIST_MODEL (store), i, 1, 0);
	return TRUE;
}

gboolean
colpicker_color_store_rename_color (ColPickerColorStore *store,
				  const gchar *old_name,
				  const gchar *new_name)
{
	ColPickerColorStorePrivate *priv;
	gchar *hex;
	GError *error = NULL;

	g_return_val_if_fail (COLPICKER_IS_COLOR_STORE (store), FALSE);
	g_return_val_if_fail (old_name != NULL && new_name != NULL, FALSE);

	priv = colpicker_color_store_get_instance_private (store);

	if (strlen (new_name) <= 0 || !g_strcmp0 (old_name, new_name)) {
		return FALSE;
	}

	if (!(g_key_file_has_key (priv->colors, "Colors", old_name, NULL))) {
		g_warning ("Cannot rename non-existing color `%s`", old_name);
		return FALSE;
	}

	if (g_key_file_has_key (priv->colors, "Colors", new_name, NULL)) {
		g_warning ("There is already a color named `%s`", new_name);
		return FALSE;
	}

	if (!(hex = g_key_file_get_string (priv->colors, "Colors", old_name, &error))) {
		g_warning ("Cannot retrieve hex value belonging to `%s`: %s", old_name, error->message);
		g_clear_error (&error);
		return FALSE;
	}

	if (!(g_key_file_remove_key (priv->colors, "Colors", old_name, &error))) {
		g_warning ("Error deleting old name `%s`: %s", old_name, error->message);
		g_clear_error (&error);
		g_free (hex);
		return FALSE;
	}

	g_key_file_set_string (priv->colors, "Colors", new_name, hex);
	g_free (hex);

	// TODO: find out position of rename. This is tricky, because
	// the removal might be in a different spot than the addition,
	// and the interface expects an in-place update. If we model
	// this by removing and inserting, the location changes and
	// this will be reflected in the UI, which is confusing.
	// g_list_model_items_changed (G_LIST_MODEL (store), x, 0, 0);

	return TRUE;
}

void
colpicker_color_store_foreach (ColPickerColorStore           *store,
			     ColPickerColorStoreForeachFunc func,
			     gpointer                     user_data)
{
	ColPickerColorStorePrivate *priv;
	GError *error = NULL;
	gchar **keys = NULL;
	gsize length;

	g_return_if_fail (COLPICKER_IS_COLOR_STORE (store));
	priv = colpicker_color_store_get_instance_private (store);

	if (!(keys = g_key_file_get_keys (priv->colors, "Colors", &length, &error))) {
		g_warning ("Error reading keys: %s. This is harmless if this is the first run", error->message);
		g_clear_error (&error);
		return;
	}

	for (guint i = 0; i < length; i++) {
		gchar *value;

		if (!(value = g_key_file_get_string (priv->colors, "Colors", keys[i], NULL))) {
			continue;
		}

		func (keys[i], value, user_data);
		g_free (value);
	}

	g_strfreev (keys);
}

gboolean
colpicker_color_store_empty (ColPickerColorStore *store)
{
	g_return_val_if_fail (COLPICKER_IS_COLOR_STORE (store), TRUE);
	return colpicker_color_store_get_n_items (G_LIST_MODEL (store)) == 0;
}

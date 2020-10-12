/* ColPickerColorItem
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

#include "config.h"

#include "colpicker-color-item.h"

enum {
      PROP_0,
      PROP_KEY,
      PROP_HEX,
};

struct _ColPickerColorItemPrivate {
	gchar *key;
	const gchar *hex;
};

G_DEFINE_TYPE_WITH_PRIVATE (ColPickerColorItem, colpicker_color_item, G_TYPE_OBJECT)

static void
colpicker_color_item_get_property (GObject    *object,
				 guint       prop_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	ColPickerColorItemPrivate *priv;

	priv = colpicker_color_item_get_instance_private (COLPICKER_COLOR_ITEM (object));

	switch (prop_id) {
	case PROP_KEY:
		g_value_set_string (value, priv->key);
		break;
	case PROP_HEX:
		g_value_set_string (value, priv->hex);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
colpicker_color_item_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	ColPickerColorItemPrivate *priv;

	priv = colpicker_color_item_get_instance_private (COLPICKER_COLOR_ITEM (object));

	switch (prop_id) {
	case PROP_KEY:
		priv->key = g_value_dup_string (value);
		break;
	case PROP_HEX:
		priv->hex = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
colpicker_color_item_finalize (GObject *object)
{
	ColPickerColorItemPrivate *priv;

	priv = colpicker_color_item_get_instance_private (COLPICKER_COLOR_ITEM (object));

	g_free (priv->key);

	G_OBJECT_CLASS (colpicker_color_item_parent_class)->finalize (object);
}

static void
colpicker_color_item_class_init (ColPickerColorItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = colpicker_color_item_finalize;
	object_class->get_property = colpicker_color_item_get_property;
	object_class->set_property = colpicker_color_item_set_property;

	g_object_class_install_property (object_class, PROP_KEY,
					 g_param_spec_string ("key",
							      "key",
							      "Key of this row's color",
							      "Black",
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_READWRITE |
							      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_HEX,
					 g_param_spec_string ("hex",
							      "HexValue",
							      "Hex value of this row's color",
							      "#000000",
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_READWRITE |
							      G_PARAM_STATIC_STRINGS));
}

static void
colpicker_color_item_init (ColPickerColorItem *item)
{
}

ColPickerColorItem *
colpicker_color_item_new (const gchar *key, const gchar *hex)
{
	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (hex != NULL, NULL);

	return g_object_new (COLPICKER_TYPE_COLOR_ITEM, "key", key, "hex", hex, NULL);
}

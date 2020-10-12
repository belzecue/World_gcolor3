/* ColPickerWindow
 *
 * Copyright (C) 2015-2018 Jente Hidskes
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>

#include "colpicker-color-item.h"
#include "colpicker-color-row.h"
#include "colpicker-color-selection.h"
#include "colpicker-color-store.h"
#include "colpicker-window.h"
#include "utils.h"

enum {
	PROP_0,
	PROP_STORE,
};

struct _ColPickerWindowPrivate {
	GtkWidget *button_primary_menu;
	GtkWidget *button_save;
	GtkWidget *entry;
	GtkWidget *page_stack;
	GtkWidget *picker;
	GtkWidget *list_stack;
	GtkWidget *scroll;
	GtkWidget *listbox;
	GtkWidget *empty_placeholder;

	ColPickerColorStore *store;

	GdkRGBA current;
};

G_DEFINE_TYPE_WITH_PRIVATE (ColPickerWindow, colpicker_window, GTK_TYPE_APPLICATION_WINDOW)

static void
save_color (ColPickerWindowPrivate *priv)
{
	const gchar *key;
	gchar *hex;

	key = gtk_entry_get_text (GTK_ENTRY (priv->entry));
	hex = hex_value (&priv->current);
	if (strlen (key) == 0) {
		/* If using `hex` as key, do not save the first character (e.g., `#`)
		 * because GKeyFile will see these as (and subsequently ignore) comments. */
		key = hex + 1;
	}

	/* This will add the color to the store (if the key isn't
	 * already used), which in turn will update the list box due
	 * to the binding between the store's list model and the list
	 * box. */
	colpicker_color_store_add_color (priv->store, key, hex);
	g_free (hex);

	if (!colpicker_color_store_empty (priv->store)) {
		gtk_stack_set_visible_child (GTK_STACK (priv->list_stack),
					     priv->scroll);
	}
}

static gboolean
colpicker_window_picker_page_key_handler (GtkWidget   *widget,
					GdkEventKey *event,
					gpointer     user_data)
{
	ColPickerWindowPrivate *priv;

	if (event->type != GDK_KEY_PRESS) {
		return GDK_EVENT_PROPAGATE;
	}

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));

	switch (event->keyval) {
	case GDK_KEY_s:
		if ((event->state & GDK_CONTROL_MASK) != GDK_CONTROL_MASK) {
			return GDK_EVENT_PROPAGATE;
		}
		/* Emulate a button click, to give the user visual feedback of
		   the save action. */
		g_signal_emit_by_name (priv->button_save, "activate");
		return GDK_EVENT_STOP;
	default:
		return GDK_EVENT_PROPAGATE;
	}
}

static void
colpicker_window_entry_activated (UNUSED GtkEntry *entry, gpointer user_data)
{
	ColPickerWindowPrivate *priv;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));
	/* Emulate a button click, to give the user visual feedback of
	   the save action. */
	g_signal_emit_by_name (priv->button_save, "activate");
}

static void
colpicker_window_save_button_clicked (UNUSED GtkButton *button, gpointer user_data)
{
	ColPickerWindowPrivate *priv;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));
	save_color (priv);
}

static void
colpicker_window_color_row_deleted (ColPickerColorRow *row, gpointer user_data)
{
	ColPickerWindowPrivate *priv;
	gchar *key;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));

	g_object_get (row, "key", &key, NULL);
	/* Removing a color from the store will automatically remove
	   it from the list box, due to the binding between the two. */
	colpicker_color_store_remove_color (priv->store, key);
	g_free (key);

	if (colpicker_color_store_empty (priv->store)) {
		gtk_stack_set_visible_child (GTK_STACK (priv->list_stack),
					     priv->empty_placeholder);
	}
}

static void
colpicker_window_color_row_renamed (UNUSED ColPickerColorRow *row,
				  const gchar *old_name,
				  const gchar *new_name,
				  gpointer user_data)
{
	ColPickerWindowPrivate *priv;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));
	colpicker_color_store_rename_color (priv->store, old_name, new_name);
}

static void
colpicker_window_action_change_page (UNUSED GSimpleAction *action,
				   UNUSED GVariant      *parameter,
				   gpointer              user_data)
{
	ColPickerWindowPrivate *priv;
	const gchar *page;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));

	page = gtk_stack_get_visible_child_name (GTK_STACK (priv->page_stack));
	if (g_strcmp0 (page, "saved-colors") == 0) {
		gtk_stack_set_visible_child_name (GTK_STACK (priv->page_stack), "picker");
	} else {
		gtk_stack_set_visible_child_name (GTK_STACK (priv->page_stack), "saved-colors");
	}
}

static const GActionEntry window_actions[] = {
	{ "change-page", colpicker_window_action_change_page, NULL, NULL, NULL },
};

static void
colpicker_window_picker_changed (ColPickerColorSelection *picker, gpointer user_data)
{
	ColPickerWindowPrivate *priv;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));

	colpicker_color_selection_get_current_color (COLPICKER_COLOR_SELECTION (picker), &priv->current);
}

static void
colpicker_window_stack_changed (GtkStack          *stack,
			      UNUSED GParamSpec *pspec,
			      gpointer           user_data)
{
	ColPickerWindowPrivate *priv;
	const gchar *page;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));

	page = gtk_stack_get_visible_child_name (stack);
	if (g_strcmp0 (page, "saved-colors") == 0) {
		gtk_widget_hide (priv->button_save);
		gtk_widget_hide (priv->entry);
	} else {
		gtk_widget_show (priv->button_save);
		gtk_widget_show (priv->entry);
	}
}

static void
colpicker_window_selection_changed (GtkListBox *listbox, gpointer user_data)
{
	ColPickerWindowPrivate *priv;
	GtkListBoxRow *row;
	GdkRGBA new, current;
	gchar *color;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (user_data));
	row = gtk_list_box_get_selected_row (listbox);

	if (!row) {
		/* This callback also triggers when a row is deleted,
		   in which the selected row returns NULL.  In this
		   case, we don't want to select another color,
		   anyway. */
		return;
	}

	g_object_get (row, "hex", &color, NULL);
	gdk_rgba_parse (&new, color);
	g_free (color);

	/* Save the old color in the picker. */
	colpicker_color_selection_get_current_color (COLPICKER_COLOR_SELECTION (priv->picker), &current);
	colpicker_color_selection_set_previous_color (COLPICKER_COLOR_SELECTION (priv->picker), &current);
	colpicker_color_selection_set_current_color (COLPICKER_COLOR_SELECTION (priv->picker), &new);
}

static GtkWidget *
create_widget_func (gpointer item, gpointer user_data)
{
	ColPickerColorRow *row;
	gchar *key, *hex;

	g_object_get ((ColPickerColorItem *) item, "key", &key, "hex", &hex, NULL);
	row = colpicker_color_row_new (key, hex);
	g_signal_connect (row, "color-removed",
			  G_CALLBACK (colpicker_window_color_row_deleted), user_data);
	g_signal_connect (row, "color-renamed",
			  G_CALLBACK (colpicker_window_color_row_renamed), user_data);
	g_free (key);
	g_free (hex);
	return GTK_WIDGET (row);
}

static void
colpicker_window_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	ColPickerWindowPrivate *priv;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (object));

	switch (prop_id) {
		case PROP_STORE:
			priv->store = COLPICKER_COLOR_STORE (g_value_dup_object (value));
			gtk_list_box_bind_model (GTK_LIST_BOX (priv->listbox),
						 G_LIST_MODEL (priv->store),
						 create_widget_func,
						 object,
						 NULL);
			if (colpicker_color_store_empty (priv->store)) {
				gtk_stack_set_visible_child (GTK_STACK (priv->list_stack),
							     priv->empty_placeholder);
			} else {
				gtk_stack_set_visible_child (GTK_STACK (priv->list_stack),
							     priv->scroll);
			}
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
colpicker_window_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	ColPickerWindowPrivate *priv;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (object));

	switch (prop_id) {
		case PROP_STORE:
			g_value_set_object (value, priv->store);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
colpicker_window_dispose (GObject *object)
{
	ColPickerWindowPrivate *priv;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (object));

	g_clear_object (&priv->store);

	G_OBJECT_CLASS (colpicker_window_parent_class)->dispose (object);
}

static void
colpicker_window_finalize (GObject *object)
{
	ColPickerWindowPrivate *priv;
	gchar *hex;

	priv = colpicker_window_get_instance_private (COLPICKER_WINDOW (object));

	hex = hex_value (&priv->current);
	g_printf ("%s\n", hex);
	g_free (hex);

	G_OBJECT_CLASS (colpicker_window_parent_class)->finalize (object);
}

static void
colpicker_window_class_init (ColPickerWindowClass *colpicker_window_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (colpicker_window_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (colpicker_window_class);

	object_class->set_property = colpicker_window_set_property;
	object_class->get_property = colpicker_window_get_property;
	object_class->dispose = colpicker_window_dispose;
	object_class->finalize = colpicker_window_finalize;

	g_object_class_install_property (object_class, PROP_STORE,
					 g_param_spec_object ("color-store",
							      "ColorStore",
							      "The managed colors",
							      COLPICKER_TYPE_COLOR_STORE,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

	gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/design/ColorPicker/window.ui");

	gtk_widget_class_bind_template_child_private (widget_class, ColPickerWindow, button_primary_menu);
	gtk_widget_class_bind_template_child_private (widget_class, ColPickerWindow, button_save);
	gtk_widget_class_bind_template_child_private (widget_class, ColPickerWindow, entry);
	gtk_widget_class_bind_template_child_private (widget_class, ColPickerWindow, page_stack);
	gtk_widget_class_bind_template_child_private (widget_class, ColPickerWindow, list_stack);
	gtk_widget_class_bind_template_child_private (widget_class, ColPickerWindow, scroll);
	gtk_widget_class_bind_template_child_private (widget_class, ColPickerWindow, listbox);
	gtk_widget_class_bind_template_child_private (widget_class, ColPickerWindow, empty_placeholder);

	gtk_widget_class_bind_template_callback (widget_class, colpicker_window_stack_changed);
	gtk_widget_class_bind_template_callback (widget_class, colpicker_window_picker_changed);
	gtk_widget_class_bind_template_callback (widget_class, colpicker_window_selection_changed);
	gtk_widget_class_bind_template_callback (widget_class, colpicker_window_picker_page_key_handler);
	gtk_widget_class_bind_template_callback (widget_class, colpicker_window_entry_activated);
	gtk_widget_class_bind_template_callback (widget_class, colpicker_window_save_button_clicked);
}

static void
colpicker_window_init (ColPickerWindow *window)
{
	ColPickerWindowPrivate *priv;
	GtkBuilder *menu_builder;
	GMenuModel *model;

	priv = colpicker_window_get_instance_private (window);

	gtk_widget_init_template (GTK_WIDGET (window));

	/* Add the primary menu */
	menu_builder = gtk_builder_new_from_resource ("/org/gnome/design/ColorPicker/menus.ui");
	model = G_MENU_MODEL (gtk_builder_get_object (menu_builder, "primary-menu"));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (priv->button_primary_menu), model);

	/* Add the custom color selection widget. */
	priv->picker = colpicker_color_selection_new ();
	gtk_widget_set_valign (priv->picker, GTK_ALIGN_CENTER);
	gtk_widget_set_halign (priv->picker, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top (priv->picker, 24);
	gtk_widget_set_margin_bottom (priv->picker, 24);
	gtk_widget_set_margin_start (priv->picker, 24);
	gtk_widget_set_margin_end (priv->picker, 24);
	g_signal_connect (priv->picker, "color-changed",
			  G_CALLBACK (colpicker_window_picker_changed), window);
	g_signal_connect (priv->picker, "key-press-event",
			  G_CALLBACK (colpicker_window_picker_page_key_handler), window);
	gtk_stack_add_titled (GTK_STACK (priv->page_stack), priv->picker, "picker", _("Picker"));
	gtk_container_child_set (GTK_CONTAINER (priv->page_stack), priv->picker, "position", 0, NULL);
	gtk_widget_set_visible (priv->picker, TRUE);

	/* Call the callback to initialise the GtkEntry and to prevent
	 * saving #000000 when saving the white color right away. */
	colpicker_window_picker_changed (COLPICKER_COLOR_SELECTION (priv->picker), window);

	g_action_map_add_action_entries (G_ACTION_MAP (window),
					 window_actions,
					 G_N_ELEMENTS (window_actions),
					 window);

	/* Finally, make the color picker the visible stack page. */
	gtk_stack_set_visible_child_name (GTK_STACK (priv->page_stack), "picker");
}

ColPickerWindow *
colpicker_window_new (ColPickerApplication *application, ColPickerColorStore *store)
{
	g_return_val_if_fail (COLPICKER_IS_APPLICATION (application), NULL);
	g_return_val_if_fail (COLPICKER_IS_COLOR_STORE (store), NULL);

	return g_object_new (COLPICKER_TYPE_WINDOW,
			     "application", application,
			     "color-store", store,
			     NULL);
}

void
colpicker_window_destroy (ColPickerWindow *window, UNUSED gpointer user_data)
{
	g_return_if_fail (window != NULL);
	gtk_widget_destroy (GTK_WIDGET (window));
}

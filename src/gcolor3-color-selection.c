/* GTK - The GIMP Toolkit
 * Copyright (C) 2017, 2018 Jente Hidskes <hjdskes@gmail.com>
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
 * Modified by the GTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 *
 * Updated and adapted for inclusion in Gcolor3 by Jente Hidskes 2017.
 */

#include "config.h"

// TODO: get rid of all deprecated API
#define GDK_DISABLE_DEPRECATION_WARNINGS

#include "gcolor3-color-selection.h"
#include "gcolor3-hsv.h"
#include "utils.h"

#include <gdk/gdk.h>
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef ENABLE_NLS
#define P_(String) g_dgettext(GETTEXT_PACKAGE "-properties",String)
#else
#define P_(String) (String)
#endif

#define I_(string) g_intern_static_string (string)

/**
 * SECTION:gtkcolorsel
 * @Short_description: Widget used to select a color
 * @Title: Gcolor3ColorSelection
 *
 * The #Gcolor3ColorSelection is a widget that is used to select
 * a color.  It consists of a color wheel and number of sliders
 * and entry boxes for color parameters such as hue, saturation,
 * value, red, green, blue, and opacity.
 */

/* The cursor for the dropper */
#define DROPPER_WIDTH 17
#define DROPPER_HEIGHT 17
#define DROPPER_STRIDE (DROPPER_WIDTH * 4)
#define DROPPER_X_HOT 2
#define DROPPER_Y_HOT 16

#define CHECK_SIZE 16
#define BIG_STEP 20

/* Conversion between 0->1 double and and guint16. See
 * scale_round() in utils.c for more general conversions
 */
#define SCALE(i) (i / 65535.)
#define UNSCALE(d) ((guint16)(d * G_MAXUINT16 + 0.5))

enum {
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_CURRENT_RGBA
};

enum {
  COLORSEL_RED = 0,
  COLORSEL_GREEN = 1,
  COLORSEL_BLUE = 2,
  COLORSEL_OPACITY = 3,
  COLORSEL_HUE,
  COLORSEL_SATURATION,
  COLORSEL_VALUE,
  COLORSEL_NUM_CHANNELS
};


struct _Gcolor3ColorSelectionPrivate
{
  guint changing          : 1;
  guint default_set       : 1;
  guint has_grab          : 1;

  // RGB and HSV values in the 0.0 and 1.0 range, inclusive.
  gdouble color[COLORSEL_NUM_CHANNELS];
  gdouble old_color[COLORSEL_NUM_CHANNELS];

  GtkWidget *vbox;
  GtkWidget *triangle_colorsel;
  GtkWidget *picker_button;
  GtkWidget *table;
  GtkAdjustment *adjustment_hue;
  GtkWidget *hue_spinbutton;
  GtkAdjustment *adjustment_sat;
  GtkWidget *sat_spinbutton;
  GtkAdjustment *adjustment_val;
  GtkWidget *val_spinbutton;
  GtkAdjustment *adjustment_red;
  GtkWidget *red_spinbutton;
  GtkAdjustment *adjustment_green;
  GtkWidget *green_spinbutton;
  GtkAdjustment *adjustment_blue;
  GtkWidget *blue_spinbutton;
  GtkAdjustment *adjustment_opacity;
  GtkWidget *opacity_slider;
  GtkWidget *opacity_label;
  GtkWidget *opacity_entry;
  GtkWidget *hex_entry;

  /* The color_sample stuff */
  GtkWidget *sample_area;
  GtkWidget *old_sample;
  GtkWidget *cur_sample;
  GtkWidget *colorsel;

  /* Window for grabbing on */
  GtkWidget *dropper_grab_widget;
  guint32    grab_time;
  GdkDevice *keyboard_device;
  GdkDevice *pointer_device;
};

static void color_sample_setup_dnd                  (Gcolor3ColorSelection   *colorsel,
						     GtkWidget               *sample);
static void gcolor3_color_selection_destroy         (GtkWidget               *widget);
static void update_color                            (Gcolor3ColorSelection   *colorsel);
static void gcolor3_color_selection_set_property    (GObject                 *object,
                                                     guint                    prop_id,
                                                     const GValue            *value,
                                                     GParamSpec              *pspec);
static void gcolor3_color_selection_get_property    (GObject                 *object,
                                                     guint                    prop_id,
                                                     GValue                  *value,
                                                     GParamSpec              *pspec);

static gboolean gcolor3_color_selection_grab_broken (GtkWidget               *widget,
                                                     GdkEventGrabBroken      *event);

static void     make_all_relations                          (AtkObject             *atk_obj,
                                                             Gcolor3ColorSelectionPrivate *priv);

static void     hsv_changed                                 (GtkWidget             *hsv,
                                                             gpointer               data);
static void     get_screen_color                            (GtkWidget             *button,
							     gpointer               data);
static void     adjustment_changed                          (GtkAdjustment         *adjustment,
                                                             gpointer               data);
static void     opacity_entry_changed                       (GtkWidget             *opacity_entry,
                                                             gpointer               data);
static void     hex_changed                                 (GtkWidget             *hex_entry,
                                                             gpointer               data);
static gboolean hex_focus_out                               (GtkWidget             *hex_entry,
                                                             GdkEventFocus         *event,
                                                             gpointer               data);
static gboolean color_old_sample_draw                       (GtkWidget             *da,
							     cairo_t               *cr,
							     Gcolor3ColorSelection *colorsel);
static gboolean color_cur_sample_draw                       (GtkWidget             *da,
							     cairo_t               *cr,
							     Gcolor3ColorSelection *colorsel);
static gboolean mouse_press                                 (GtkWidget             *invisible,
                                                             GdkEventButton        *event,
                                                             gpointer               data);
static void shutdown_eyedropper                             (GtkWidget *widget);

static guint color_selection_signals[LAST_SIGNAL] = { 0 };

static const guchar dropper_bits[] = {
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377"
  "\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\0\0\0\377"
  "\0\0\0\377\0\0\0\377\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377"
  "\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\377\377\377\377"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377"
  "\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\377\0\0"
  "\0\377\0\0\0\377\0\0\0\377\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\0\0\0\377\0\0\0\377\0"
  "\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\377\377\377"
  "\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\377\377\377\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0"
  "\0\0\377\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\0\0\0\377\0\0"
  "\0\377\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\377\377\377"
  "\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\0\0\0\377\377\377\377\377\0\0\0\377\377\377\377\377\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\377\377"
  "\377\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0\0"
  "\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\0\0"
  "\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\0\0\0\0\0\0\0\377\0\0\0"
  "\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"};

G_DEFINE_TYPE_WITH_PRIVATE (Gcolor3ColorSelection, gcolor3_color_selection, GTK_TYPE_BOX)

static void
gcolor3_color_selection_class_init (Gcolor3ColorSelectionClass *gcolor3_color_selection_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (gcolor3_color_selection_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (gcolor3_color_selection_class);

  object_class->set_property = gcolor3_color_selection_set_property;
  object_class->get_property = gcolor3_color_selection_get_property;

  widget_class->destroy = gcolor3_color_selection_destroy;
  widget_class->grab_broken_event = gcolor3_color_selection_grab_broken;

  /**
   * Gcolor3ColorSelection:current-rgba:
   *
   * The current RGBA color.
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_CURRENT_RGBA,
                                   g_param_spec_boxed ("current-rgba",
                                                       P_("Current RGBA"),
                                                       P_("The current RGBA color"),
                                                       GDK_TYPE_RGBA,
                                                       G_PARAM_READWRITE|
                                                       G_PARAM_STATIC_NAME|
                                                       G_PARAM_STATIC_NICK|
                                                       G_PARAM_STATIC_BLURB));

  /**
   * Gcolor3ColorSelection::color-changed:
   * @colorselection: the object which received the signal.
   *
   * This signal is emitted when the color changes in the #Gcolor3ColorSelection
   * according to its update policy.
   */
  color_selection_signals[COLOR_CHANGED] =
    g_signal_new (I_("color-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (Gcolor3ColorSelectionClass, color_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/nl/hjdskes/gcolor3/color-selection.ui");

  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, vbox);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, picker_button);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, table);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, adjustment_hue);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, hue_spinbutton);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, adjustment_sat);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, sat_spinbutton);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, adjustment_val);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, val_spinbutton);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, adjustment_red);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, red_spinbutton);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, adjustment_green);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, green_spinbutton);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, adjustment_blue);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, blue_spinbutton);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, adjustment_opacity);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, opacity_slider);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, opacity_label);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, opacity_entry);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, hex_entry);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, sample_area);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, old_sample);
  gtk_widget_class_bind_template_child_private (widget_class, Gcolor3ColorSelection, cur_sample);

  gtk_widget_class_bind_template_callback (widget_class, color_old_sample_draw);
  gtk_widget_class_bind_template_callback (widget_class, color_cur_sample_draw);
  gtk_widget_class_bind_template_callback (widget_class, get_screen_color);
  gtk_widget_class_bind_template_callback (widget_class, opacity_entry_changed);
  gtk_widget_class_bind_template_callback (widget_class, hex_changed);
  gtk_widget_class_bind_template_callback (widget_class, hex_focus_out);
}

static void
gcolor3_color_selection_init (Gcolor3ColorSelection *colorsel)
{
  Gcolor3ColorSelectionPrivate *priv;
  AtkObject *atk_obj;
  GList *focus_chain = NULL;

  gtk_widget_init_template (GTK_WIDGET (colorsel));

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  priv->changing = FALSE;
  priv->default_set = FALSE;

  priv->triangle_colorsel = gcolor3_hsv_new ();
  g_signal_connect (priv->triangle_colorsel, "changed", G_CALLBACK (hsv_changed), colorsel);
  gcolor3_hsv_set_metrics (GCOLOR3_HSV (priv->triangle_colorsel), 174, 15);
  gtk_box_pack_start (GTK_BOX (priv->vbox), priv->triangle_colorsel, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (priv->triangle_colorsel,
			       _("Select the color you want from the outer ring. "
				 "Select the darkness or lightness of that color "
				 "using the inner triangle."));
  gtk_widget_show_all (priv->vbox);

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default()))
    {
      gtk_widget_set_sensitive (priv->picker_button, FALSE);
      gtk_widget_set_tooltip_text (priv->picker_button,
				   _("Picking a color is currently not supported on "
                                   "Wayland."));
    }
  else
#endif
    {
      gtk_widget_set_tooltip_text (priv->picker_button,
                                   _("Click the eyedropper, then click a color "
                                   "anywhere on your screen to select that color."));
    }

  color_sample_setup_dnd (colorsel, priv->old_sample);
  color_sample_setup_dnd (colorsel, priv->cur_sample);

  g_object_set_data (G_OBJECT (priv->adjustment_red), I_("COLORSEL"), colorsel);
  g_signal_connect (priv->adjustment_red, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_RED));
  
  g_object_set_data (G_OBJECT (priv->adjustment_green), I_("COLORSEL"), colorsel);
  g_signal_connect (priv->adjustment_green, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_GREEN));

  g_object_set_data (G_OBJECT (priv->adjustment_blue), I_("COLORSEL"), colorsel);
  g_signal_connect (priv->adjustment_blue, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_BLUE));

  g_object_set_data (G_OBJECT (priv->adjustment_hue), I_("COLORSEL"), colorsel);
  g_signal_connect (priv->adjustment_hue, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_HUE));

  g_object_set_data (G_OBJECT (priv->adjustment_sat), I_("COLORSEL"), colorsel);
  g_signal_connect (priv->adjustment_sat, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_SATURATION));

  g_object_set_data (G_OBJECT (priv->adjustment_val), I_("COLORSEL"), colorsel);
  g_signal_connect (priv->adjustment_val, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_VALUE));

  g_object_set_data (G_OBJECT (priv->adjustment_opacity), I_("COLORSEL"), colorsel);
  g_signal_connect (priv->adjustment_opacity, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_OPACITY));

  focus_chain = g_list_append (focus_chain, priv->hue_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->sat_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->val_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->red_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->green_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->blue_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->opacity_slider);
  focus_chain = g_list_append (focus_chain, priv->opacity_entry);
  focus_chain = g_list_append (focus_chain, priv->hex_entry);
  gtk_container_set_focus_chain (GTK_CONTAINER (priv->table), focus_chain);
  g_list_free (focus_chain);

  atk_obj = gtk_widget_get_accessible (priv->triangle_colorsel);
  if (GTK_IS_ACCESSIBLE (atk_obj))
    {
      atk_object_set_name (atk_obj, _("Color Wheel"));
      atk_object_set_role (gtk_widget_get_accessible (GTK_WIDGET (colorsel)), ATK_ROLE_COLOR_CHOOSER);
      make_all_relations (atk_obj, priv);
    }
}

/* GObject methods */
static void
gcolor3_color_selection_set_property (GObject         *object,
                                      guint            prop_id,
                                      const GValue    *value,
                                      GParamSpec      *pspec)
{
  Gcolor3ColorSelection *colorsel = GCOLOR3_COLOR_SELECTION (object);

  switch (prop_id)
    {
    case PROP_CURRENT_RGBA:
      gcolor3_color_selection_set_current_rgba (colorsel, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
gcolor3_color_selection_get_property (GObject     *object,
                                      guint        prop_id,
                                      GValue      *value,
                                      GParamSpec  *pspec)
{
  Gcolor3ColorSelection *colorsel = GCOLOR3_COLOR_SELECTION (object);

  switch (prop_id)
    {
    case PROP_CURRENT_RGBA:
      {
        GdkRGBA rgba;

        gcolor3_color_selection_get_current_rgba (colorsel, &rgba);
        g_value_set_boxed (value, &rgba);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* GtkWidget methods */

static void
gcolor3_color_selection_destroy (GtkWidget *widget)
{
  Gcolor3ColorSelectionPrivate *priv;

  priv = gcolor3_color_selection_get_instance_private (GCOLOR3_COLOR_SELECTION (widget));

  if (priv->dropper_grab_widget)
    {
      gtk_widget_destroy (priv->dropper_grab_widget);
      priv->dropper_grab_widget = NULL;
    }

  GTK_WIDGET_CLASS (gcolor3_color_selection_parent_class)->destroy (widget);
}

static gboolean
gcolor3_color_selection_grab_broken (GtkWidget                 *widget,
                                     UNUSED GdkEventGrabBroken *event)
{
  shutdown_eyedropper (widget);

  return TRUE;
}

/*
 *
 * The Sample Color
 *
 */

static void color_sample_update_samples (Gcolor3ColorSelection *colorsel);

static void
set_color_internal (Gcolor3ColorSelection *colorsel,
                    GdkRGBA               *color)
{
  Gcolor3ColorSelectionPrivate *priv;
  gint i;

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  priv->changing = TRUE;
  priv->color[COLORSEL_RED] = color->red;
  priv->color[COLORSEL_GREEN] = color->green;
  priv->color[COLORSEL_BLUE] = color->blue;
  priv->color[COLORSEL_OPACITY] = color->alpha;
  gtk_rgb_to_hsv (priv->color[COLORSEL_RED],
                  priv->color[COLORSEL_GREEN],
                  priv->color[COLORSEL_BLUE],
                  &priv->color[COLORSEL_HUE],
                  &priv->color[COLORSEL_SATURATION],
                  &priv->color[COLORSEL_VALUE]);
  if (priv->default_set == FALSE)
    {
      for (i = 0; i < COLORSEL_NUM_CHANNELS; i++)
        priv->old_color[i] = priv->color[i];
    }
  priv->default_set = TRUE;
  update_color (colorsel);
}

static void
set_color_icon (GdkDragContext *context,
                gdouble        *colors)
{
  GdkPixbuf *pixbuf;
  guint32 pixel;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE,
                           8, 48, 32);

  pixel = (((UNSCALE (colors[COLORSEL_RED])   & 0xff00) << 16) |
           ((UNSCALE (colors[COLORSEL_GREEN]) & 0xff00) << 8) |
           ((UNSCALE (colors[COLORSEL_BLUE])  & 0xff00)));

  gdk_pixbuf_fill (pixbuf, pixel);

  gtk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
  g_object_unref (pixbuf);
}

static void
color_sample_drag_begin (GtkWidget      *widget,
                         GdkDragContext *context,
                         gpointer        data)
{
  Gcolor3ColorSelectionPrivate *priv;
  gdouble *colsrc;

  priv = gcolor3_color_selection_get_instance_private (GCOLOR3_COLOR_SELECTION (data));

  if (widget == priv->old_sample)
    colsrc = priv->old_color;
  else
    colsrc = priv->color;

  set_color_icon (context, colsrc);
}

static void
color_sample_drag_end (GtkWidget             *widget,
                       UNUSED GdkDragContext *context,
                       UNUSED gpointer        data)
{
  g_object_set_data (G_OBJECT (widget), I_("gtk-color-selection-drag-window"), NULL);
}

static void
color_sample_drop_handle (GtkWidget             *widget,
                          UNUSED GdkDragContext *context,
                          UNUSED gint            x,
                          UNUSED gint            y,
                          GtkSelectionData      *selection_data,
                          UNUSED guint           info,
                          UNUSED guint           time,
                          gpointer               data)
{
  Gcolor3ColorSelection *colorsel = GCOLOR3_COLOR_SELECTION (data);
  Gcolor3ColorSelectionPrivate *priv;
  GdkRGBA color;
  gint length;
  guint16 *vals;

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  /* This is currently a guint16 array of the format:
   * R
   * G
   * B
   * opacity
   */

  length = gtk_selection_data_get_length (selection_data);

  if (length < 0)
    return;

  /* We accept drops with the wrong format, since the KDE color
   * chooser incorrectly drops application/x-color with format 8.
   */
  if (length != 8)
    {
      g_warning ("Received invalid color data");
      return;
    }

  vals = (guint16 *) gtk_selection_data_get_data (selection_data);

  if (widget == priv->cur_sample)
    {
      color.red = SCALE (vals[0]);
      color.green = SCALE (vals[1]);
      color.blue = SCALE (vals[2]);
      color.alpha = SCALE (vals[3]);

      set_color_internal (colorsel, &color);
    }
}

static void
color_sample_drag_handle (GtkWidget             *widget,
                          UNUSED GdkDragContext *context,
                          GtkSelectionData      *selection_data,
                          UNUSED guint           info,
                          UNUSED guint           time,
                          gpointer               data)
{
  Gcolor3ColorSelectionPrivate *priv;
  guint16 vals[4];
  gdouble *colsrc;

  priv = gcolor3_color_selection_get_instance_private (GCOLOR3_COLOR_SELECTION (data));

  if (widget == priv->old_sample)
    colsrc = priv->old_color;
  else
    colsrc = priv->color;

  vals[0] = UNSCALE (colsrc[COLORSEL_RED]);
  vals[1] = UNSCALE (colsrc[COLORSEL_GREEN]);
  vals[2] = UNSCALE (colsrc[COLORSEL_BLUE]);
  vals[3] = UNSCALE (colsrc[COLORSEL_OPACITY]);

  gtk_selection_data_set (selection_data,
                          gdk_atom_intern_static_string ("application/x-color"),
                          16, (guchar *)vals, 8);
}

static void
color_sample_draw_sample (Gcolor3ColorSelection *colorsel,
			  GtkWidget             *sample,
			  cairo_t               *cr,
			  GdkRGBA               *color,
			  gint                   goff)
{
  Gcolor3ColorSelectionPrivate *priv;
  gint x, y, width, height;

  g_return_if_fail (colorsel != NULL);
  priv = gcolor3_color_selection_get_instance_private (colorsel);

  g_return_if_fail (priv->sample_area != NULL);
  if (!gtk_widget_is_drawable (priv->sample_area))
    return;

  /* Below needs tweaking for non-power-of-two */
  width = gtk_widget_get_allocated_width (sample);
  height = gtk_widget_get_allocated_height (sample);

  /* Draw checks in background */
  cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  cairo_set_source_rgb (cr, 0.75, 0.75, 0.75);
  for (x = goff & -CHECK_SIZE; x < goff + width; x += CHECK_SIZE)
    for (y = 0; y < height; y += CHECK_SIZE)
      if ((x / CHECK_SIZE + y / CHECK_SIZE) % 2 == 0)
	cairo_rectangle (cr, x - goff, y, CHECK_SIZE, CHECK_SIZE);
  cairo_fill (cr);

  cairo_set_source_rgba (cr, color->red, color->green, color->blue, color->alpha);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);
}


static void
color_sample_update_samples (Gcolor3ColorSelection *colorsel)
{
  Gcolor3ColorSelectionPrivate *priv;

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  gtk_widget_queue_draw (priv->old_sample);
  gtk_widget_queue_draw (priv->cur_sample);
}

static gboolean
color_old_sample_draw (UNUSED GtkWidget      *da,
                       cairo_t               *cr,
                       Gcolor3ColorSelection *colorsel)
{
  Gcolor3ColorSelectionPrivate *priv;
  GdkRGBA color;

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  color.red = priv->old_color[COLORSEL_RED];
  color.green = priv->old_color[COLORSEL_GREEN];
  color.blue = priv->old_color[COLORSEL_BLUE];
  color.alpha = priv->old_color[COLORSEL_OPACITY];

  color_sample_draw_sample (colorsel, priv->old_sample, cr, &color, 0);
  return FALSE;
}


static gboolean
color_cur_sample_draw (UNUSED GtkWidget      *da,
                       cairo_t               *cr,
                       Gcolor3ColorSelection *colorsel)
{
  Gcolor3ColorSelectionPrivate *priv;
  GtkAllocation old_sample_allocation;
  GdkRGBA color;
  gint goff;

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  color.red = priv->color[COLORSEL_RED];
  color.green = priv->color[COLORSEL_GREEN];
  color.blue = priv->color[COLORSEL_BLUE];
  color.alpha = priv->color[COLORSEL_OPACITY];

  gtk_widget_get_allocation (priv->old_sample, &old_sample_allocation);
  goff = old_sample_allocation.width % 32;

  color_sample_draw_sample (colorsel, priv->cur_sample, cr, &color, goff);
  return FALSE;
}

static void
color_sample_setup_dnd (Gcolor3ColorSelection *colorsel, GtkWidget *sample)
{
  static const GtkTargetEntry targets[] = {
    { "application/x-color", 0 }
  };
  Gcolor3ColorSelectionPrivate *priv;

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  gtk_drag_source_set (sample,
                       GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
                       targets, 1,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);

  g_signal_connect (sample, "drag-begin",
                    G_CALLBACK (color_sample_drag_begin),
                    colorsel);
  if (sample == priv->cur_sample)
    {

      gtk_drag_dest_set (sample,
                         GTK_DEST_DEFAULT_HIGHLIGHT |
                         GTK_DEST_DEFAULT_MOTION |
                         GTK_DEST_DEFAULT_DROP,
                         targets, 1,
                         GDK_ACTION_COPY);

      g_signal_connect (sample, "drag-end",
                        G_CALLBACK (color_sample_drag_end),
                        colorsel);
    }

  g_signal_connect (sample, "drag-data-get",
                    G_CALLBACK (color_sample_drag_handle),
                    colorsel);
  g_signal_connect (sample, "drag-data-received",
                    G_CALLBACK (color_sample_drop_handle),
                    colorsel);

}

/* The actual Gcolor3ColorSelection widget */

static GdkCursor *
make_picker_cursor (GdkScreen *screen)
{
  GdkCursor *cursor;

  cursor = gdk_cursor_new_from_name (gdk_screen_get_display (screen),
                                     "color-picker");

  if (!cursor)
    {
      GdkPixbuf *pixbuf;

      pixbuf = gdk_pixbuf_new_from_data (dropper_bits,
                                         GDK_COLORSPACE_RGB, TRUE, 8,
                                         DROPPER_WIDTH, DROPPER_HEIGHT,
                                         DROPPER_STRIDE,
                                         NULL, NULL);

      cursor = gdk_cursor_new_from_pixbuf (gdk_screen_get_display (screen),
                                           pixbuf,
                                           DROPPER_X_HOT, DROPPER_Y_HOT);

      g_object_unref (pixbuf);
    }

  return cursor;
}

static void
grab_color_at_pointer (GdkScreen *screen,
                       GdkDevice *device,
                       gint       x_root,
                       gint       y_root,
                       gpointer   data)
{
  Gcolor3ColorSelection *colorsel = GCOLOR3_COLOR_SELECTION (data);
  Gcolor3ColorSelectionPrivate *priv;
  GdkPixbuf *pixbuf;
  guchar *pixels;
  GdkWindow *root_window;

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  root_window = gdk_screen_get_root_window (screen);
  pixbuf = gdk_pixbuf_get_from_window (root_window,
                                       x_root, y_root,
                                       1, 1);
  if (!pixbuf)
    {
      gint x, y;
      GdkWindow *window = gdk_device_get_window_at_position (device, &x, &y);
      if (!window)
        return;
      pixbuf = gdk_pixbuf_get_from_window (window,
                                           x, y,
                                           1, 1);
      if (!pixbuf)
        return;
    }
  pixels = gdk_pixbuf_get_pixels (pixbuf);
  priv->color[COLORSEL_RED] = SCALE(pixels[0] * 0x101);
  priv->color[COLORSEL_GREEN] = SCALE(pixels[1] * 0x101);
  priv->color[COLORSEL_BLUE] = SCALE(pixels[2] * 0x101);
  g_object_unref (pixbuf);

  gtk_rgb_to_hsv (priv->color[COLORSEL_RED],
                  priv->color[COLORSEL_GREEN],
                  priv->color[COLORSEL_BLUE],
                  &priv->color[COLORSEL_HUE],
                  &priv->color[COLORSEL_SATURATION],
                  &priv->color[COLORSEL_VALUE]);

  update_color (colorsel);
}

static void
shutdown_eyedropper (GtkWidget *widget)
{
  Gcolor3ColorSelectionPrivate *priv;

  priv = gcolor3_color_selection_get_instance_private (GCOLOR3_COLOR_SELECTION (widget));

  if (priv->has_grab)
    {
      gdk_device_ungrab (priv->keyboard_device, priv->grab_time);
      gdk_device_ungrab (priv->pointer_device, priv->grab_time);
      gtk_device_grab_remove (priv->dropper_grab_widget, priv->pointer_device);

      priv->has_grab = FALSE;
      priv->keyboard_device = NULL;
      priv->pointer_device = NULL;
    }
}

static void
mouse_motion (UNUSED GtkWidget      *invisible,
              GdkEventMotion        *event,
              gpointer               data)
{
  grab_color_at_pointer (gdk_event_get_screen ((GdkEvent *) event),
                         gdk_event_get_device ((GdkEvent *) event),
                         event->x_root, event->y_root, data);
}

static gboolean
mouse_release (GtkWidget      *invisible,
               GdkEventButton *event,
               gpointer        data)
{
  /* Gcolor3ColorSelection *colorsel = data; */

  if (event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  grab_color_at_pointer (gdk_event_get_screen ((GdkEvent *) event),
                         gdk_event_get_device ((GdkEvent *) event),
                         event->x_root, event->y_root, data);

  shutdown_eyedropper (GTK_WIDGET (data));

  g_signal_handlers_disconnect_by_func (invisible,
                                        mouse_motion,
                                        data);
  g_signal_handlers_disconnect_by_func (invisible,
                                        mouse_release,
                                        data);

  return TRUE;
}

/* Helper Functions */

static gboolean
key_press (GtkWidget   *invisible,
           GdkEventKey *event,
           gpointer     data)
{
  GdkScreen *screen = gdk_event_get_screen ((GdkEvent *) event);
  GdkDevice *device, *pointer_device;
  guint state = event->state & gtk_accelerator_get_default_mod_mask ();
  gint x, y;
  gint dx, dy;

  device = gdk_event_get_device ((GdkEvent * ) event);
  pointer_device = gdk_device_get_associated_device (device);
  gdk_device_get_position (pointer_device, NULL, &x, &y);

  dx = 0;
  dy = 0;

  switch (event->keyval)
    {
    case GDK_KEY_space:
    case GDK_KEY_Return:
    case GDK_KEY_ISO_Enter:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_KP_Space:
      grab_color_at_pointer (screen, pointer_device, x, y, data);
      /* fall through */

    case GDK_KEY_Escape:
      shutdown_eyedropper (data);

      g_signal_handlers_disconnect_by_func (invisible,
                                            mouse_press,
                                            data);
      g_signal_handlers_disconnect_by_func (invisible,
                                            key_press,
                                            data);

      return TRUE;

    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      dy = state == GDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      dy = state == GDK_MOD1_MASK ? BIG_STEP : 1;
      break;

    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
      dx = state == GDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
      dx = state == GDK_MOD1_MASK ? BIG_STEP : 1;
      break;

    default:
      return FALSE;
    }

  gdk_device_warp (pointer_device, screen, x + dx, y + dy);

  return TRUE;

}

static gboolean
mouse_press (GtkWidget      *invisible,
             GdkEventButton *event,
             gpointer        data)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY)
    {
      g_signal_connect (invisible, "motion-notify-event",
                        G_CALLBACK (mouse_motion), data);
      g_signal_connect (invisible, "button-release-event",
                        G_CALLBACK (mouse_release), data);
      g_signal_handlers_disconnect_by_func (invisible,
                                            mouse_press,
                                            data);
      g_signal_handlers_disconnect_by_func (invisible,
                                            key_press,
                                            data);
      return TRUE;
    }

  return FALSE;
}

/* when the button is clicked */
static void
get_screen_color (GtkWidget *button, gpointer user_data)
{
  Gcolor3ColorSelection *colorsel = GCOLOR3_COLOR_SELECTION (user_data);
  Gcolor3ColorSelectionPrivate *priv;
  GdkScreen *screen;
  GdkDevice *device, *keyb_device, *pointer_device;
  GdkCursor *picker_cursor;
  GdkGrabStatus grab_status;
  GdkWindow *window;
  GtkWidget *grab_widget, *toplevel;

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  guint32 time = gtk_get_current_event_time ();

  device = gtk_get_current_event_device ();
  screen = gtk_widget_get_screen (GTK_WIDGET (button));

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      keyb_device = device;
      pointer_device = gdk_device_get_associated_device (device);
    }
  else
    {
      pointer_device = device;
      keyb_device = gdk_device_get_associated_device (device);
    }

  if (priv->dropper_grab_widget == NULL)
    {
      grab_widget = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_screen (GTK_WINDOW (grab_widget), screen);
      gtk_window_resize (GTK_WINDOW (grab_widget), 1, 1);
      gtk_window_move (GTK_WINDOW (grab_widget), -100, -100);
      gtk_widget_show (grab_widget);

      gtk_widget_add_events (grab_widget,
                             GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (colorsel));

      if (GTK_IS_WINDOW (toplevel))
        {
          if (gtk_window_has_group (GTK_WINDOW (toplevel)))
            gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)),
                                         GTK_WINDOW (grab_widget));
        }

      priv->dropper_grab_widget = grab_widget;
    }

  window = gtk_widget_get_window (priv->dropper_grab_widget);

  if (gdk_device_grab (keyb_device,
                       window,
                       GDK_OWNERSHIP_APPLICATION, FALSE,
                       GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK,
                       NULL, time) != GDK_GRAB_SUCCESS)
    return;

  picker_cursor = make_picker_cursor (screen);
  grab_status = gdk_device_grab (pointer_device,
                                 window,
                                 GDK_OWNERSHIP_APPLICATION,
                                 FALSE,
                                 GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK,
                                 picker_cursor,
                                 time);
  g_object_unref (picker_cursor);

  if (grab_status != GDK_GRAB_SUCCESS)
    {
      gdk_device_ungrab (keyb_device, time);
      return;
    }

  gtk_device_grab_add (priv->dropper_grab_widget,
                       pointer_device,
                       TRUE);

  priv->grab_time = time;
  priv->has_grab = TRUE;
  priv->keyboard_device = keyb_device;
  priv->pointer_device = pointer_device;

  g_signal_connect (priv->dropper_grab_widget, "button-press-event",
                    G_CALLBACK (mouse_press), colorsel);
  g_signal_connect (priv->dropper_grab_widget, "key-press-event",
                    G_CALLBACK (key_press), colorsel);
}

static void
hex_changed (UNUSED GtkWidget *hex_entry,
             gpointer          data)
{
  Gcolor3ColorSelection *colorsel = GCOLOR3_COLOR_SELECTION (data);
  Gcolor3ColorSelectionPrivate *priv;
  GdkRGBA color;
  gchar *text;

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  if (priv->changing)
    return;

  text = gtk_editable_get_chars (GTK_EDITABLE (priv->hex_entry), 0, -1);
  if (gdk_rgba_parse (&color, text))
    {
      priv->color[COLORSEL_RED]   = color.red;
      priv->color[COLORSEL_GREEN] = color.green;
      priv->color[COLORSEL_BLUE]  = color.blue;
      gtk_rgb_to_hsv (priv->color[COLORSEL_RED],
                      priv->color[COLORSEL_GREEN],
                      priv->color[COLORSEL_BLUE],
                      &priv->color[COLORSEL_HUE],
                      &priv->color[COLORSEL_SATURATION],
                      &priv->color[COLORSEL_VALUE]);
      update_color (colorsel);
    }
  g_free (text);
}

static gboolean
hex_focus_out (GtkWidget            *hex_entry,
               UNUSED GdkEventFocus *event,
               gpointer              data)
{
  hex_changed (hex_entry, data);

  return FALSE;
}

static void
hsv_changed (GtkWidget *hsv,
             gpointer   data)
{
  Gcolor3ColorSelection *colorsel = GCOLOR3_COLOR_SELECTION (data);
  Gcolor3ColorSelectionPrivate *priv;

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  if (priv->changing)
    return;

  gcolor3_hsv_get_color (GCOLOR3_HSV (hsv),
                         &priv->color[COLORSEL_HUE],
                         &priv->color[COLORSEL_SATURATION],
                         &priv->color[COLORSEL_VALUE]);
  gtk_hsv_to_rgb (priv->color[COLORSEL_HUE],
                  priv->color[COLORSEL_SATURATION],
                  priv->color[COLORSEL_VALUE],
                  &priv->color[COLORSEL_RED],
                  &priv->color[COLORSEL_GREEN],
                  &priv->color[COLORSEL_BLUE]);
  update_color (colorsel);
}

static void
adjustment_changed (GtkAdjustment *adjustment,
                    gpointer       data)
{
  Gcolor3ColorSelection *colorsel;
  Gcolor3ColorSelectionPrivate *priv;

  colorsel = GCOLOR3_COLOR_SELECTION (g_object_get_data (G_OBJECT (adjustment), "COLORSEL"));
  priv = gcolor3_color_selection_get_instance_private (colorsel);

  if (priv->changing)
    return;

  switch (GPOINTER_TO_INT (data))
    {
    case COLORSEL_SATURATION:
    case COLORSEL_VALUE:
      priv->color[GPOINTER_TO_INT (data)] = gtk_adjustment_get_value (adjustment) / 100;
      gtk_hsv_to_rgb (priv->color[COLORSEL_HUE],
                      priv->color[COLORSEL_SATURATION],
                      priv->color[COLORSEL_VALUE],
                      &priv->color[COLORSEL_RED],
                      &priv->color[COLORSEL_GREEN],
                      &priv->color[COLORSEL_BLUE]);
      break;
    case COLORSEL_HUE:
      priv->color[GPOINTER_TO_INT (data)] = gtk_adjustment_get_value (adjustment) / 360;
      gtk_hsv_to_rgb (priv->color[COLORSEL_HUE],
                      priv->color[COLORSEL_SATURATION],
                      priv->color[COLORSEL_VALUE],
                      &priv->color[COLORSEL_RED],
                      &priv->color[COLORSEL_GREEN],
                      &priv->color[COLORSEL_BLUE]);
      break;
    case COLORSEL_RED:
    case COLORSEL_GREEN:
    case COLORSEL_BLUE:
      priv->color[GPOINTER_TO_INT (data)] = gtk_adjustment_get_value (adjustment) / 255;

      gtk_rgb_to_hsv (priv->color[COLORSEL_RED],
                      priv->color[COLORSEL_GREEN],
                      priv->color[COLORSEL_BLUE],
                      &priv->color[COLORSEL_HUE],
                      &priv->color[COLORSEL_SATURATION],
                      &priv->color[COLORSEL_VALUE]);
      break;
    default:
      priv->color[GPOINTER_TO_INT (data)] = gtk_adjustment_get_value (adjustment) / 255;
      break;
    }
  update_color (colorsel);
}

static void
opacity_entry_changed (UNUSED GtkWidget *opacity_entry,
                       gpointer          data)
{
  Gcolor3ColorSelection *colorsel = GCOLOR3_COLOR_SELECTION (data);
  Gcolor3ColorSelectionPrivate *priv;
  GtkAdjustment *adj;
  gchar *text;

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  if (priv->changing)
    return;

  text = gtk_editable_get_chars (GTK_EDITABLE (priv->opacity_entry), 0, -1);
  adj = gtk_range_get_adjustment (GTK_RANGE (priv->opacity_slider));
  gtk_adjustment_set_value (adj, g_strtod (text, NULL));

  update_color (colorsel);

  g_free (text);
}

static void
update_color (Gcolor3ColorSelection *colorsel)
{
  Gcolor3ColorSelectionPrivate *priv;
  gchar entryval[12];
  gchar opacity_text[32];
  gchar *ptr;

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  priv->changing = TRUE;
  color_sample_update_samples (colorsel);

  gcolor3_hsv_set_color (GCOLOR3_HSV (priv->triangle_colorsel),
                         priv->color[COLORSEL_HUE],
                         priv->color[COLORSEL_SATURATION],
                         priv->color[COLORSEL_VALUE]);
  gtk_adjustment_set_value (gtk_spin_button_get_adjustment
                            (GTK_SPIN_BUTTON (priv->hue_spinbutton)),
                            scale_round (priv->color[COLORSEL_HUE], 360));
  gtk_adjustment_set_value (gtk_spin_button_get_adjustment
                            (GTK_SPIN_BUTTON (priv->sat_spinbutton)),
                            scale_round (priv->color[COLORSEL_SATURATION], 100));
  gtk_adjustment_set_value (gtk_spin_button_get_adjustment
                            (GTK_SPIN_BUTTON (priv->val_spinbutton)),
                            scale_round (priv->color[COLORSEL_VALUE], 100));
  gtk_adjustment_set_value (gtk_spin_button_get_adjustment
                            (GTK_SPIN_BUTTON (priv->red_spinbutton)),
                            scale_round (priv->color[COLORSEL_RED], 255));
  gtk_adjustment_set_value (gtk_spin_button_get_adjustment
                            (GTK_SPIN_BUTTON (priv->green_spinbutton)),
                            scale_round (priv->color[COLORSEL_GREEN], 255));
  gtk_adjustment_set_value (gtk_spin_button_get_adjustment
                            (GTK_SPIN_BUTTON (priv->blue_spinbutton)),
                            scale_round (priv->color[COLORSEL_BLUE], 255));
  gtk_adjustment_set_value (gtk_range_get_adjustment
                            (GTK_RANGE (priv->opacity_slider)),
                            scale_round (priv->color[COLORSEL_OPACITY], 255));

  g_snprintf (opacity_text, 32, "%.0f", scale_round (priv->color[COLORSEL_OPACITY], 255));
  gtk_entry_set_text (GTK_ENTRY (priv->opacity_entry), opacity_text);

  g_snprintf (entryval, 11, "#%2X%2X%2X",
              (guint) (scale_round (priv->color[COLORSEL_RED], 255)),
              (guint) (scale_round (priv->color[COLORSEL_GREEN], 255)),
              (guint) (scale_round (priv->color[COLORSEL_BLUE], 255)));

  for (ptr = entryval; *ptr; ptr++)
    if (*ptr == ' ')
      *ptr = '0';
  gtk_entry_set_text (GTK_ENTRY (priv->hex_entry), entryval);
  priv->changing = FALSE;

  g_object_ref (colorsel);

  g_signal_emit (colorsel, color_selection_signals[COLOR_CHANGED], 0);

  g_object_freeze_notify (G_OBJECT (colorsel));
  g_object_notify (G_OBJECT (colorsel), "current-rgba");
  g_object_thaw_notify (G_OBJECT (colorsel));

  g_object_unref (colorsel);
}

/**
 * gcolor3_color_selection_new:
 *
 * Creates a new Gcolor3ColorSelection.
 *
 * Returns: a new #Gcolor3ColorSelection
 */
GtkWidget *
gcolor3_color_selection_new (void)
{
  Gcolor3ColorSelection *colorsel;
  Gcolor3ColorSelectionPrivate *priv;
  GdkRGBA color = { 1, 1, 1, 1 };

  colorsel = g_object_new (GCOLOR3_TYPE_COLOR_SELECTION, NULL);
  priv = gcolor3_color_selection_get_instance_private (colorsel);
  set_color_internal (colorsel, &color);

  /* We want to make sure that default_set is FALSE.
   * This way the user can still set it.
   * It gets initialized to FALSE, but `set_color_internal`
   * sets it to TRUE.
   */
  priv->default_set = FALSE;

  return GTK_WIDGET (colorsel);
}

/**
 * gcolor3_color_selection_set_current_rgba:
 * @colorsel: a #Gcolor3ColorSelection
 * @rgba: A #GdkRGBA to set the current color with
 *
 * Sets the current color to be @rgba.
 *
 * The first time this is called, it will also set
 * the original color to be @rgba too.
 *
 * Since: 3.0
 */
void
gcolor3_color_selection_set_current_rgba (Gcolor3ColorSelection *colorsel,
                                          const GdkRGBA         *rgba)
{
  Gcolor3ColorSelectionPrivate *priv;
  gint i;

  g_return_if_fail (GCOLOR3_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (rgba != NULL);

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  priv->changing = TRUE;

  priv->color[COLORSEL_RED] = rgba->red;
  priv->color[COLORSEL_GREEN] = rgba->green;
  priv->color[COLORSEL_BLUE] = rgba->blue;
  priv->color[COLORSEL_OPACITY] = rgba->alpha;

  gtk_rgb_to_hsv (priv->color[COLORSEL_RED],
                  priv->color[COLORSEL_GREEN],
                  priv->color[COLORSEL_BLUE],
                  &priv->color[COLORSEL_HUE],
                  &priv->color[COLORSEL_SATURATION],
                  &priv->color[COLORSEL_VALUE]);

  if (priv->default_set == FALSE)
    {
      for (i = 0; i < COLORSEL_NUM_CHANNELS; i++)
        priv->old_color[i] = priv->color[i];
    }

  priv->default_set = TRUE;
  update_color (colorsel);
}

/**
 * gcolor3_color_selection_get_current_rgba:
 * @colorsel: a #Gcolor3ColorSelection
 * @rgba: (out): a #GdkRGBA to fill in with the current color
 *
 * Sets @rgba to be the current color in the Gcolor3ColorSelection widget.
 *
 * Since: 3.0
 */
void
gcolor3_color_selection_get_current_rgba (Gcolor3ColorSelection *colorsel,
                                          GdkRGBA               *rgba)
{
  Gcolor3ColorSelectionPrivate *priv;

  g_return_if_fail (GCOLOR3_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (rgba != NULL);

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  rgba->red = priv->color[COLORSEL_RED];
  rgba->green = priv->color[COLORSEL_GREEN];
  rgba->blue = priv->color[COLORSEL_BLUE];
  rgba->alpha = priv->color[COLORSEL_OPACITY];
}

/**
 * gcolor3_color_selection_set_previous_rgba:
 * @colorsel: a #Gcolor3ColorSelection
 * @rgba: a #GdkRGBA to set the previous color with
 *
 * Sets the “previous” color to be @rgba.
 *
 * This function should be called with some hesitations,
 * as it might seem confusing to have that color change.
 * Calling gcolor3_color_selection_set_current_rgba() will also
 * set this color the first time it is called.
 *
 * Since: 3.0
 */
void
gcolor3_color_selection_set_previous_rgba (Gcolor3ColorSelection *colorsel,
                                           const GdkRGBA         *rgba)
{
  Gcolor3ColorSelectionPrivate *priv;

  g_return_if_fail (GCOLOR3_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (rgba != NULL);

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  priv->changing = TRUE;

  priv->old_color[COLORSEL_RED] = rgba->red;
  priv->old_color[COLORSEL_GREEN] = rgba->green;
  priv->old_color[COLORSEL_BLUE] = rgba->blue;
  priv->old_color[COLORSEL_OPACITY] = rgba->alpha;

  gtk_rgb_to_hsv (priv->old_color[COLORSEL_RED],
                  priv->old_color[COLORSEL_GREEN],
                  priv->old_color[COLORSEL_BLUE],
                  &priv->old_color[COLORSEL_HUE],
                  &priv->old_color[COLORSEL_SATURATION],
                  &priv->old_color[COLORSEL_VALUE]);

  color_sample_update_samples (colorsel);
  priv->default_set = TRUE;
  priv->changing = FALSE;
}

/**
 * gcolor3_color_selection_get_previous_rgba:
 * @colorsel: a #Gcolor3ColorSelection
 * @rgba: (out): a #GdkRGBA to fill in with the original color value
 *
 * Fills @rgba in with the original color value.
 *
 * Since: 3.0
 */
void
gcolor3_color_selection_get_previous_rgba (Gcolor3ColorSelection *colorsel,
                                           GdkRGBA               *rgba)
{
  Gcolor3ColorSelectionPrivate *priv;

  g_return_if_fail (GCOLOR3_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (rgba != NULL);

  priv = gcolor3_color_selection_get_instance_private (colorsel);
  rgba->red = priv->old_color[COLORSEL_RED];
  rgba->green = priv->old_color[COLORSEL_GREEN];
  rgba->blue = priv->old_color[COLORSEL_BLUE];
  rgba->alpha = priv->old_color[COLORSEL_OPACITY];
}

/**
 * gcolor3_color_selection_is_adjusting:
 * @colorsel: a #Gcolor3ColorSelection
 *
 * Gets the current state of the @colorsel.
 *
 * Returns: %TRUE if the user is currently dragging
 *     a color around, and %FALSE if the selection has stopped
 */
gboolean
gcolor3_color_selection_is_adjusting (Gcolor3ColorSelection *colorsel)
{
  Gcolor3ColorSelectionPrivate *priv;

  g_return_val_if_fail (GCOLOR3_IS_COLOR_SELECTION (colorsel), FALSE);

  priv = gcolor3_color_selection_get_instance_private (colorsel);

  return (gcolor3_hsv_is_adjusting (GCOLOR3_HSV (priv->triangle_colorsel)));
}

static void
make_control_relations (AtkObject *atk_obj,
                        GtkWidget *widget)
{
  AtkObject *obj;

  obj = gtk_widget_get_accessible (widget);
  atk_object_add_relationship (atk_obj, ATK_RELATION_CONTROLLED_BY, obj);
  atk_object_add_relationship (obj, ATK_RELATION_CONTROLLER_FOR, atk_obj);
}

static void
make_all_relations (AtkObject                *atk_obj,
                    Gcolor3ColorSelectionPrivate *priv)
{
  make_control_relations (atk_obj, priv->hue_spinbutton);
  make_control_relations (atk_obj, priv->sat_spinbutton);
  make_control_relations (atk_obj, priv->val_spinbutton);
  make_control_relations (atk_obj, priv->red_spinbutton);
  make_control_relations (atk_obj, priv->green_spinbutton);
  make_control_relations (atk_obj, priv->blue_spinbutton);
}

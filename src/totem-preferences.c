/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001,2002,2003 Bastien Nocera <hadess@hadess.net>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *
 * The theater project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and theater. This
 * permission are above and beyond the permissions granted by the GPL license
 * theater is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libpeas-gtk/peas-gtk-plugin-manager.h>

#include "bacon-video-widget-enums.h"
#include "theater.h"
#include "theater-private.h"
#include "theater-preferences.h"
#include "theater-interface.h"
#include "theater-subtitle-encoding.h"
#include "theater-plugins-engine.h"

#define PWID(x) (GtkWidget *) gtk_builder_get_object (theater->prefs_xml, x)
#define POBJ(x) gtk_builder_get_object (theater->prefs_xml, x)

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT void checkbutton2_toggled_cb (GtkToggleButton *togglebutton, theater *theater);
G_MODULE_EXPORT void tpw_color_reset_clicked_cb (GtkButton *button, theater *theater);
G_MODULE_EXPORT void font_set_cb (GtkFontButton * fb, theater * theater);
G_MODULE_EXPORT void encoding_set_cb (GtkComboBox *cb, theater *theater);

static void
disable_kbd_shortcuts_changed_cb (GSettings *settings, const gchar *key, theaterObject *theater)
{
	theater->disable_kbd_shortcuts = g_settings_get_boolean (theater->settings, "disable-keyboard-shortcuts");
}

void
tpw_color_reset_clicked_cb (GtkButton *button, theater *theater)
{
	guint i;
	const char *scales[] = {
		"tpw_bright_scale",
		"tpw_contrast_scale",
		"tpw_saturation_scale",
		"tpw_hue_scale"
	};

	for (i = 0; i < G_N_ELEMENTS (scales); i++) {
		GtkRange *item;
		item = GTK_RANGE (POBJ (scales[i]));
		gtk_range_set_value (item, 65535/2);
	}
}

void
font_set_cb (GtkFontButton * fb, theater * theater)
{
	const gchar *font;

	font = gtk_font_button_get_font_name (fb);
	g_settings_set_string (theater->settings, "subtitle-font", font);
}

void
encoding_set_cb (GtkComboBox *cb, theater *theater)
{
	const gchar *encoding;

	encoding = theater_subtitle_encoding_get_selected (cb);
	if (encoding)
		g_settings_set_string (theater->settings, "subtitle-encoding", encoding);
}

static void
font_changed_cb (GSettings *settings, const gchar *key, theaterObject *theater)
{
	gchar *font;
	GtkFontButton *item;

	item = GTK_FONT_BUTTON (POBJ ("font_sel_button"));
	font = g_settings_get_string (settings, "subtitle-font");
	gtk_font_button_set_font_name (item, font);
	bacon_video_widget_set_subtitle_font (theater->bvw, font);
	g_free (font);
}

static void
encoding_changed_cb (GSettings *settings, const gchar *key, theaterObject *theater)
{
	gchar *encoding;
	GtkComboBox *item;

	item = GTK_COMBO_BOX (POBJ ("subtitle_encoding_combo"));
	encoding = g_settings_get_string (settings, "subtitle-encoding");
	theater_subtitle_encoding_set (item, encoding);
	bacon_video_widget_set_subtitle_encoding (theater->bvw, encoding);
	g_free (encoding);
}

static gboolean
int_enum_get_mapping (GValue *value, GVariant *variant, GEnumClass *enum_class)
{
	GEnumValue *enum_value;
	const gchar *nick;

	g_return_val_if_fail (G_IS_ENUM_CLASS (enum_class), FALSE);

	nick = g_variant_get_string (variant, NULL);
	enum_value = g_enum_get_value_by_nick (enum_class, nick);

	if (enum_value == NULL)
		return FALSE;

	g_value_set_int (value, enum_value->value);

	return TRUE;
}

static GVariant *
int_enum_set_mapping (const GValue *value, const GVariantType *expected_type, GEnumClass *enum_class)
{
	GEnumValue *enum_value;

	g_return_val_if_fail (G_IS_ENUM_CLASS (enum_class), NULL);

	enum_value = g_enum_get_value (enum_class, g_value_get_int (value));

	if (enum_value == NULL)
		return NULL;

	return g_variant_new_string (enum_value->value_nick);
}

static gboolean
theater_plugins_window_delete_cb (GtkWidget *window,
				   GdkEventAny *event,
				   gpointer data)
{
	gtk_widget_hide (window);

	return TRUE;
}

static void
theater_plugins_response_cb (GtkDialog *dialog,
			      int response_id,
			      gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
plugin_button_clicked_cb (GtkButton *button,
			  theater     *theater)
{
	if (theater->plugins == NULL) {
		GtkWidget *manager;

		theater->plugins = gtk_dialog_new_with_buttons (_("Configure Plugins"),
							      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
							      GTK_DIALOG_DESTROY_WITH_PARENT,
							      _("_Close"),
							      GTK_RESPONSE_CLOSE,
							      NULL);
		gtk_window_set_modal (GTK_WINDOW (theater->plugins), TRUE);
		gtk_container_set_border_width (GTK_CONTAINER (theater->plugins), 5);
		gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (theater->plugins))), 2);

		g_signal_connect_object (G_OBJECT (theater->plugins),
					 "delete_event",
					 G_CALLBACK (theater_plugins_window_delete_cb),
					 NULL, 0);
		g_signal_connect_object (G_OBJECT (theater->plugins),
					 "response",
					 G_CALLBACK (theater_plugins_response_cb),
					 NULL, 0);

		manager = peas_gtk_plugin_manager_new (NULL);
		gtk_widget_show_all (GTK_WIDGET (manager));
		gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (theater->plugins))),
				    manager, TRUE, TRUE, 0);
		gtk_window_set_default_size (GTK_WINDOW (theater->plugins), 600, 400);
	}

	gtk_window_present (GTK_WINDOW (theater->plugins));
}

void
theater_setup_preferences (theater *theater)
{
	GtkWidget *bvw;
	guint i, hidden;
	char *font, *encoding;
	GObject *item;

	static struct {
		const char *name;
		BvwVideoProperty prop;
		const char *label;
		const gchar *key;
		const gchar *adjustment;
	} props[4] = {
		{ "tpw_contrast_scale", BVW_VIDEO_CONTRAST, "tpw_contrast_label", "contrast", "tpw_contrast_adjustment" },
		{ "tpw_saturation_scale", BVW_VIDEO_SATURATION, "tpw_saturation_label", "saturation", "tpw_saturation_adjustment" },
		{ "tpw_bright_scale", BVW_VIDEO_BRIGHTNESS, "tpw_brightness_label", "brightness", "tpw_bright_adjustment" },
		{ "tpw_hue_scale", BVW_VIDEO_HUE, "tpw_hue_label", "hue", "tpw_hue_adjustment" }
	};

	g_return_if_fail (theater->settings != NULL);

	bvw = theater_object_get_video_widget (theater);

	theater->prefs = PWID ("theater_preferences_window");
	g_signal_connect (G_OBJECT (theater->prefs), "response",
			G_CALLBACK (gtk_widget_hide), NULL);
	g_signal_connect (G_OBJECT (theater->prefs), "delete-event",
			G_CALLBACK (gtk_widget_hide_on_delete), NULL);
        g_signal_connect (theater->prefs, "destroy",
                          G_CALLBACK (gtk_widget_destroyed), &theater->prefs);

	/* Disable deinterlacing */
	item = POBJ ("tpw_no_deinterlace_checkbutton");
	g_settings_bind (theater->settings, "disable-deinterlacing", item, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (theater->settings, "disable-deinterlacing", bvw, "deinterlacing",
	                 G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY | G_SETTINGS_BIND_INVERT_BOOLEAN);

	/* Auto-load subtitles */
	item = POBJ ("tpw_auto_subtitles_checkbutton");
	g_settings_bind (theater->settings, "autoload-subtitles", item, "active", G_SETTINGS_BIND_DEFAULT);

	/* Plugins button */
	item = POBJ ("tpw_plugins_button");
	g_signal_connect (G_OBJECT (item), "clicked",
			  G_CALLBACK (plugin_button_clicked_cb), theater);

	/* Brightness and all */
	hidden = 0;
	for (i = 0; i < G_N_ELEMENTS (props); i++) {
		int prop_value;

		item = POBJ (props[i].adjustment);
		g_settings_bind (theater->settings, props[i].key, item, "value", G_SETTINGS_BIND_DEFAULT);
		g_settings_bind (theater->settings, props[i].key, bvw, props[i].key, G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY);

		prop_value = bacon_video_widget_get_video_property (theater->bvw, props[i].prop);
		if (prop_value < 0) {
			/* The property's unsupported, so hide the widget and its label */
			item = POBJ (props[i].name);
			gtk_range_set_value (GTK_RANGE (item), (gdouble) 65535/2);
			gtk_widget_hide (GTK_WIDGET (item));
			item = POBJ (props[i].label);
			gtk_widget_hide (GTK_WIDGET (item));
			hidden++;
		}
	}

	/* If all the properties have been hidden, hide their section box */
	if (hidden == G_N_ELEMENTS (props)) {
		item = POBJ ("tpw_bright_contr_vbox");
		gtk_widget_hide (GTK_WIDGET (item));
	}

	/* Sound output type */
	item = POBJ ("tpw_sound_output_combobox");
	g_settings_bind (theater->settings, "audio-output-type", bvw, "audio-output-type",
	                 G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY);
	g_settings_bind_with_mapping (theater->settings, "audio-output-type", item, "active", G_SETTINGS_BIND_DEFAULT,
	                              (GSettingsBindGetMapping) int_enum_get_mapping, (GSettingsBindSetMapping) int_enum_set_mapping,
	                              g_type_class_ref (BVW_TYPE_AUDIO_OUTPUT_TYPE), (GDestroyNotify) g_type_class_unref);

	/* Subtitle font selection */
	item = POBJ ("font_sel_button");
	gtk_font_button_set_title (GTK_FONT_BUTTON (item),
				   _("Select Subtitle Font"));
	font = g_settings_get_string (theater->settings, "subtitle-font");
	if (*font != '\0') {
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (item), font);
		bacon_video_widget_set_subtitle_font (theater->bvw, font);
	}
	g_free (font);
	g_signal_connect (theater->settings, "changed::subtitle-font", (GCallback) font_changed_cb, theater);

	/* Subtitle encoding selection */
	item = POBJ ("subtitle_encoding_combo");
	theater_subtitle_encoding_init (GTK_COMBO_BOX (item));
	encoding = g_settings_get_string (theater->settings, "subtitle-encoding");
	/* Make sure the default is UTF-8 */
	if (*encoding == '\0') {
		g_free (encoding);
		encoding = g_strdup ("UTF-8");
	}
	theater_subtitle_encoding_set (GTK_COMBO_BOX(item), encoding);
	if (encoding && strcasecmp (encoding, "") != 0) {
		bacon_video_widget_set_subtitle_encoding (theater->bvw, encoding);
	}
	g_free (encoding);
	g_signal_connect (theater->settings, "changed::subtitle-encoding", (GCallback) encoding_changed_cb, theater);

	/* Disable keyboard shortcuts */
	theater->disable_kbd_shortcuts = g_settings_get_boolean (theater->settings, "disable-keyboard-shortcuts");
	g_signal_connect (theater->settings, "changed::disable-keyboard-shortcuts", (GCallback) disable_kbd_shortcuts_changed_cb, theater);

	g_object_unref (bvw);
}

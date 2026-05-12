/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Philip Withnall <philip@tecnocode.co.uk>
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
 * The theater project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and theater. This
 * permission are above and beyond the permissions granted by the GPL license
 * theater is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Bastien Nocera <hadess@hadess.net>, Philip Withnall <philip@tecnocode.co.uk>
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <libpeas/peas-activatable.h>

#include "theater-plugin.h"
#include "theater-skipto.h"

#define theater_TYPE_SKIPTO_PLUGIN		(theater_skipto_plugin_get_type ())
#define theater_SKIPTO_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), theater_TYPE_SKIPTO_PLUGIN, theaterSkiptoPlugin))
#define theater_SKIPTO_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), theater_TYPE_SKIPTO_PLUGIN, theaterSkiptoPluginClass))
#define theater_IS_SKIPTO_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), theater_TYPE_SKIPTO_PLUGIN))
#define theater_IS_SKIPTO_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), theater_TYPE_SKIPTO_PLUGIN))
#define theater_SKIPTO_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), theater_TYPE_SKIPTO_PLUGIN, theaterSkiptoPluginClass))

typedef struct {
	theaterObject	*theater;
	theaterSkipto	*st;
	guint		handler_id_stream_length;
	guint		handler_id_seekable;
	guint		handler_id_key_press;
	GSimpleAction  *action;
} theaterSkiptoPluginPrivate;

theater_PLUGIN_REGISTER(theater_TYPE_SKIPTO_PLUGIN, theaterSkiptoPlugin, theater_skipto_plugin)

static void
destroy_dialog (theaterSkiptoPlugin *plugin)
{
	theaterSkiptoPluginPrivate *priv = plugin->priv;

	if (priv->st != NULL) {
		g_object_remove_weak_pointer (G_OBJECT (priv->st),
					      (gpointer *)&(priv->st));
		gtk_widget_destroy (GTK_WIDGET (priv->st));
		priv->st = NULL;
	}
}

static void
theater_skipto_update_from_state (theaterObject *theater,
				theaterSkiptoPlugin *plugin)
{
	gint64 _time;
	gboolean seekable;
	theaterSkiptoPluginPrivate *priv = plugin->priv;

	g_object_get (G_OBJECT (theater),
				"stream-length", &_time,
				"seekable", &seekable,
				NULL);

	if (priv->st != NULL) {
		theater_skipto_update_range (priv->st, _time);
		theater_skipto_set_seekable (priv->st, seekable);
	}

	/* Update the action's sensitivity */
	g_simple_action_set_enabled (G_SIMPLE_ACTION (priv->action), seekable);
}

static void
property_notify_cb (theaterObject *theater,
		    GParamSpec *spec,
		    theaterSkiptoPlugin *plugin)
{
	theater_skipto_update_from_state (theater, plugin);
}

static void
skip_to_response_callback (GtkDialog *dialog, gint response, theaterSkiptoPlugin *plugin)
{
	if (response != GTK_RESPONSE_OK) {
		destroy_dialog (plugin);
		return;
	}

	gtk_widget_hide (GTK_WIDGET (dialog));

	theater_object_seek_time (plugin->priv->theater,
				theater_skipto_get_range (plugin->priv->st),
				TRUE);
	destroy_dialog (plugin);
}

static void
run_skip_to_dialog (theaterSkiptoPlugin *plugin)
{
	theaterSkiptoPluginPrivate *priv = plugin->priv;

	if (theater_object_is_seekable (priv->theater) == FALSE)
		return;

	if (priv->st != NULL) {
		gtk_window_present (GTK_WINDOW (priv->st));
		theater_skipto_set_current (priv->st, theater_object_get_current_time
					  (priv->theater));
		return;
	}

	priv->st = theater_SKIPTO (theater_skipto_new (priv->theater));
	g_signal_connect (G_OBJECT (priv->st), "delete-event",
			  G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (G_OBJECT (priv->st), "response",
			  G_CALLBACK (skip_to_response_callback), plugin);
	g_object_add_weak_pointer (G_OBJECT (priv->st),
				   (gpointer *)&(priv->st));
	theater_skipto_update_from_state (priv->theater, plugin);
	theater_skipto_set_current (priv->st,
				  theater_object_get_current_time (priv->theater));
}

static void
skip_to_action_callback (GSimpleAction     *action,
			 GVariant          *parameter,
			 theaterSkiptoPlugin *plugin)
{
	run_skip_to_dialog (plugin);
}

static gboolean
on_window_key_press_event (GtkWidget *window, GdkEventKey *event, theaterSkiptoPlugin *plugin)
{

	if (event->state == 0 || !(event->state & GDK_CONTROL_MASK))
		return FALSE;

	switch (event->keyval) {
		case GDK_KEY_k:
		case GDK_KEY_K:
			run_skip_to_dialog (plugin);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

static void
impl_activate (PeasActivatable *plugin)
{
	GtkWindow *window;
	theaterSkiptoPlugin *pi = theater_SKIPTO_PLUGIN (plugin);
	theaterSkiptoPluginPrivate *priv = pi->priv;
	GMenu *menu;
	GMenuItem *item;

	priv->theater = g_object_get_data (G_OBJECT (plugin), "object");
	priv->handler_id_stream_length = g_signal_connect (G_OBJECT (priv->theater),
				"notify::stream-length",
				G_CALLBACK (property_notify_cb),
				pi);
	priv->handler_id_seekable = g_signal_connect (G_OBJECT (priv->theater),
				"notify::seekable",
				G_CALLBACK (property_notify_cb),
				pi);

	/* Key press handler */
	window = theater_object_get_main_window (priv->theater);
	priv->handler_id_key_press = g_signal_connect (G_OBJECT(window),
				"key-press-event",
				G_CALLBACK (on_window_key_press_event),
				pi);
	g_object_unref (window);

	/* Install the menu */
	priv->action = g_simple_action_new ("skip-to", NULL);
	g_signal_connect (G_OBJECT (priv->action), "activate",
			  G_CALLBACK (skip_to_action_callback), plugin);
	g_action_map_add_action (G_ACTION_MAP (priv->theater), G_ACTION (priv->action));

	menu = theater_object_get_menu_section (priv->theater, "skipto-placeholder");
	item = g_menu_item_new (_("_Skip To…"), "app.skip-to");
	g_menu_item_set_attribute (item, "accel", "s", "<Ctrl>K");
	g_menu_append_item (G_MENU (menu), item);

	theater_skipto_update_from_state (priv->theater, pi);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	GtkWindow *window;
	theaterObject *theater;
	theaterSkiptoPluginPrivate *priv = theater_SKIPTO_PLUGIN (plugin)->priv;

	theater = g_object_get_data (G_OBJECT (plugin), "object");

	g_signal_handler_disconnect (G_OBJECT (theater),
				     priv->handler_id_stream_length);
	g_signal_handler_disconnect (G_OBJECT (theater),
				     priv->handler_id_seekable);

	if (priv->handler_id_key_press != 0) {
		window = theater_object_get_main_window (theater);
		g_signal_handler_disconnect (G_OBJECT(window),
					     priv->handler_id_key_press);
		priv->handler_id_key_press = 0;
		g_object_unref (window);
	}

	/* Remove the menu */
	theater_object_empty_menu_section (priv->theater, "skipto-placeholder");

	destroy_dialog (theater_SKIPTO_PLUGIN (plugin));
}


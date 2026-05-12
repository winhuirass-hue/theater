/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) Bastien Nocera 2019 <hadess@hadess.net>
 * Copyright (C) Simon Wenner 2011 <simon@wenner.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include "config.h"

#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>
#include <errno.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>

#include "theater-plugin.h"
#include "theater-interface.h"
#include "backend/bacon-video-widget.h"

#define theater_TYPE_ROTATION_PLUGIN		(theater_rotation_plugin_get_type ())
#define theater_ROTATION_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), theater_TYPE_ROTATION_PLUGIN, theaterRotationPlugin))
#define theater_ROTATION_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), theater_TYPE_ROTATION_PLUGIN, theaterRotationPluginClass))
#define theater_IS_ROTATION_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), theater_TYPE_ROTATION_PLUGIN))
#define theater_IS_ROTATION_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), theater_TYPE_ROTATION_PLUGIN))
#define theater_ROTATION_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), theater_TYPE_ROTATION_PLUGIN, theaterRotationPluginClass))

#define GIO_ROTATION_FILE_ATTRIBUTE "metadata::theater::rotation"
#define STATE_COUNT 4

typedef struct {
	theaterObject *theater;
	GtkWidget   *bvw;

	GCancellable *cancellable;
	GSimpleAction *rotate_left_action;
	GSimpleAction *rotate_right_action;
} theaterRotationPluginPrivate;

theater_PLUGIN_REGISTER(theater_TYPE_ROTATION_PLUGIN, theaterRotationPlugin, theater_rotation_plugin)

static void
store_state_cb (GObject      *source_object,
		GAsyncResult *res,
		gpointer      user_data)
{
	GError *error = NULL;

	if (!g_file_set_attributes_finish (G_FILE (source_object), res, NULL, &error)) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED) &&
		    !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_warning ("Could not store file attribute: %s", error->message);
		}
		g_error_free (error);
	}
}

static void
store_state (theaterRotationPlugin *pi)
{
	theaterRotationPluginPrivate *priv = pi->priv;
	BvwRotation rotation;
	char *rotation_s;
	GFileInfo *info;
	char *mrl;
	GFile *file;

	rotation = bacon_video_widget_get_rotation (BACON_VIDEO_WIDGET (priv->bvw));
	rotation_s = g_strdup_printf ("%u", rotation);
	info = g_file_info_new ();
	g_file_info_set_attribute_string (info, GIO_ROTATION_FILE_ATTRIBUTE, rotation_s);
	g_free (rotation_s);

	mrl = theater_object_get_current_mrl (priv->theater);
	file = g_file_new_for_uri (mrl);
	g_free (mrl);
	g_file_set_attributes_async (file,
				     info,
				     G_FILE_QUERY_INFO_NONE,
				     G_PRIORITY_DEFAULT,
				     priv->cancellable,
				     store_state_cb,
				     pi);
	g_object_unref (file);
}

static void
restore_state_cb (GObject      *source_object,
		  GAsyncResult *res,
		  gpointer      user_data)
{
	theaterRotationPlugin *pi;
	GError *error = NULL;
	GFileInfo *info;
	const char *rotation_s;
	BvwRotation rotation;

	info = g_file_query_info_finish (G_FILE (source_object), res, &error);
	if (info == NULL) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED) &&
		    !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_warning ("Could not query file attribute: %s", error->message);
		}
		g_error_free (error);
		return;
	}

	pi = user_data;

	rotation_s = g_file_info_get_attribute_string (info, GIO_ROTATION_FILE_ATTRIBUTE);
	if (!rotation_s)
		goto out;

	rotation = atoi (rotation_s);
	bacon_video_widget_set_rotation (BACON_VIDEO_WIDGET (pi->priv->bvw), rotation);

out:
	g_object_unref (info);
}

static void
restore_state (theaterRotationPlugin *pi)
{
	theaterRotationPluginPrivate *priv = pi->priv;
	char *mrl;
	GFile *file;

	mrl = theater_object_get_current_mrl (priv->theater);
	file = g_file_new_for_uri (mrl);
	g_free (mrl);

	g_file_query_info_async (file,
				 GIO_ROTATION_FILE_ATTRIBUTE,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_DEFAULT,
				 priv->cancellable,
				 restore_state_cb,
				 pi);
	g_object_unref (file);
}

static void
update_state (theaterRotationPlugin *pi,
	      const char          *mrl)
{
	theaterRotationPluginPrivate *priv = pi->priv;

	if (mrl == NULL) {
		bacon_video_widget_set_rotation (BACON_VIDEO_WIDGET (priv->bvw),
					 BVW_ROTATION_R_ZERO);
		g_simple_action_set_enabled (priv->rotate_left_action, FALSE);
		g_simple_action_set_enabled (priv->rotate_right_action, FALSE);
	} else {
		g_simple_action_set_enabled (priv->rotate_left_action, TRUE);
		g_simple_action_set_enabled (priv->rotate_right_action, TRUE);
		restore_state (pi);
	}
}

static void
cb_rotate_left (GSimpleAction *simple,
		GVariant      *parameter,
		gpointer       user_data)
{
	theaterRotationPlugin *pi = user_data;
	theaterRotationPluginPrivate *priv = pi->priv;
        int state;

        state = (bacon_video_widget_get_rotation (BACON_VIDEO_WIDGET (priv->bvw)) - 1) % STATE_COUNT;
        bacon_video_widget_set_rotation (BACON_VIDEO_WIDGET (priv->bvw), state);
        store_state (pi);
}

static void
cb_rotate_right (GSimpleAction *simple,
		 GVariant      *parameter,
		 gpointer       user_data)
{
	theaterRotationPlugin *pi = user_data;
	theaterRotationPluginPrivate *priv = pi->priv;
        int state;

        state = (bacon_video_widget_get_rotation (BACON_VIDEO_WIDGET (priv->bvw)) + 1) % STATE_COUNT;
        bacon_video_widget_set_rotation (BACON_VIDEO_WIDGET (priv->bvw), state);
        store_state (pi);
}

static void
theater_rotation_file_closed (theaterObject *theater,
			    theaterRotationPlugin *pi)
{
	update_state (pi, NULL);
}

static void
theater_rotation_file_opened (theaterObject *theater,
			    const char *mrl,
			    theaterRotationPlugin *pi)
{
	update_state (pi, mrl);
}

static void
impl_activate (PeasActivatable *plugin)
{
	theaterRotationPlugin *pi = theater_ROTATION_PLUGIN (plugin);
	theaterRotationPluginPrivate *priv = pi->priv;
	GMenu *menu;
	GMenuItem *item;
	char *mrl;

	priv->theater = g_object_get_data (G_OBJECT (plugin), "object");
	priv->bvw = theater_object_get_video_widget (priv->theater);
	priv->cancellable = g_cancellable_new ();

	g_signal_connect (priv->theater,
			  "file-opened",
			  G_CALLBACK (theater_rotation_file_opened),
			  plugin);
	g_signal_connect (priv->theater,
			  "file-closed",
			  G_CALLBACK (theater_rotation_file_closed),
			  plugin);

	/* add UI */
	menu = theater_object_get_menu_section (priv->theater, "rotation-placeholder");

	priv->rotate_left_action = g_simple_action_new ("rotate-left", NULL);
	g_signal_connect (G_OBJECT (priv->rotate_left_action), "activate",
			  G_CALLBACK (cb_rotate_left), pi);
	g_action_map_add_action (G_ACTION_MAP (priv->theater),
				 G_ACTION (priv->rotate_left_action));

	priv->rotate_right_action = g_simple_action_new ("rotate-right", NULL);
	g_signal_connect (G_OBJECT (priv->rotate_right_action), "activate",
			  G_CALLBACK (cb_rotate_right), pi);
	g_action_map_add_action (G_ACTION_MAP (priv->theater),
				 G_ACTION (priv->rotate_right_action));

	item = g_menu_item_new (_("_Rotate ↷"), "app.rotate-right");
	g_menu_item_set_attribute (item, "accel", "s", "<Primary>R");
	g_menu_append_item (G_MENU (menu), item);

	item = g_menu_item_new (_("Rotate ↶"), "app.rotate-left");
	g_menu_item_set_attribute (item, "accel", "s", "<Primary><Shift>R");
	g_menu_append_item (G_MENU (menu), item);

	mrl = theater_object_get_current_mrl (priv->theater);
	update_state (pi, mrl);
	g_free (mrl);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	theaterRotationPlugin *pi = theater_ROTATION_PLUGIN (plugin);
	theaterRotationPluginPrivate *priv = pi->priv;

	if (priv->cancellable != NULL) {
		g_cancellable_cancel (priv->cancellable);
		g_clear_object (&priv->cancellable);
	}

	g_signal_handlers_disconnect_by_func (priv->theater, theater_rotation_file_opened, plugin);
	g_signal_handlers_disconnect_by_func (priv->theater, theater_rotation_file_closed, plugin);

	theater_object_empty_menu_section (priv->theater, "rotation-placeholder");
	g_action_map_remove_action (G_ACTION_MAP (priv->theater), "rotate-left");
	g_action_map_remove_action (G_ACTION_MAP (priv->theater), "rotate-right");

	bacon_video_widget_set_rotation (BACON_VIDEO_WIDGET (priv->bvw),
					 BVW_ROTATION_R_ZERO);

	priv->theater = NULL;
	priv->bvw = NULL;
}

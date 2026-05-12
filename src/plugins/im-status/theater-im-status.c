/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007, 2011 Bastien Nocera <hadess@hadess.net>
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
 * See license_change file for details.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>

#include "theater.h"
#include "theater-interface.h"
#include "theater-plugin.h"

#define theater_TYPE_IM_STATUS_PLUGIN		(theater_im_status_plugin_get_type ())
#define theater_IM_STATUS_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), theater_TYPE_IM_STATUS_PLUGIN, theaterImStatusPlugin))
#define theater_IM_STATUS_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), theater_TYPE_IM_STATUS_PLUGIN, theaterImStatusPluginClass))
#define theater_IS_IM_STATUS_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), theater_TYPE_IM_STATUS_PLUGIN))
#define theater_IS_IM_STATUS_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), theater_TYPE_IM_STATUS_PLUGIN))
#define theater_IM_STATUS_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), theater_TYPE_IM_STATUS_PLUGIN, theaterImStatusPluginClass))

typedef struct {
	guint		handler_id_fullscreen;
	guint		handler_id_playing;
	GCancellable   *cancellable;
	gboolean	idle; /* Whether we're idle */
	GDBusProxy     *proxy;
} theaterImStatusPluginPrivate;

enum {
	STATUS_AVAILABLE = 0,
	STATUS_INVISIBLE = 1,
	STATUS_BUSY      = 2,
	STATUS_IDLE      = 3
};

theater_PLUGIN_REGISTER (theater_TYPE_IM_STATUS_PLUGIN, theaterImStatusPlugin, theater_im_status_plugin);

static void
theater_im_status_set_idleness (theaterImStatusPlugin *pi,
			      gboolean             idle)
{
	GVariant *variant;
	GError *error = NULL;

	if (pi->priv->idle == idle)
		return;

	pi->priv->idle = idle;
	variant = g_dbus_proxy_call_sync (pi->priv->proxy,
					  "SetStatus",
					  g_variant_new ("(u)", idle ? STATUS_BUSY : STATUS_AVAILABLE),
					  G_DBUS_CALL_FLAGS_NONE,
					  -1,
					  NULL,
					  &error);
	if (variant == NULL) {
		g_warning ("Failed to set presence: %s", error->message);
		g_error_free (error);
		return;
	}
	g_variant_unref (variant);
}

static void
theater_im_status_update_from_state (theaterObject         *theater,
				   theaterImStatusPlugin *pi)
{
	/* Session Proxy not ready yet */
	if (pi->priv->proxy == NULL)
		return;

	if (theater_object_is_playing (theater) != FALSE
	    && theater_object_is_fullscreen (theater) != FALSE) {
		theater_im_status_set_idleness (pi, TRUE);
	} else {
		theater_im_status_set_idleness (pi, FALSE);
	}
}

static void
property_notify_cb (theaterObject         *theater,
		    GParamSpec          *spec,
		    theaterImStatusPlugin *plugin)
{
	theater_im_status_update_from_state (theater, plugin);
}

static void
got_proxy_cb (GObject             *source_object,
	      GAsyncResult        *res,
	      theaterImStatusPlugin *pi)
{
	GError *error = NULL;
	theaterObject *theater;

	pi->priv->proxy = g_dbus_proxy_new_for_bus_finish (res, &error);

	g_object_unref (pi->priv->cancellable);
	pi->priv->cancellable = NULL;

	if (pi->priv->proxy == NULL) {
		g_warning ("Failed to contact session manager: %s", error->message);
		g_error_free (error);
		return;
	}
	g_object_get (pi, "object", &theater, NULL);
	theater_im_status_update_from_state (theater, pi);
	g_object_unref (theater);
}

static void
impl_activate (PeasActivatable *plugin)
{
	theaterImStatusPlugin *pi = theater_IM_STATUS_PLUGIN (plugin);
	theaterObject *theater;

	pi->priv->cancellable = g_cancellable_new ();
	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
				  G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
				  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
				  NULL,
				  "org.gnome.SessionManager",
				  "/org/gnome/SessionManager/Presence",
				  "org.gnome.SessionManager.Presence",
				  pi->priv->cancellable,
				  (GAsyncReadyCallback) got_proxy_cb,
				  pi);

	g_object_get (plugin, "object", &theater, NULL);

	pi->priv->handler_id_fullscreen = g_signal_connect (G_OBJECT (theater),
				"notify::fullscreen",
				G_CALLBACK (property_notify_cb),
				pi);
	pi->priv->handler_id_playing = g_signal_connect (G_OBJECT (theater),
				"notify::playing",
				G_CALLBACK (property_notify_cb),
				pi);

	g_object_unref (theater);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	theaterImStatusPlugin *pi = theater_IM_STATUS_PLUGIN (plugin);
	theaterObject *theater;

	/* In flight? */
	if (pi->priv->cancellable != NULL) {
		g_cancellable_cancel (pi->priv->cancellable);
		g_object_unref (pi->priv->cancellable);
		pi->priv->cancellable = NULL;
	}

	if (pi->priv->proxy != NULL) {
		g_object_unref (pi->priv->proxy);
		pi->priv->proxy = NULL;
	}

	g_object_get (plugin, "object", &theater, NULL);

	if (pi->priv->handler_id_fullscreen != 0) {
		g_signal_handler_disconnect (G_OBJECT (theater),
					     pi->priv->handler_id_fullscreen);
		pi->priv->handler_id_fullscreen = 0;
	}
	if (pi->priv->handler_id_playing != 0) {
		g_signal_handler_disconnect (G_OBJECT (theater),
					     pi->priv->handler_id_playing);
		pi->priv->handler_id_playing = 0;
	}

	g_object_unref (theater);
}

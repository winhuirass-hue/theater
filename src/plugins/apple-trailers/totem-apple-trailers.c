/*
 *  Copyright (C) 2012 Bastien Nocera <hadess@hadess.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
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


#include "config.h"

#include <glib-object.h>

#include "theater-plugin.h"
#include "theater.h"

#define theater_TYPE_APPLE_TRAILERS_PLUGIN	(theater_apple_trailers_plugin_get_type ())
#define theater_APPLE_TRAILERS_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), theater_TYPE_APPLE_TRAILERS_PLUGIN, theaterAppleTrailersPlugin))

typedef struct {
	guint signal_id;
	theaterObject *theater;
} theaterAppleTrailersPluginPrivate;

theater_PLUGIN_REGISTER(theater_TYPE_APPLE_TRAILERS_PLUGIN, theaterAppleTrailersPlugin, theater_apple_trailers_plugin)

static char *
get_user_agent_cb (theaterObject *theater,
		   const char  *mrl)
{
	if (g_str_has_prefix (mrl, "http://movies.apple.com") ||
	    g_str_has_prefix (mrl, "http://trailers.apple.com"))
		return g_strdup ("Quicktime/7.2.0");
	return NULL;
}

static void
impl_activate (PeasActivatable *plugin)
{
	theaterAppleTrailersPlugin *pi = theater_APPLE_TRAILERS_PLUGIN (plugin);

	pi->priv->theater = g_object_ref (g_object_get_data (G_OBJECT (plugin), "object"));
	pi->priv->signal_id = g_signal_connect (G_OBJECT (pi->priv->theater), "get-user-agent",
						G_CALLBACK (get_user_agent_cb), NULL);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	theaterAppleTrailersPlugin *pi = theater_APPLE_TRAILERS_PLUGIN (plugin);

	if (pi->priv->signal_id) {
		g_signal_handler_disconnect (pi->priv->theater, pi->priv->signal_id);
		pi->priv->signal_id = 0;
	}

	if (pi->priv->theater) {
		g_object_unref (pi->priv->theater);
		pi->priv->theater = NULL;
	}
}

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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
#include <gmodule.h>
#include <string.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>
#include <bacon-video-widget-properties.h>

#include "theater-plugin.h"
#include "theater.h"
#include "bacon-video-widget.h"

#define theater_TYPE_MOVIE_PROPERTIES_PLUGIN		(theater_movie_properties_plugin_get_type ())
#define theater_MOVIE_PROPERTIES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), theater_TYPE_MOVIE_PROPERTIES_PLUGIN, theaterMoviePropertiesPlugin))
#define theater_MOVIE_PROPERTIES_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), theater_TYPE_MOVIE_PROPERTIES_PLUGIN, theaterMoviePropertiesPluginClass))
#define theater_IS_MOVIE_PROPERTIES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), theater_TYPE_MOVIE_PROPERTIES_PLUGIN))
#define theater_IS_MOVIE_PROPERTIES_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), theater_TYPE_MOVIE_PROPERTIES_PLUGIN))
#define theater_MOVIE_PROPERTIES_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), theater_TYPE_MOVIE_PROPERTIES_PLUGIN, theaterMoviePropertiesPluginClass))

typedef struct {
	GtkWidget     *props;
	GtkWidget     *dialog;
	guint          handler_id_stream_length;
	guint          handler_id_main_page;
	GSimpleAction *props_action;
} theaterMoviePropertiesPluginPrivate;

theater_PLUGIN_REGISTER(theater_TYPE_MOVIE_PROPERTIES_PLUGIN,
		      theaterMoviePropertiesPlugin,
		      theater_movie_properties_plugin)

/* used in update_properties_from_bvw() */
#define UPDATE_FROM_STRING(type, name) \
	do { \
		const char *temp; \
		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), \
						 type, &value); \
		if ((temp = g_value_get_string (&value)) != NULL) { \
			bacon_video_widget_properties_set_label (props, name, \
								 temp); \
		} \
		g_value_unset (&value); \
	} while (0)

#define UPDATE_FROM_INT(type, name, format, empty) \
	do { \
		char *temp; \
		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), \
						 type, &value); \
		if (g_value_get_int (&value) != 0) \
			temp = g_strdup_printf (gettext (format), \
					g_value_get_int (&value)); \
		else \
			temp = g_strdup (empty); \
		bacon_video_widget_properties_set_label (props, name, temp); \
		g_free (temp); \
		g_value_unset (&value); \
	} while (0)

#define UPDATE_FROM_INT2(type1, type2, name, format) \
	do { \
		int x, y; \
		char *temp; \
		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), \
						 type1, &value); \
		x = g_value_get_int (&value); \
		g_value_unset (&value); \
		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), \
						 type2, &value); \
		y = g_value_get_int (&value); \
		g_value_unset (&value); \
		temp = g_strdup_printf (gettext (format), x, y); \
		bacon_video_widget_properties_set_label (props, name, temp); \
		g_free (temp); \
	} while (0)

static void
update_properties_from_bvw (BaconVideoWidgetProperties *props,
				      GtkWidget *widget)
{
	GValue value = { 0, };
	gboolean has_video, has_audio;
	BaconVideoWidget *bvw;

	g_return_if_fail (BACON_IS_VIDEO_WIDGET_PROPERTIES (props));
	g_return_if_fail (BACON_IS_VIDEO_WIDGET (widget));

	bvw = BACON_VIDEO_WIDGET (widget);

	/* General */
	UPDATE_FROM_STRING (BVW_INFO_TITLE, "title");
	UPDATE_FROM_STRING (BVW_INFO_ARTIST, "artist");
	UPDATE_FROM_STRING (BVW_INFO_ALBUM, "album");
	UPDATE_FROM_STRING (BVW_INFO_YEAR, "year");
	UPDATE_FROM_STRING (BVW_INFO_COMMENT, "comment");
	UPDATE_FROM_STRING (BVW_INFO_CONTAINER, "container");

	bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw),
					 BVW_INFO_DURATION, &value);
	bacon_video_widget_properties_set_duration (props,
						    g_value_get_int (&value) * 1000);
	g_value_unset (&value);

	/* Types */
	bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw),
					 BVW_INFO_HAS_VIDEO, &value);
	has_video = g_value_get_boolean (&value);
	g_value_unset (&value);

	bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw),
					 BVW_INFO_HAS_AUDIO, &value);
	has_audio = g_value_get_boolean (&value);
	g_value_unset (&value);

	bacon_video_widget_properties_set_has_type (props, has_video, has_audio);

	/* Video */
	if (has_video != FALSE)
	{
		UPDATE_FROM_INT2 (BVW_INFO_DIMENSION_X, BVW_INFO_DIMENSION_Y,
				  "dimensions", N_("%d × %d"));
		UPDATE_FROM_STRING (BVW_INFO_VIDEO_CODEC, "vcodec");
		UPDATE_FROM_INT (BVW_INFO_VIDEO_BITRATE, "video_bitrate",
				 N_("%d kbps"), C_("Stream bit rate", "N/A"));

		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), BVW_INFO_FPS, &value);
		bacon_video_widget_properties_set_framerate (props, g_value_get_float (&value));
		g_value_unset (&value);
	}

	/* Audio */
	if (has_audio != FALSE)
	{
		UPDATE_FROM_INT (BVW_INFO_AUDIO_BITRATE, "audio_bitrate",
				 N_("%d kbps"), C_("Stream bit rate", "N/A"));
		UPDATE_FROM_STRING (BVW_INFO_AUDIO_CODEC, "acodec");
		UPDATE_FROM_INT (BVW_INFO_AUDIO_SAMPLE_RATE, "samplerate",
				N_("%d Hz"), C_("Sample rate", "N/A"));
		UPDATE_FROM_STRING (BVW_INFO_AUDIO_CHANNELS, "channels");
	}

#undef UPDATE_FROM_STRING
#undef UPDATE_FROM_INT
#undef UPDATE_FROM_INT2
}

static void
main_page_notify_cb (theaterObject                *theater,
		     GParamSpec                 *arg1,
		     theaterMoviePropertiesPlugin *pi)
{
	char *main_page;

	g_object_get (G_OBJECT (theater), "main-page", &main_page, NULL);
	if (g_strcmp0 (main_page, "player") == 0)
		gtk_widget_hide (pi->priv->dialog);
	g_free (main_page);
}

static void
stream_length_notify_cb (theaterObject *theater,
			 GParamSpec *arg1,
			 theaterMoviePropertiesPlugin *plugin)
{
	gint64 stream_length;

	g_object_get (G_OBJECT (theater),
		      "stream-length", &stream_length,
		      NULL);

	bacon_video_widget_properties_set_duration
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props),
		 stream_length);
}

static void
theater_movie_properties_plugin_file_opened (theaterObject *theater,
					   const char *mrl,
					   theaterMoviePropertiesPlugin *plugin)
{
	GtkWidget *bvw;

	bvw = theater_object_get_video_widget (theater);
	update_properties_from_bvw
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props), bvw);
	g_object_unref (bvw);
	gtk_widget_set_sensitive (plugin->priv->props, TRUE);
}

static void
theater_movie_properties_plugin_file_closed (theaterObject *theater,
					   theaterMoviePropertiesPlugin *plugin)
{
        /* Reset the properties and wait for the signal*/
        bacon_video_widget_properties_reset
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props));
	gtk_widget_set_sensitive (plugin->priv->props, FALSE);
}

static void
theater_movie_properties_plugin_metadata_updated (theaterObject *theater,
						const char *artist, 
						const char *title, 
						const char *album,
						guint track_num,
						theaterMoviePropertiesPlugin *plugin)
{
	GtkWidget *bvw;

	bvw = theater_object_get_video_widget (theater);
	update_properties_from_bvw
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props), bvw);
	g_object_unref (bvw);
}

static void
properties_action_cb (GSimpleAction              *simple,
		      GVariant                   *parameter,
		      theaterMoviePropertiesPlugin *pi)
{
	theaterObject *theater;
	char *main_page;

	theater = g_object_get_data (G_OBJECT (pi), "object");
	g_object_get (G_OBJECT (theater), "main-page", &main_page, NULL);
	if (g_strcmp0 (main_page, "player") == 0)
		gtk_widget_show (pi->priv->dialog);
	g_free (main_page);
}

static void
impl_activate (PeasActivatable *plugin)
{
	theaterMoviePropertiesPlugin *pi;
	theaterObject *theater;
	GtkWindow *parent;
	GMenu *menu;
	GMenuItem *item;
	const char * const accels[] = { "<Primary>p", "View", NULL };

	pi = theater_MOVIE_PROPERTIES_PLUGIN (plugin);
	theater = g_object_get_data (G_OBJECT (plugin), "object");

	pi->priv->props = bacon_video_widget_properties_new ();
	gtk_widget_show (pi->priv->props);
	gtk_widget_set_sensitive (pi->priv->props, FALSE);

	parent = theater_object_get_main_window (theater);
	pi->priv->dialog = gtk_dialog_new_with_buttons (_("Properties"),
							parent,
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
							NULL,
							GTK_RESPONSE_CLOSE,
							NULL);
	g_object_unref (parent);
	g_signal_connect (pi->priv->dialog, "delete-event",
			  G_CALLBACK (gtk_widget_hide_on_delete), NULL);
	g_signal_connect (pi->priv->dialog, "response",
			  G_CALLBACK (gtk_widget_hide_on_delete), NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (pi->priv->dialog))),
			   pi->priv->props);

	/* Properties action */
	pi->priv->props_action = g_simple_action_new ("properties", NULL);
	g_signal_connect (G_OBJECT (pi->priv->props_action), "activate",
			  G_CALLBACK (properties_action_cb), pi);
	g_action_map_add_action (G_ACTION_MAP (theater), G_ACTION (pi->priv->props_action));
	gtk_application_set_accels_for_action (GTK_APPLICATION (theater),
					       "app.properties",
					       accels);

	/* Install the menu */
	menu = theater_object_get_menu_section (theater, "properties-placeholder");
	item = g_menu_item_new (_("_Properties"), "app.properties");
	g_menu_item_set_attribute (item, "accel", "s", "<Primary>p");
	g_menu_append_item (G_MENU (menu), item);
	g_object_unref (item);

	g_signal_connect (G_OBJECT (theater),
			  "file-opened",
			  G_CALLBACK (theater_movie_properties_plugin_file_opened),
			  plugin);
	g_signal_connect (G_OBJECT (theater),
			  "file-closed",
			  G_CALLBACK (theater_movie_properties_plugin_file_closed),
			  plugin);
	g_signal_connect (G_OBJECT (theater),
			  "metadata-updated",
			  G_CALLBACK (theater_movie_properties_plugin_metadata_updated),
			  plugin);
	pi->priv->handler_id_stream_length = g_signal_connect (G_OBJECT (theater),
							       "notify::stream-length",
							       G_CALLBACK (stream_length_notify_cb),
							       plugin);
	pi->priv->handler_id_main_page = g_signal_connect (G_OBJECT (theater),
							   "notify::main-page",
							   G_CALLBACK (main_page_notify_cb),
							   plugin);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	theaterMoviePropertiesPlugin *pi;
	theaterObject *theater;
	const char * const accels[] = { NULL };

	pi = theater_MOVIE_PROPERTIES_PLUGIN (plugin);
	theater = g_object_get_data (G_OBJECT (plugin), "object");

	g_signal_handler_disconnect (G_OBJECT (theater), pi->priv->handler_id_stream_length);
	g_signal_handler_disconnect (G_OBJECT (theater), pi->priv->handler_id_main_page);
	g_signal_handlers_disconnect_by_func (G_OBJECT (theater),
					      theater_movie_properties_plugin_metadata_updated,
					      plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (theater),
					      theater_movie_properties_plugin_file_opened,
					      plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (theater),
					      theater_movie_properties_plugin_file_closed,
					      plugin);
	pi->priv->handler_id_stream_length = 0;
	pi->priv->handler_id_main_page = 0;

	gtk_application_set_accels_for_action (GTK_APPLICATION (theater),
					       "app.properties",
					       accels);
	theater_object_empty_menu_section (theater, "properties-placeholder");
}

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
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

/**
 * SECTION:theater-object
 * @short_description: main theater object
 * @stability: Unstable
 * @include: theater.h
 *
 * #theaterObject is the core object of theater; a singleton which controls all theater's main functions.
 **/

#include "config.h"

#include <glib-object.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <libgd/gd.h>

#include <string.h>

#include "theater.h"
#include "theater-private.h"
#include "theater-options.h"
#include "theater-plugins-engine.h"
#include "theater-playlist.h"
#include "theater-grilo.h"
#include "bacon-video-widget.h"
#include "bacon-time-label.h"
#include "theater-menu.h"
#include "theater-uri.h"
#include "theater-interface.h"
#include "theater-preferences.h"
#include "theater-session.h"
#include "theater-main-toolbar.h"

#define WANT_MIME_TYPES 1
#include "theater-mime-types.h"
#include "theater-uri-schemes.h"

#define REWIND_OR_PREVIOUS 4000

#define SEEK_FORWARD_SHORT_OFFSET 15
#define SEEK_BACKWARD_SHORT_OFFSET -5

#define SEEK_FORWARD_LONG_OFFSET 10*60
#define SEEK_BACKWARD_LONG_OFFSET -3*60

#define DEFAULT_WINDOW_W 650
#define DEFAULT_WINDOW_H 500

#define theater_SESSION_SAVE_TIMEOUT 10 /* seconds */

/* casts are to shut gcc up */
static const GtkTargetEntry target_table[] = {
	{ (gchar*) "text/uri-list", 0, 0 },
	{ (gchar*) "_NETSCAPE_URL", 0, 1 }
};

static gboolean theater_object_open_files_list (theaterObject *theater, GSList *list);
static void update_buttons (theaterObject *theater);
static void update_fill (theaterObject *theater, gdouble level);
static void update_media_menu_items (theaterObject *theater);
static void playlist_changed_cb (GtkWidget *playlist, theaterObject *theater);
static void play_pause_set_label (theaterObject *theater, theaterStates state);
static void theater_object_set_mrl_and_play (theaterObject *theater, const char *mrl, const char *subtitle);

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT gboolean main_window_destroy_cb (GtkWidget *widget, GdkEvent *event, theaterObject *theater);
G_MODULE_EXPORT gboolean window_state_event_cb (GtkWidget *window, GdkEventWindowState *event, theaterObject *theater);
G_MODULE_EXPORT gboolean seek_slider_pressed_cb (GtkWidget *widget, GdkEventButton *event, theaterObject *theater);
G_MODULE_EXPORT void seek_slider_changed_cb (GtkAdjustment *adj, theaterObject *theater);
G_MODULE_EXPORT gboolean seek_slider_released_cb (GtkWidget *widget, GdkEventButton *event, theaterObject *theater);
G_MODULE_EXPORT gboolean window_key_press_event_cb (GtkWidget *win, GdkEventKey *event, theaterObject *theater);

enum {
	PROP_0,
	PROP_FULLSCREEN,
	PROP_PLAYING,
	PROP_STREAM_LENGTH,
	PROP_SEEKABLE,
	PROP_CURRENT_TIME,
	PROP_CURRENT_MRL,
	PROP_CURRENT_CONTENT_TYPE,
	PROP_CURRENT_DISPLAY_NAME,
	PROP_MAIN_PAGE
};

enum {
	FILE_OPENED,
	FILE_CLOSED,
	FILE_HAS_PLAYED,
	METADATA_UPDATED,
	GET_USER_AGENT,
	GET_TEXT_SUBTITLE,
	LAST_SIGNAL
};

static void theater_object_get_property		(GObject *object,
						 guint property_id,
						 GValue *value,
						 GParamSpec *pspec);
static void theater_object_finalize (GObject *theater);

static int theater_table_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(theaterObject, theater_object, GTK_TYPE_APPLICATION)

static void
theater_object_app_open (GApplication  *application,
		       GFile        **files,
		       gint           n_files,
		       const char    *hint)
{
	GSList *slist = NULL;
	theater *theater = theater_OBJECT (application);
	int i;

	optionstate.had_filenames = (n_files > 0);

	g_application_activate (application);
	gtk_window_present_with_time (GTK_WINDOW (theater->win),
				      gtk_get_current_event_time ());

	theater_object_set_main_page (theater_OBJECT (application), "player");

	for (i = 0 ; i < n_files; i++)
		slist = g_slist_prepend (slist, g_file_get_uri (files[i]));

	slist = g_slist_reverse (slist);
	theater_object_open_files_list (theater_OBJECT (application), slist);
	g_slist_free_full (slist, g_free);
}

static void
theater_object_app_activate (GApplication *app)
{
	theater *theater;

	theater = theater_OBJECT (app);

	/* Already init'ed? */
	if (theater->xml != NULL)
		return;

	/* Main window */
	theater->xml = theater_interface_load ("theater.ui", TRUE, NULL, theater);
	if (theater->xml == NULL)
		theater_object_exit (NULL);

	theater->win = GTK_WIDGET (gtk_builder_get_object (theater->xml, "theater_main_window"));

	/* Menubar */
	theater->stack = GTK_WIDGET (gtk_builder_get_object (theater->xml, "tmw_main_stack"));

	/* The playlist widget */
	playlist_widget_setup (theater);

	/* The rest of the widgets */
	theater->state = STATE_STOPPED;

	theater->seek_lock = FALSE;

	theater_setup_file_monitoring (theater);
	theater_setup_file_filters ();
	theater_app_menu_setup (theater);
	/* theater_callback_connect (theater); XXX we do this later now, so it might look ugly for a short while */

	theater_setup_window (theater);

	/* Show ! (again) the video widget this time. */
	video_widget_create (theater);
	grilo_widget_setup (theater);

	/* Show ! */
	gtk_widget_show (theater->win);
	g_application_mark_busy (G_APPLICATION (theater));

	theater->controls_visibility = theater_CONTROLS_UNDEFINED;

	theater->seek = g_object_get_data (theater->controls, "seek_scale");
	theater->seekadj = gtk_range_get_adjustment (GTK_RANGE (theater->seek));
	theater->volume = g_object_get_data (theater->controls, "volume_button");
	theater->time_label = g_object_get_data (theater->controls, "time_label");
	theater->time_rem_label = g_object_get_data (theater->controls, "time_rem_label");
	theater->pause_start = optionstate.pause;

	theater_callback_connect (theater);

	gtk_widget_grab_focus (GTK_WIDGET (theater->bvw));

	/* The prefs after the video widget is connected */
	theater->prefs_xml = theater_interface_load ("preferences.ui", TRUE, NULL, theater);
	theater->prefs = GTK_WIDGET (gtk_builder_get_object (theater->prefs_xml, "theater_preferences_window"));

	gtk_window_set_modal (GTK_WINDOW (theater->prefs), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (theater->prefs), GTK_WINDOW(theater->win));

	theater_setup_preferences (theater);

	/* Initialise all the plugins, and set the default page, in case
	 * it comes from a plugin */
	theater_object_plugins_init (theater);

	/* We're only supposed to be called from theater_object_app_handle_local_options()
	 * and theater_object_app_open() */
	g_assert (optionstate.filenames == NULL);

	if (!optionstate.had_filenames) {
		if (theater_session_try_restore (theater) == FALSE) {
			theater_object_set_main_page (theater, "grilo");
			theater_object_set_mrl (theater, NULL, NULL);
		} else {
			theater_object_set_main_page (theater, "player");
		}
	} else {
		theater_object_set_main_page (theater, "player");
	}

	optionstate.had_filenames = FALSE;

	if (optionstate.fullscreen != FALSE) {
		if (g_strcmp0 (theater_object_get_main_page (theater), "player") == 0)
			theater_object_set_fullscreen (theater, TRUE);
	}

	/* Set the logo at the last minute so we won't try to show it before a video */
	bacon_video_widget_set_logo (theater->bvw, "org.gnome.theater");

	g_application_unmark_busy (G_APPLICATION (theater));

	gtk_window_set_application (GTK_WINDOW (theater->win), GTK_APPLICATION (theater));
}

static int
theater_object_app_handle_local_options (GApplication *application,
				       GVariantDict *options)
{
	GError *error = NULL;

	if (!g_application_register (application, NULL, &error)) {
		g_warning ("Failed to register application: %s", error->message);
		g_error_free (error);
		return 1;
	}
	theater_options_process_for_server (theater_OBJECT (application), &optionstate);
	return 0;
}

static gboolean
accumulator_first_non_null_wins (GSignalInvocationHint *ihint,
				 GValue *return_accu,
				 const GValue *handler_return,
				 gpointer data)
{
	const gchar *str;

	str = g_value_get_string (handler_return);
	if (str == NULL)
		return TRUE;
	g_value_set_string (return_accu, str);

	return FALSE;
}

static void
theater_object_class_init (theaterObjectClass *klass)
{
	GObjectClass *object_class;
	GApplicationClass *app_class;

	object_class = (GObjectClass *) klass;
	app_class = (GApplicationClass *) klass;

	object_class->get_property = theater_object_get_property;
	object_class->finalize = theater_object_finalize;

	app_class->activate = theater_object_app_activate;
	app_class->open = theater_object_app_open;
	app_class->handle_local_options = theater_object_app_handle_local_options;

	/**
	 * theaterObject:fullscreen:
	 *
	 * If %TRUE, theater is in fullscreen mode.
	 **/
	g_object_class_install_property (object_class, PROP_FULLSCREEN,
					 g_param_spec_boolean ("fullscreen", "Fullscreen?", "Whether theater is in fullscreen mode.",
							       FALSE, G_PARAM_READABLE));

	/**
	 * theaterObject:playing:
	 *
	 * If %TRUE, theater is playing an audio or video file.
	 **/
	g_object_class_install_property (object_class, PROP_PLAYING,
					 g_param_spec_boolean ("playing", "Playing?", "Whether theater is currently playing a file.",
							       FALSE, G_PARAM_READABLE));

	/**
	 * theaterObject:stream-length:
	 *
	 * The length of the current stream, in milliseconds.
	 **/
	g_object_class_install_property (object_class, PROP_STREAM_LENGTH,
					 g_param_spec_int64 ("stream-length", "Stream length", "The length of the current stream.",
							     G_MININT64, G_MAXINT64, 0,
							     G_PARAM_READABLE));

	/**
	 * theaterObject:current-time:
	 *
	 * The player's position (time) in the current stream, in milliseconds.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_TIME,
					 g_param_spec_int64 ("current-time", "Current time", "The player's position (time) in the current stream.",
							     G_MININT64, G_MAXINT64, 0,
							     G_PARAM_READABLE));

	/**
	 * theaterObject:seekable:
	 *
	 * If %TRUE, the current stream is seekable.
	 **/
	g_object_class_install_property (object_class, PROP_SEEKABLE,
					 g_param_spec_boolean ("seekable", "Seekable?", "Whether the current stream is seekable.",
							       FALSE, G_PARAM_READABLE));

	/**
	 * theaterObject:current-mrl:
	 *
	 * The MRL of the current stream.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_MRL,
					 g_param_spec_string ("current-mrl", "Current MRL", "The MRL of the current stream.",
							      NULL, G_PARAM_READABLE));

	/**
	 * theaterObject:current-content-type:
	 *
	 * The content-type of the current stream.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_CONTENT_TYPE,
					 g_param_spec_string ("current-content-type",
							      "Current stream's content-type",
							      "Current stream's content-type.",
							      NULL, G_PARAM_READABLE));

	/**
	 * theaterObject:current-display-name:
	 *
	 * The display name of the current stream.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_DISPLAY_NAME,
					 g_param_spec_string ("current-display-name",
							      "Current stream's display name",
							      "Current stream's display name.",
							      NULL, G_PARAM_READABLE));

	/**
	 * theaterObject:main-page:
	 *
	 * The name of the current main page (usually "grilo", or "player").
	 **/
	g_object_class_install_property (object_class, PROP_MAIN_PAGE,
					 g_param_spec_string ("main-page",
							      "Current main page",
							      "Current main page.",
							      NULL, G_PARAM_READABLE));

	/**
	 * theaterObject::file-opened:
	 * @theater: the #theaterObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #theaterObject::file-opened signal is emitted when a new stream is opened by theater.
	 */
	theater_table_signals[FILE_OPENED] =
		g_signal_new ("file-opened",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (theaterObjectClass, file_opened),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);

	/**
	 * theaterObject::file-has-played:
	 * @theater: the #theaterObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #theaterObject::file-has-played signal is emitted when a new stream has started playing in theater.
	 */
	theater_table_signals[FILE_HAS_PLAYED] =
		g_signal_new ("file-has-played",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (theaterObjectClass, file_has_played),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);

	/**
	 * theaterObject::file-closed:
	 * @theater: the #theaterObject which received the signal
	 *
	 * The #theaterObject::file-closed signal is emitted when theater closes a stream.
	 */
	theater_table_signals[FILE_CLOSED] =
		g_signal_new ("file-closed",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (theaterObjectClass, file_closed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0, G_TYPE_NONE);

	/**
	 * theaterObject::metadata-updated:
	 * @theater: the #theaterObject which received the signal
	 * @artist: the name of the artist, or %NULL
	 * @title: the stream title, or %NULL
	 * @album: the name of the stream's album, or %NULL
	 * @track_number: the stream's track number
	 *
	 * The #theaterObject::metadata-updated signal is emitted when the metadata of a stream is updated, typically
	 * when it's being loaded.
	 */
	theater_table_signals[METADATA_UPDATED] =
		g_signal_new ("metadata-updated",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (theaterObjectClass, metadata_updated),
				NULL, NULL,
	                        g_cclosure_marshal_generic,
				G_TYPE_NONE, 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);

	/**
	 * theaterObject::get-user-agent:
	 * @theater: the #theaterObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #theaterObject::get-user-agent signal is emitted before opening a stream, so that plugins
	 * have the opportunity to return the user-agent to be set.
	 *
	 * Return value: allocated string representing the user-agent to use for @mrl
	 */
	theater_table_signals[GET_USER_AGENT] =
		g_signal_new ("get-user-agent",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (theaterObjectClass, get_user_agent),
			      accumulator_first_non_null_wins, NULL,
	                      g_cclosure_marshal_generic,
			      G_TYPE_STRING, 1, G_TYPE_STRING);

	/**
	 * theaterObject::get-text-subtitle:
	 * @theater: the #theaterObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #theaterObject::get-text-subtitle signal is emitted before opening a stream, so that plugins
	 * have the opportunity to detect or download text subtitles for the stream if necessary.
	 *
	 * Return value: allocated string representing the URI of the subtitle to use for @mrl
	 */
	theater_table_signals[GET_TEXT_SUBTITLE] =
		g_signal_new ("get-text-subtitle",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (theaterObjectClass, get_text_subtitle),
			      accumulator_first_non_null_wins, NULL,
	                      g_cclosure_marshal_generic,
			      G_TYPE_STRING, 1, G_TYPE_STRING);
}

static void
theater_object_init (theaterObject *theater)
{
	GtkSettings *gtk_settings;

	if (gtk_clutter_init (NULL, NULL) != CLUTTER_INIT_SUCCESS)
		g_warning ("gtk-clutter failed to initialise, expect problems from here on.");

	gtk_settings = gtk_settings_get_default ();
	g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

	theater->settings = g_settings_new (theater_GSETTINGS_SCHEMA);

	g_application_add_main_option_entries (G_APPLICATION (theater), all_options);
	g_application_add_option_group (G_APPLICATION (theater), bacon_video_widget_get_option_group ());

	theater_app_actions_setup (theater);
}

static void
theater_object_finalize (GObject *object)
{
	theaterObject *theater = theater_OBJECT (object);

	g_clear_pointer (&theater->title, g_free);
	g_clear_pointer (&theater->subtitle, g_free);
	g_clear_pointer (&theater->search_string, g_free);
	g_clear_pointer (&theater->player_title, g_free);
	g_clear_object (&theater->custom_title);

	G_OBJECT_CLASS (theater_object_parent_class)->finalize (object);
}

static void
theater_object_get_property (GObject *object,
			   guint property_id,
			   GValue *value,
			   GParamSpec *pspec)
{
	theaterObject *theater;

	theater = theater_OBJECT (object);

	switch (property_id)
	{
	case PROP_FULLSCREEN:
		g_value_set_boolean (value, theater_object_is_fullscreen (theater));
		break;
	case PROP_PLAYING:
		g_value_set_boolean (value, theater_object_is_playing (theater));
		break;
	case PROP_STREAM_LENGTH:
		g_value_set_int64 (value, bacon_video_widget_get_stream_length (theater->bvw));
		break;
	case PROP_CURRENT_TIME:
		g_value_set_int64 (value, bacon_video_widget_get_current_time (theater->bvw));
		break;
	case PROP_SEEKABLE:
		g_value_set_boolean (value, theater_object_is_seekable (theater));
		break;
	case PROP_CURRENT_MRL:
		g_value_set_string (value, theater->mrl);
		break;
	case PROP_CURRENT_CONTENT_TYPE:
		g_value_take_string (value, theater_playlist_get_current_content_type (theater->playlist));
		break;
	case PROP_CURRENT_DISPLAY_NAME:
		g_value_take_string (value, theater_playlist_get_current_title (theater->playlist));
		break;
	case PROP_MAIN_PAGE:
		g_value_set_string (value, theater_object_get_main_page (theater));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

/**
 * theater_object_plugins_init:
 * @theater: a #theaterObject
 *
 * Initialises the plugin engine and activates all the
 * enabled plugins.
 **/
void
theater_object_plugins_init (theaterObject *theater)
{
	if (theater->engine == NULL)
		theater->engine = theater_plugins_engine_get_default (theater);
}

/**
 * theater_object_plugins_shutdown:
 * @theater: a #theaterObject
 *
 * Shuts down the plugin engine and deactivates all the
 * plugins.
 **/
void
theater_object_plugins_shutdown (theaterObject *theater)
{
	if (theater->engine)
		theater_plugins_engine_shut_down (theater->engine);
	g_clear_object (&theater->engine);
}

/**
 * theater_object_get_main_window:
 * @theater: a #theaterObject
 *
 * Gets theater's main window and increments its reference count.
 *
 * Return value: (transfer full): theater's main window
 **/
GtkWindow *
theater_object_get_main_window (theaterObject *theater)
{
	g_return_val_if_fail (theater_IS_OBJECT (theater), NULL);

	g_object_ref (G_OBJECT (theater->win));

	return GTK_WINDOW (theater->win);
}

/**
 * theater_object_get_menu_section:
 * @theater: a #theaterObject
 * @id: the ID for the menu section to look up
 *
 * Get the #GMenu of the given @id from the main theater #GtkBuilder file.
 *
 * Return value: (transfer none) (nullable): a #GMenu or %NULL on failure
 **/
GMenu *
theater_object_get_menu_section (theaterObject *theater,
			       const char  *id)
{
	GObject *object;
	g_return_val_if_fail (theater_IS_OBJECT (theater), NULL);

	object = gtk_builder_get_object (theater->xml, id);
	if (object == NULL || !G_IS_MENU (object))
		return NULL;

	return G_MENU (object);
}

/**
 * theater_object_empty_menu_section:
 * @theater: a #theaterObject
 * @id: the ID for the menu section to empty
 *
 * Empty the GMenu section pointed to by @id, and remove any
 * related actions. Note that menu items with specific target
 * will not have the associated action removed.
 **/
void
theater_object_empty_menu_section (theaterObject *theater,
				 const char  *id)
{
	GMenu *menu;

	g_return_if_fail (theater_IS_OBJECT (theater));

	menu = G_MENU (gtk_builder_get_object (theater->xml, id));
	g_return_if_fail (menu != NULL);

	while (g_menu_model_get_n_items (G_MENU_MODEL (menu)) > 0) {
		const char *action;
		g_menu_model_get_item_attribute (G_MENU_MODEL (menu), 0, G_MENU_ATTRIBUTE_ACTION, "s", &action);
		if (g_str_has_prefix (action, "app.")) {
			GVariant *target;

			target = g_menu_model_get_item_attribute_value (G_MENU_MODEL (menu), 0, G_MENU_ATTRIBUTE_TARGET, NULL);

			/* Don't remove actions that have a specific target */
			if (target == NULL)
				g_action_map_remove_action (G_ACTION_MAP (theater), action + strlen ("app."));
			else
				g_variant_unref (target);
		}
		g_menu_remove (G_MENU (menu), 0);
	}
}

/**
 * theater_object_get_video_widget:
 * @theater: a #theaterObject
 *
 * Gets theater's video widget and increments its reference count.
 *
 * Return value: (transfer full): theater's video widget
 **/
GtkWidget *
theater_object_get_video_widget (theaterObject *theater)
{
	g_return_val_if_fail (theater_IS_OBJECT (theater), NULL);

	g_object_ref (G_OBJECT (theater->bvw));

	return GTK_WIDGET (theater->bvw);
}

/**
 * theater_object_get_current_time:
 * @theater: a #theaterObject
 *
 * Gets the current position's time in the stream as a gint64.
 *
 * Return value: the current position in the stream
 **/
gint64
theater_object_get_current_time (theaterObject *theater)
{
	g_return_val_if_fail (theater_IS_OBJECT (theater), 0);

	return bacon_video_widget_get_current_time (theater->bvw);
}

static void
add_items_to_playlist_and_play_cb (theaterPlaylist *playlist, GAsyncResult *async_result, theaterObject *theater)
{
	char *mrl, *subtitle;

	/* theater_playlist_add_mrls_finish() never returns an error */
	theater_playlist_add_mrls_finish (playlist, async_result, NULL);

	theater_signal_unblock_by_data (playlist, theater);

	/* And start playback */
	mrl = theater_playlist_get_current_mrl (playlist, &subtitle);
	theater_object_set_mrl_and_play (theater, mrl, subtitle);
	g_free (mrl);
	g_free (subtitle);
}

typedef struct {
	theaterObject *theater;
	gchar *uri;
	gchar *display_name;
	gboolean play;
} AddToPlaylistData;

static void
add_to_playlist_and_play_cb (theaterPlaylist *playlist, GAsyncResult *async_result, AddToPlaylistData *data)
{
	int end = -1;
	gboolean playlist_changed;
	GError *error = NULL;

	playlist_changed = theater_playlist_add_mrl_finish (playlist, async_result, &error);

	if (playlist_changed == FALSE && error != NULL) {
		/* FIXME: Crappy dialogue */
		theater_object_show_error (data->theater, "", error->message);
		g_error_free (error);
	}

	if (data->play)
		end = theater_playlist_get_last (playlist);

	theater_signal_unblock_by_data (playlist, data->theater);

	if (data->play && playlist_changed && end != -1) {
		char *mrl, *subtitle;

		subtitle = NULL;
		theater_playlist_set_current (playlist, end);
		mrl = theater_playlist_get_current_mrl (playlist, &subtitle);
		theater_object_set_mrl_and_play (data->theater, mrl, subtitle);
		g_free (mrl);
		g_free (subtitle);
	}

	/* Free the closure data */
	g_object_unref (data->theater);
	g_free (data->uri);
	g_free (data->display_name);
	g_slice_free (AddToPlaylistData, data);
}

static gboolean
save_session_timeout_cb (theater *theater)
{
	theater_session_save (theater);
	return TRUE;
}

static void
setup_save_timeout_cb (theater    *theater,
		       gboolean  enable)
{
	if (enable && theater->save_timeout_id == 0) {
		theater->save_timeout_id = g_timeout_add_seconds (theater_SESSION_SAVE_TIMEOUT,
								(GSourceFunc) save_session_timeout_cb,
								theater);
		g_source_set_name_by_id (theater->save_timeout_id, "[theater] save_session_timeout_cb");
	} else if (theater->save_timeout_id > 0) {
		g_source_remove (theater->save_timeout_id);
		theater->save_timeout_id = 0;
	}
}

/**
 * theater_object_add_to_playlist:
 * @theater: a #theaterObject
 * @uri: the URI to add to the playlist
 * @display_name: the display name of the URI
 * @play: whether to play the added item
 *
 * Add @uri to the playlist and play it immediately.
 **/
void
theater_object_add_to_playlist (theaterObject *theater,
			      const char  *uri,
			      const char  *display_name,
			      gboolean     play)
{
	AddToPlaylistData *data;

	/* Block all signals from the playlist until we're finished. They're unblocked in the callback, add_to_playlist_and_play_cb.
	 * There are no concurrency issues here, since blocking the signals multiple times should require them to be unblocked the
	 * same number of times before they fire again. */
	theater_signal_block_by_data (theater->playlist, theater);

	data = g_slice_new (AddToPlaylistData);
	data->theater = g_object_ref (theater);
	data->uri = g_strdup (uri);
	data->display_name = g_strdup (display_name);
	data->play = play;

	theater_playlist_add_mrl (theater->playlist, uri, display_name, TRUE,
	                        NULL, (GAsyncReadyCallback) add_to_playlist_and_play_cb, data);
}

/**
 * theater_object_add_items_to_playlist:
 * @theater: a #theaterObject
 * @items: a #GList of #theaterPlaylistMrlData
 *
 * Add @items to the playlist and play them immediately.
 * This function takes ownership of both the list and its elements when
 * called, so don't free either after calling
 * theater_object_add_items_to_playlist().
 **/
void
theater_object_add_items_to_playlist (theaterObject *theater,
				    GList       *items)
{
	/* Block all signals from the playlist until we're finished. They're unblocked in the callback, add_to_playlist_and_play_cb.
	 * There are no concurrency issues here, since blocking the signals multiple times should require them to be unblocked the
	 * same number of times before they fire again. */
	theater_signal_block_by_data (theater->playlist, theater);

	theater_playlist_add_mrls (theater->playlist, items, TRUE, NULL,
				 (GAsyncReadyCallback) add_items_to_playlist_and_play_cb, theater);
}

/**
 * theater_object_clear_playlist:
 * @theater: a #theaterObject
 *
 * Empties the current playlist.
 **/
void
theater_object_clear_playlist (theaterObject *theater)
{
	theater_playlist_clear (theater->playlist);
}

/**
 * theater_object_get_current_mrl:
 * @theater: a #theaterObject
 *
 * Get the MRL of the current stream, or %NULL if nothing's playing.
 * Free with g_free().
 *
 * Return value: a newly-allocated string containing the MRL of the current stream
 **/
char *
theater_object_get_current_mrl (theaterObject *theater)
{
	return theater_playlist_get_current_mrl (theater->playlist, NULL);
}

/**
 * theater_object_get_playlist_length:
 * @theater: a #theaterObject
 *
 * Returns the length of the current playlist.
 *
 * Return value: the playlist length
 **/
guint
theater_object_get_playlist_length (theaterObject *theater)
{
	int last;

	last = theater_playlist_get_last (theater->playlist);
	if (last == -1)
		return 0;
	return last + 1;
}

/**
 * theater_object_get_playlist_pos:
 * @theater: a #theaterObject
 *
 * Returns the <code class="literal">0</code>-based index of the current entry in the playlist. If
 * there is no current entry in the playlist, <code class="literal">-1</code> is returned.
 *
 * Return value: the index of the current playlist entry, or <code class="literal">-1</code>
 **/
int
theater_object_get_playlist_pos (theaterObject *theater)
{
	return theater_playlist_get_current (theater->playlist);
}

/**
 * theater_object_get_title_at_playlist_pos:
 * @theater: a #theaterObject
 * @playlist_index: the <code class="literal">0</code>-based entry index
 *
 * Gets the title of the playlist entry at @index.
 *
 * Return value: the entry title at @index, or %NULL; free with g_free()
 **/
char *
theater_object_get_title_at_playlist_pos (theaterObject *theater, guint playlist_index)
{
	return theater_playlist_get_title (theater->playlist, playlist_index);
}

/**
 * theater_object_get_short_title:
 * @theater: a #theaterObject
 *
 * Gets the title of the current entry in the playlist.
 *
 * Return value: the current entry's title, or %NULL; free with g_free()
 **/
char *
theater_object_get_short_title (theaterObject *theater)
{
	return theater_playlist_get_current_title (theater->playlist);
}

/**
 * theater_object_add_to_view:
 * @theater: a #theaterObject
 * @file: a #GFile representing a media
 * @title: a title for the media, or %NULL
 *
 * Adds a local media file to the main view.
 *
 **/
void
theater_object_add_to_view (theaterObject *theater,
			  GFile       *file,
			  const char  *title)
{
	char *uri;

	uri = g_file_get_uri (file);
	if (!theater_grilo_add_item_to_recent (theater_GRILO (theater->grilo),
					     uri, title, FALSE)) {
		g_warning ("Failed to add '%s' to view", uri);
	}
	g_free (uri);
}

/**
 * theater_object_set_current_subtitle:
 * @theater: a #theaterObject
 * @subtitle_uri: the URI of the subtitle file to add
 *
 * Add the @subtitle_uri subtitle file to the playlist, setting it as the subtitle for the current
 * playlist entry.
 **/
void
theater_object_set_current_subtitle (theaterObject *theater, const char *subtitle_uri)
{
	theater_playlist_set_current_subtitle (theater->playlist, subtitle_uri);
}

void
theater_object_set_main_page (theaterObject *theater,
			    const char  *page_id)
{
	if (g_strcmp0 (page_id, gtk_stack_get_visible_child_name (GTK_STACK (theater->stack))) == 0) {
		if (g_strcmp0 (page_id, "grilo") == 0)
			theater_grilo_start (theater_GRILO (theater->grilo));
		else
			theater_grilo_pause (theater_GRILO (theater->grilo));
		return;
	}

	gtk_stack_set_visible_child_full (GTK_STACK (theater->stack), page_id, GTK_STACK_TRANSITION_TYPE_NONE);

	if (g_strcmp0 (page_id, "player") == 0) {
		theater_grilo_pause (theater_GRILO (theater->grilo));
		g_object_get (theater->header,
			      "title", &theater->title,
			      "subtitle", &theater->subtitle,
			      "search-string", &theater->search_string,
			      "select-mode", &theater->select_mode,
			      "custom-title", &theater->custom_title,
			      NULL);
		g_object_set (theater->header,
			      "show-back-button", TRUE,
			      "show-select-button", FALSE,
			      "show-search-button", FALSE,
			      "title", theater->player_title,
			      "subtitle", NULL,
			      "search-string", NULL,
			      "select-mode", FALSE,
			      "custom-title", NULL,
			      NULL);
		gtk_widget_show (theater->fullscreen_button);
		gtk_widget_show (theater->gear_button);
		gtk_widget_hide (theater->add_button);
		gtk_widget_hide (theater->main_menu_button);
		bacon_video_widget_show_popup (theater->bvw);
	} else if (g_strcmp0 (page_id, "grilo") == 0) {
		theater_grilo_start (theater_GRILO (theater->grilo));
		g_object_set (theater->header,
			      "show-back-button", theater_grilo_get_show_back_button (theater_GRILO (theater->grilo)),
			      "show-select-button", TRUE,
			      "show-search-button", TRUE,
			      "title", theater->title,
			      "subtitle", theater->subtitle,
			      "search-string", theater->search_string,
			      "select-mode", theater->select_mode,
			      "custom-title", theater->custom_title,
			      NULL);
		g_clear_pointer (&theater->title, g_free);
		g_clear_pointer (&theater->subtitle, g_free);
		g_clear_pointer (&theater->search_string, g_free);
		g_clear_pointer (&theater->player_title, g_free);
		g_clear_object (&theater->custom_title);
		gtk_widget_show (theater->main_menu_button);
		gtk_widget_hide (theater->fullscreen_button);
		gtk_widget_hide (theater->gear_button);
		if (theater_grilo_get_current_page (theater_GRILO (theater->grilo)) == theater_GRILO_PAGE_RECENT)
			gtk_widget_show (theater->add_button);
		theater_grilo_start (theater_GRILO (theater->grilo));
	}

	g_object_notify (G_OBJECT (theater), "main-page");
}

/**
 * theater_object_get_main_page:
 * @theater: a #theaterObject
 *
 * Gets the identifier for the current page in theater's
 * main view.
 *
 * Return value: identifier for current page
 */
const char *
theater_object_get_main_page (theater *theater)
{
	return gtk_stack_get_visible_child_name (GTK_STACK (theater->stack));
}

/*
 * emit_file_opened:
 * @theater: a #theaterObject
 * @mrl: the MRL opened
 *
 * Emits the #theaterObject::file-opened signal on @theater, with the
 * specified @mrl.
 **/
static void
emit_file_opened (theaterObject *theater,
		   const char *mrl)
{
	theater_session_save (theater);
	setup_save_timeout_cb (theater, TRUE);
	g_signal_emit (G_OBJECT (theater),
		       theater_table_signals[FILE_OPENED],
		       0, mrl);
}

/*
 * emit_file_closed:
 * @theater: a #theaterObject
 *
 * Emits the #theaterObject::file-closed signal on @theater.
 **/
static void
emit_file_closed (theaterObject *theater)
{
	setup_save_timeout_cb (theater, FALSE);
	theater_session_save (theater);
	g_signal_emit (G_OBJECT (theater),
		       theater_table_signals[FILE_CLOSED],
		       0);
}

/**
 * theater_file_has_played:
 * @theater: a #theaterObject
 *
 * Emits the #theaterObject::file-played signal on @theater.
 **/
void
theater_file_has_played (theaterObject *theater,
		       const char  *mrl)
{
	g_signal_emit (G_OBJECT (theater),
		       theater_table_signals[FILE_HAS_PLAYED],
		       0, mrl);
}

/*
 * emit_metadata_updated:
 * @theater: a #theaterObject
 * @artist: the stream's artist, or %NULL
 * @title: the stream's title, or %NULL
 * @album: the stream's album, or %NULL
 * @track_num: the track number of the stream
 *
 * Emits the #theaterObject::metadata-updated signal on @theater,
 * with the specified stream data.
 **/
static void
emit_metadata_updated (theaterObject *theater,
			const char *artist,
			const char *title,
			const char *album,
			guint track_num)
{
	g_signal_emit (G_OBJECT (theater),
		       theater_table_signals[METADATA_UPDATED],
		       0,
		       artist,
		       title,
		       album,
		       track_num);
}

GQuark
theater_remote_command_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("theater_remote_command");

	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
theater_remote_command_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (theater_REMOTE_COMMAND_UNKNOWN, "unknown"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_PLAY, "play"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_PAUSE, "pause"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_STOP, "stop"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_PLAYPAUSE, "play-pause"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_NEXT, "next"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_PREVIOUS, "previous"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_SEEK_FORWARD, "seek-forward"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_SEEK_BACKWARD, "seek-backward"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_VOLUME_UP, "volume-up"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_VOLUME_DOWN, "volume-down"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_FULLSCREEN, "fullscreen"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_QUIT, "quit"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_ENQUEUE, "enqueue"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_REPLACE, "replace"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_SHOW, "show"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_UP, "up"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_DOWN, "down"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_LEFT, "left"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_RIGHT, "right"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_SELECT, "select"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_DVD_MENU, "dvd-menu"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_ZOOM_UP, "zoom-up"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_ZOOM_DOWN, "zoom-down"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_EJECT, "eject"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_PLAY_DVD, "play-dvd"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_MUTE, "mute"),
			ENUM_ENTRY (theater_REMOTE_COMMAND_TOGGLE_ASPECT, "toggle-aspect-ratio"),
			{ 0, NULL, NULL }
		};

		etype = g_enum_register_static ("theaterRemoteCommand", values);
	}

	return etype;
}

GQuark
theater_remote_setting_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("theater_remote_setting");

	return quark;
}

GType
theater_remote_setting_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (theater_REMOTE_SETTING_REPEAT, "repeat"),
			{ 0, NULL, NULL }
		};

		etype = g_enum_register_static ("theaterRemoteSetting", values);
	}

	return etype;
}

static void
reset_seek_status (theaterObject *theater)
{
	/* Release the lock and reset everything so that we
	 * avoid being "stuck" seeking on errors */

	if (theater->seek_lock != FALSE) {
		theater->seek_lock = FALSE;
		bacon_video_widget_unmark_popup_busy (theater->bvw, "seek started");
		bacon_video_widget_seek (theater->bvw, 0, NULL);
		bacon_video_widget_stop (theater->bvw);
		play_pause_set_label (theater, STATE_STOPPED);
	}
}

/**
 * theater_object_show_error:
 * @theater: a #theaterObject
 * @title: the error dialog title
 * @reason: the error dialog text
 *
 * Displays a non-blocking error dialog with the
 * given @title and @reason.
 **/
void
theater_object_show_error (theaterObject *theater, const char *title, const char *reason)
{
	reset_seek_status (theater);
	theater_interface_error (title, reason,
			GTK_WINDOW (theater->win));
}

G_GNUC_NORETURN void
theater_object_show_error_and_exit (const char *title,
		const char *reason, theaterObject *theater)
{
	reset_seek_status (theater);
	theater_interface_error_blocking (title, reason,
			GTK_WINDOW (theater->win));
	theater_object_exit (theater);
}

static void
theater_object_save_size (theaterObject *theater)
{
	if (theater->bvw == NULL)
		return;

	if (theater_object_is_fullscreen (theater) != FALSE)
		return;

	/* Save the size of the video widget */
	gtk_window_get_size (GTK_WINDOW (theater->win), &theater->window_w, &theater->window_h);
}

static void
theater_object_save_state (theaterObject *theater)
{
	GKeyFile *keyfile;
	char *contents, *filename;

	if (theater->win == NULL)
		return;
	if (theater->window_w == 0
	    || theater->window_h == 0)
		return;

	keyfile = g_key_file_new ();
	g_key_file_set_integer (keyfile, "State",
				"window_w", theater->window_w);
	g_key_file_set_integer (keyfile, "State",
			"window_h", theater->window_h);
	g_key_file_set_boolean (keyfile, "State",
			"maximised", theater->maximised);

	contents = g_key_file_to_data (keyfile, NULL, NULL);
	g_key_file_free (keyfile);
	filename = g_build_filename (theater_dot_dir (), "state.ini", NULL);
	g_file_set_contents (filename, contents, -1, NULL);

	g_free (filename);
	g_free (contents);
}

/**
 * theater_object_exit:
 * @theater: a #theaterObject
 *
 * Closes theater.
 **/
void
theater_object_exit (theaterObject *theater)
{
	GdkDisplay *display = NULL;

	/* Shut down the plugins first, allowing them to display modal dialogues (etc.) without threat of being killed from another thread */
	if (theater != NULL && theater->engine != NULL)
		theater_object_plugins_shutdown (theater);

	if (gtk_main_level () > 0)
		gtk_main_quit ();

	if (theater == NULL)
		exit (0);

	if (theater->bvw)
		theater_object_save_size (theater);

	if (theater->win != NULL) {
		gtk_widget_hide (theater->win);
		display = gtk_widget_get_display (theater->win);
	}

	if (theater->prefs != NULL)
		gtk_widget_hide (theater->prefs);

	if (display != NULL)
		gdk_display_sync (display);

	setup_save_timeout_cb (theater, FALSE);
	theater_session_cleanup (theater);

	if (theater->bvw)
		bacon_video_widget_close (theater->bvw);

	theater_object_save_state (theater);

	theater_sublang_exit (theater);
	theater_destroy_file_filters ();

	g_clear_object (&theater->settings);

	if (theater->win)
		gtk_widget_destroy (GTK_WIDGET (theater->win));

	g_object_unref (theater);

	exit (0);
}

G_GNUC_NORETURN gboolean
main_window_destroy_cb (GtkWidget *widget, GdkEvent *event, theaterObject *theater)
{
	theater_object_exit (theater);
}

static void
play_pause_set_label (theaterObject *theater, theaterStates state)
{
	GtkWidget *image;
	const char *id, *tip;

	if (state == theater->state)
		return;

	switch (state)
	{
	case STATE_PLAYING:
		id = "media-playback-pause-symbolic";
		tip = N_("Pause");
		theater_playlist_set_playing (theater->playlist, theater_PLAYLIST_STATUS_PLAYING);
		break;
	case STATE_PAUSED:
		id = "media-playback-start-symbolic";
		tip = N_("Play");
		theater_playlist_set_playing (theater->playlist, theater_PLAYLIST_STATUS_PAUSED);
		break;
	case STATE_STOPPED:
		bacon_time_label_set_time (theater->time_label,
					   0, 0);
		bacon_time_label_set_time (theater->time_rem_label,
					   0, 0);
		id = "media-playback-start-symbolic";
		theater_playlist_set_playing (theater->playlist, theater_PLAYLIST_STATUS_NONE);
		tip = N_("Play");
		break;
	default:
		g_assert_not_reached ();
		return;
	}

	gtk_widget_set_tooltip_text (theater->play_button, _(tip));
	image = gtk_button_get_image (GTK_BUTTON (theater->play_button));
	gtk_image_set_from_icon_name (GTK_IMAGE (image), id, GTK_ICON_SIZE_MENU);

	theater->state = state;

	g_object_notify (G_OBJECT (theater), "playing");
}

void
theater_object_eject (theaterObject *theater)
{
	GMount *mount;

	mount = theater_get_mount_for_media (theater->mrl);
	if (mount == NULL)
		return;

	g_clear_pointer (&theater->mrl, g_free);
	bacon_video_widget_close (theater->bvw);
	emit_file_closed (theater);
	theater->has_played_emitted = FALSE;

	/* The volume monitoring will take care of removing the items */
	g_mount_eject_with_operation (mount, G_MOUNT_UNMOUNT_NONE, NULL, NULL, NULL, NULL);
	g_object_unref (mount);
}

/**
 * theater_object_play:
 * @theater: a #theaterObject
 *
 * Plays the current stream. If theater is already playing, it continues
 * to play. If the stream cannot be played, and error dialog is displayed.
 **/
void
theater_object_play (theaterObject *theater)
{
	GError *err = NULL;
	int retval;
	char *msg, *disp;

	if (theater->mrl == NULL)
		return;

	if (bacon_video_widget_is_playing (theater->bvw) != FALSE)
		return;

	retval = bacon_video_widget_play (theater->bvw,  &err);
	play_pause_set_label (theater, retval ? STATE_PLAYING : STATE_STOPPED);

	if (retval != FALSE) {
		if (theater->has_played_emitted == FALSE) {
			theater_file_has_played (theater, theater->mrl);
			theater->has_played_emitted = TRUE;
		}
		return;
	}

	disp = theater_uri_escape_for_display (theater->mrl);
	msg = g_strdup_printf(_("theater could not play “%s”."), disp);
	g_free (disp);

	theater_object_show_error (theater, msg, err->message);
	bacon_video_widget_stop (theater->bvw);
	play_pause_set_label (theater, STATE_STOPPED);
	g_free (msg);
	g_error_free (err);
}

static void
theater_object_seek (theaterObject *theater, double pos)
{
	GError *err = NULL;
	int retval;

	if (theater->mrl == NULL)
		return;
	if (bacon_video_widget_is_seekable (theater->bvw) == FALSE)
		return;

	retval = bacon_video_widget_seek (theater->bvw, pos, &err);

	if (retval == FALSE)
	{
		char *msg, *disp;

		disp = theater_uri_escape_for_display (theater->mrl);
		msg = g_strdup_printf(_("theater could not play “%s”."), disp);
		g_free (disp);

		reset_seek_status (theater);

		theater_object_show_error (theater, msg, err->message);
		g_free (msg);
		g_error_free (err);
	}
}

static void
theater_object_set_mrl_and_play (theaterObject *theater, const char *mrl, const char *subtitle)
{
	theater_object_set_mrl (theater, mrl, subtitle);
	theater_object_play (theater);
}

static gboolean
theater_object_open_dialog (theaterObject *theater, const char *path)
{
	GSList *filenames, *l;

	filenames = theater_add_files (GTK_WINDOW (theater->win), path);

	if (filenames == NULL)
		return FALSE;

	for (l = filenames; l != NULL; l = l->next) {
		char *uri = l->data;

		theater_grilo_add_item_to_recent (theater_GRILO (theater->grilo), uri, NULL, FALSE);
		g_free (uri);
	}
	g_slist_free (filenames);

	return TRUE;
}

/**
 * theater_object_play_pause:
 * @theater: a #theaterObject
 *
 * Gets the current MRL from the playlist and attempts to play it.
 * If the stream is already playing, playback is paused.
 **/
void
theater_object_play_pause (theaterObject *theater)
{
	if (theater->mrl == NULL) {
		char *mrl, *subtitle;

		/* Try to pull an mrl from the playlist */
		mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);
		if (mrl == NULL) {
			play_pause_set_label (theater, STATE_STOPPED);
			return;
		} else {
			theater_object_set_mrl_and_play (theater, mrl, subtitle);
			g_free (mrl);
			g_free (subtitle);
			return;
		}
	}

	if (bacon_video_widget_is_playing (theater->bvw) == FALSE) {
		if (bacon_video_widget_play (theater->bvw, NULL) != FALSE &&
		    theater->has_played_emitted == FALSE) {
			theater_file_has_played (theater, theater->mrl);
			theater->has_played_emitted = TRUE;
		}
		play_pause_set_label (theater, STATE_PLAYING);
	} else {
		bacon_video_widget_pause (theater->bvw);
		play_pause_set_label (theater, STATE_PAUSED);
	}
}

/**
 * theater_object_stop:
 * @theater: a #theaterObject
 *
 * Stops playback, and sets the playlist back at the start.
 */
void
theater_object_stop (theaterObject *theater)
{
	char *mrl, *subtitle;

	theater_playlist_set_at_start (theater->playlist);
	update_buttons (theater);
	bacon_video_widget_stop (theater->bvw);
	play_pause_set_label (theater, STATE_STOPPED);
	mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);
	if (mrl != NULL) {
		theater_object_set_mrl (theater, mrl, subtitle);
		bacon_video_widget_pause (theater->bvw);
		g_free (mrl);
		g_free (subtitle);
	}
}

/**
 * theater_object_pause:
 * @theater: a #theaterObject
 *
 * Pauses the current stream. If theater is already paused, it continues
 * to be paused.
 **/
void
theater_object_pause (theaterObject *theater)
{
	if (bacon_video_widget_is_playing (theater->bvw) != FALSE) {
		bacon_video_widget_pause (theater->bvw);
		play_pause_set_label (theater, STATE_PAUSED);
	}
}

gboolean
window_state_event_cb (GtkWidget           *window,
		       GdkEventWindowState *event,
		       theaterObject         *theater)
{
	GAction *action;

	theater->maximised = !!(event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED);

	if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) == 0)
		return FALSE;

	if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
		if (theater->controls_visibility != theater_CONTROLS_UNDEFINED)
			theater_object_save_size (theater);

		theater->controls_visibility = theater_CONTROLS_FULLSCREEN;
		show_controls (theater, FALSE);
	} else {
		theater->controls_visibility = theater_CONTROLS_VISIBLE;
		show_controls (theater, TRUE);
	}

	bacon_video_widget_set_fullscreen (theater->bvw,
					   theater->controls_visibility == theater_CONTROLS_FULLSCREEN);

	action = g_action_map_lookup_action (G_ACTION_MAP (theater), "fullscreen");
	g_simple_action_set_state (G_SIMPLE_ACTION (action),
				   g_variant_new_boolean (theater->controls_visibility == theater_CONTROLS_FULLSCREEN));

	g_object_notify (G_OBJECT (theater), "fullscreen");

	return FALSE;
}

static void
theater_object_action_fullscreen_toggle (theaterObject *theater)
{
	if (theater_object_is_fullscreen (theater) != FALSE)
		gtk_window_unfullscreen (GTK_WINDOW (theater->win));
	else
		gtk_window_fullscreen (GTK_WINDOW (theater->win));
}

/**
 * theater_object_set_fullscreen:
 * @theater: a #theaterObject
 * @state: %TRUE if theater should be fullscreened
 *
 * Sets theater's fullscreen state according to @state.
 **/
void
theater_object_set_fullscreen (theaterObject *theater, gboolean state)
{
	if (theater_object_is_fullscreen (theater) == state)
		return;

	if (state)
		gtk_window_fullscreen (GTK_WINDOW (theater->win));
	else
		gtk_window_unfullscreen (GTK_WINDOW (theater->win));
}

void
theater_object_open (theaterObject *theater)
{
	theater_object_open_dialog (theater, NULL);
}

static void
theater_open_location_response_cb (GtkDialog *dialog, gint response, theaterObject *theater)
{
	char *uri;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (theater->open_location));
		return;
	}

	gtk_widget_hide (GTK_WIDGET (dialog));

	/* Open the specified URI */
	uri = theater_open_location_get_uri (theater->open_location);

	if (uri != NULL) {
		theater_grilo_add_item_to_recent (theater_GRILO (theater->grilo), uri, NULL, TRUE);
		g_free (uri);
	}

	gtk_widget_destroy (GTK_WIDGET (theater->open_location));
}

void
theater_object_open_location (theaterObject *theater)
{
	if (theater->open_location != NULL) {
		gtk_window_present (GTK_WINDOW (theater->open_location));
		return;
	}

	theater->open_location = theater_OPEN_LOCATION (theater_open_location_new ());

	g_signal_connect (G_OBJECT (theater->open_location), "delete-event",
			G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (G_OBJECT (theater->open_location), "response",
			G_CALLBACK (theater_open_location_response_cb), theater);
	g_object_add_weak_pointer (G_OBJECT (theater->open_location), (gpointer *)&(theater->open_location));

	gtk_window_set_transient_for (GTK_WINDOW (theater->open_location),
			GTK_WINDOW (theater->win));
	gtk_widget_show (GTK_WIDGET (theater->open_location));
}

static char *
theater_get_nice_name_for_stream (theaterObject *theater)
{
	GValue title_value = { 0, };
	GValue album_value = { 0, };
	GValue artist_value = { 0, };
	GValue value = { 0, };
	char *retval;
	int tracknum;

	bacon_video_widget_get_metadata (theater->bvw, BVW_INFO_TITLE, &title_value);
	bacon_video_widget_get_metadata (theater->bvw, BVW_INFO_ARTIST, &artist_value);
	bacon_video_widget_get_metadata (theater->bvw, BVW_INFO_ALBUM, &album_value);
	bacon_video_widget_get_metadata (theater->bvw,
					 BVW_INFO_TRACK_NUMBER,
					 &value);

	tracknum = g_value_get_int (&value);
	g_value_unset (&value);

	emit_metadata_updated (theater,
	                       g_value_get_string (&artist_value),
	                       g_value_get_string (&title_value),
	                       g_value_get_string (&album_value),
	                       tracknum);

	if (g_value_get_string (&title_value) == NULL) {
		retval = NULL;
		goto bail;
	}
	if (g_value_get_string (&artist_value) == NULL) {
		retval = g_value_dup_string (&title_value);
		goto bail;
	}

	if (tracknum != 0) {
		retval = g_strdup_printf ("%02d. %s - %s",
					  tracknum,
					  g_value_get_string (&artist_value),
					  g_value_get_string (&title_value));
	} else {
		retval = g_strdup_printf ("%s - %s",
					  g_value_get_string (&artist_value),
					  g_value_get_string (&title_value));
	}

bail:
	g_value_unset (&album_value);
	g_value_unset (&artist_value);
	g_value_unset (&title_value);

	return retval;
}

static void
update_mrl_label (theaterObject *theater, const char *name)
{
	if (name != NULL) {
		/* Update the mrl label */
		g_clear_pointer (&theater->player_title, g_free);
		theater->player_title = g_strdup (name);
	} else {
		bacon_time_label_set_time (theater->time_label,
					   0, 0);
		bacon_time_label_set_time (theater->time_rem_label,
					   0, 0);

		g_object_notify (G_OBJECT (theater), "stream-length");

		/* Update the mrl label */
		g_clear_pointer (&theater->player_title, g_free);
	}

	if (g_strcmp0 (theater_object_get_main_page (theater), "player") == 0)
		g_object_set (theater->header, "title", theater->player_title, NULL);
}

static void
theater_object_set_next_subtitle (theaterObject *theater,
				const char  *subtitle)
{
	g_clear_pointer (&theater->next_subtitle, g_free);
	theater->next_subtitle = g_strdup (subtitle);
}

/**
 * theater_object_set_mrl:
 * @theater: a #theaterObject
 * @mrl: the MRL to play
 * @subtitle: a subtitle file to load, or %NULL
 *
 * Loads the specified @mrl and optionally the specified subtitle
 * file. If @subtitle is %NULL theater will attempt to auto-locate
 * any subtitle files for @mrl.
 *
 * If a stream is already playing, it will be stopped and closed.
 *
 * Errors will be reported asynchronously.
 **/
void
theater_object_set_mrl (theaterObject *theater,
		      const char *mrl,
		      const char *subtitle)
{
	if (theater->mrl != NULL) {
		theater->pause_start = FALSE;

		g_clear_pointer (&theater->mrl, g_free);
		bacon_video_widget_close (theater->bvw);
		emit_file_closed (theater);
		theater->has_played_emitted = FALSE;
		play_pause_set_label (theater, STATE_STOPPED);
		update_fill (theater, -1.0);
	}

	if (mrl == NULL) {
		play_pause_set_label (theater, STATE_STOPPED);

		/* Play/Pause */
		theater_object_set_sensitivity2 ("play", FALSE);

		/* Volume */
		theater_controls_set_sensitivity ("volume_button", FALSE);
		theater->volume_sensitive = FALSE;

		/* Control popup */
		theater_object_set_sensitivity2 ("next-chapter", FALSE);
		theater_object_set_sensitivity2 ("previous-chapter", FALSE);

		/* Subtitle selection */
		theater_object_set_sensitivity2 ("select-subtitle", FALSE);

		/* Set the logo */
		bacon_video_widget_set_logo_mode (theater->bvw, TRUE);
		update_mrl_label (theater, NULL);

		g_object_notify (G_OBJECT (theater), "playing");
	} else {
		gboolean caps;
		char *user_agent;
		char *autoload_sub;

		bacon_video_widget_set_logo_mode (theater->bvw, FALSE);

		autoload_sub = NULL;
		if (subtitle == NULL)
			g_signal_emit (G_OBJECT (theater), theater_table_signals[GET_TEXT_SUBTITLE], 0, mrl, &autoload_sub);

		user_agent = NULL;
		g_signal_emit (G_OBJECT (theater), theater_table_signals[GET_USER_AGENT], 0, mrl, &user_agent);
		bacon_video_widget_set_user_agent (theater->bvw, user_agent);
		g_free (user_agent);

		g_application_mark_busy (G_APPLICATION (theater));
		bacon_video_widget_open (theater->bvw, mrl);
		if (subtitle) {
			bacon_video_widget_set_text_subtitle (theater->bvw, subtitle);
		} else if (autoload_sub) {
			bacon_video_widget_set_text_subtitle (theater->bvw, autoload_sub);
			g_free (autoload_sub);
		} else {
			theater_playlist_set_current_subtitle (theater->playlist, theater->next_subtitle);
			theater_object_set_next_subtitle (theater, NULL);
		}
		g_application_unmark_busy (G_APPLICATION (theater));
		theater->mrl = g_strdup (mrl);

		/* Play/Pause */
		theater_object_set_sensitivity2 ("play", TRUE);

		/* Volume */
		caps = bacon_video_widget_can_set_volume (theater->bvw);
		theater_controls_set_sensitivity ("volume_button", caps);
		theater->volume_sensitive = caps;

		/* Subtitle selection */
		theater_object_set_sensitivity2 ("select-subtitle", !theater_is_special_mrl (mrl));

		/* Set the playlist */
		play_pause_set_label (theater, STATE_PAUSED);

		emit_file_opened (theater, theater->mrl);

		theater_object_set_main_page (theater, "player");
	}

	g_object_notify (G_OBJECT (theater), "current-mrl");

	update_buttons (theater);
	update_media_menu_items (theater);
}

static gboolean
theater_time_within_seconds (theaterObject *theater)
{
	gint64 _time;

	_time = bacon_video_widget_get_current_time (theater->bvw);

	return (_time < REWIND_OR_PREVIOUS);
}

#define theater_has_direction_track(theater, dir) (dir == theater_PLAYLIST_DIRECTION_NEXT ? bacon_video_widget_has_next_track (theater->bvw) : bacon_video_widget_has_previous_track (theater->bvw))

static void
theater_object_direction (theaterObject *theater, theaterPlaylistDirection dir)
{
	if (theater_has_direction_track (theater, dir) == FALSE &&
	    theater_playlist_has_direction (theater->playlist, dir) == FALSE &&
	    theater_playlist_get_repeat (theater->playlist) == FALSE)
		return;

	if (theater_has_direction_track (theater, dir) != FALSE) {
		BvwDVDEvent event;
		event = (dir == theater_PLAYLIST_DIRECTION_NEXT ? BVW_DVD_NEXT_CHAPTER : BVW_DVD_PREV_CHAPTER);
		bacon_video_widget_dvd_event (theater->bvw, event);
		return;
	}

	if (dir == theater_PLAYLIST_DIRECTION_NEXT ||
	    bacon_video_widget_is_seekable (theater->bvw) == FALSE ||
	    theater_time_within_seconds (theater) != FALSE) {
		char *mrl, *subtitle;

		theater_playlist_set_direction (theater->playlist, dir);
		mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);
		theater_object_set_mrl_and_play (theater, mrl, subtitle);

		g_free (subtitle);
		g_free (mrl);
	} else {
		theater_object_seek (theater, 0);
	}
}

/**
 * theater_object_can_seek_previous:
 * @theater: a #theaterObject
 *
 * Returns true if theater_object_seek_previous() would have an effect.
 */
gboolean
theater_object_can_seek_previous (theaterObject *theater)
{
	return bacon_video_widget_has_previous_track (theater->bvw) ||
		theater_playlist_has_previous_mrl (theater->playlist) ||
		theater_playlist_get_repeat (theater->playlist);
}

/**
 * theater_object_seek_previous:
 * @theater: a #theaterObject
 *
 * If a DVD is being played, goes to the previous chapter. If a normal stream
 * is being played, goes to the start of the stream if possible. If seeking is
 * not possible, plays the previous entry in the playlist.
 **/
void
theater_object_seek_previous (theaterObject *theater)
{
	theater_object_direction (theater, theater_PLAYLIST_DIRECTION_PREVIOUS);
}

/**
 * theater_object_can_seek_next:
 * @theater: a #theaterObject
 *
 * Returns true if theater_object_seek_next() would have an effect.
 */
gboolean
theater_object_can_seek_next (theaterObject *theater)
{
	return bacon_video_widget_has_next_track (theater->bvw) ||
		theater_playlist_has_next_mrl (theater->playlist) ||
		theater_playlist_get_repeat (theater->playlist);
}

/**
 * theater_object_seek_next:
 * @theater: a #theaterObject
 *
 * If a DVD is being played, goes to the next chapter. If a normal stream
 * is being played, plays the next entry in the playlist.
 **/
void
theater_object_seek_next (theaterObject *theater)
{
	theater_object_direction (theater, theater_PLAYLIST_DIRECTION_NEXT);
}

static void
theater_seek_time_rel (theaterObject *theater, gint64 _time, gboolean relative, gboolean accurate)
{
	GError *err = NULL;
	gint64 sec;

	if (theater->mrl == NULL)
		return;
	if (bacon_video_widget_is_seekable (theater->bvw) == FALSE)
		return;

	if (relative != FALSE) {
		gint64 oldmsec;
		oldmsec = bacon_video_widget_get_current_time (theater->bvw);
		sec = MAX (0, oldmsec + _time);
	} else {
		sec = _time;
	}

	bacon_video_widget_seek_time (theater->bvw, sec, accurate, &err);

	if (err != NULL)
	{
		char *msg, *disp;

		disp = theater_uri_escape_for_display (theater->mrl);
		msg = g_strdup_printf(_("theater could not play “%s”."), disp);
		g_free (disp);

		bacon_video_widget_stop (theater->bvw);
		play_pause_set_label (theater, STATE_STOPPED);
		theater_object_show_error (theater, msg, err->message);
		g_free (msg);
		g_error_free (err);
	}
}

/**
 * theater_object_seek_relative:
 * @theater: a #theaterObject
 * @offset: the time offset to seek to
 * @accurate: whether to use accurate seek, an accurate seek might be slower for some formats (see GStreamer docs)
 *
 * Seeks to an @offset from the current position in the stream,
 * or displays an error dialog if that's not possible.
 **/
void
theater_object_seek_relative (theaterObject *theater, gint64 offset, gboolean accurate)
{
	theater_seek_time_rel (theater, offset, TRUE, accurate);
}

/**
 * theater_object_seek_time:
 * @theater: a #theaterObject
 * @msec: the time to seek to
 * @accurate: whether to use accurate seek, an accurate seek might be slower for some formats (see GStreamer docs)
 *
 * Seeks to an absolute time in the stream, or displays an
 * error dialog if that's not possible.
 **/
void
theater_object_seek_time (theaterObject *theater, gint64 msec, gboolean accurate)
{
	theater_seek_time_rel (theater, msec, FALSE, accurate);
}

void
theater_object_set_zoom (theaterObject *theater,
		       gboolean     zoom)
{
	GAction *action;

	action = g_action_map_lookup_action (G_ACTION_MAP (theater), "zoom");
	g_action_change_state (action, g_variant_new_boolean (zoom));
}

/**
 * theater_object_get_volume:
 * @theater: a #theaterObject
 *
 * Gets the current volume level, as a value between <code class="literal">0.0</code> and <code class="literal">1.0</code>.
 *
 * Return value: the volume level
 **/
double
theater_object_get_volume (theaterObject *theater)
{
	return bacon_video_widget_get_volume (theater->bvw);
}

/**
 * theater_object_set_volume:
 * @theater: a #theaterObject
 * @volume: the new absolute volume value
 *
 * Sets the volume, with <code class="literal">1.0</code> being the maximum, and <code class="literal">0.0</code> being the minimum level.
 **/
void
theater_object_set_volume (theaterObject *theater, double volume)
{
	if (bacon_video_widget_can_set_volume (theater->bvw) == FALSE)
		return;

	bacon_video_widget_set_volume (theater->bvw, volume);
}

/**
 * theater_object_get_rate:
 * @theater: a #theaterObject
 *
 * Gets the current playback rate, with `1.0` being the normal playback rate.
 *
 * Return value: the volume level
 **/
float
theater_object_get_rate (theaterObject *theater)
{
	return bacon_video_widget_get_rate (theater->bvw);
}

/**
 * theater_object_set_rate:
 * @theater: a #theaterObject
 * @rate: the new absolute playback rate
 *
 * Sets the playback rate, with `1.0` being the normal playback rate.
 *
 * Return value: %TRUE on success, %FALSE on failure.
 **/
gboolean
theater_object_set_rate (theaterObject *theater, float rate)
{
	return bacon_video_widget_set_rate (theater->bvw, rate);
}

/**
 * theater_object_set_volume_relative:
 * @theater: a #theaterObject
 * @off_pct: the value by which to increase or decrease the volume
 *
 * Sets the volume relative to its current level, with <code class="literal">1.0</code> being the
 * maximum, and <code class="literal">0.0</code> being the minimum level.
 **/
void
theater_object_set_volume_relative (theaterObject *theater, double off_pct)
{
	double vol;

	if (bacon_video_widget_can_set_volume (theater->bvw) == FALSE)
		return;
	if (theater->muted != FALSE)
		theater_object_volume_toggle_mute (theater);

	vol = bacon_video_widget_get_volume (theater->bvw);
	bacon_video_widget_set_volume (theater->bvw, vol + off_pct);
}

/**
 * theater_object_volume_toggle_mute:
 * @theater: a #theaterObject
 *
 * Toggles the mute status.
 **/
void
theater_object_volume_toggle_mute (theaterObject *theater)
{
	if (theater->muted == FALSE) {
		theater->muted = TRUE;
		theater->prev_volume = bacon_video_widget_get_volume (theater->bvw);
		bacon_video_widget_set_volume (theater->bvw, 0.0);
	} else {
		theater->muted = FALSE;
		bacon_video_widget_set_volume (theater->bvw, theater->prev_volume);
	}
}

static void
theater_object_toggle_aspect_ratio (theaterObject *theater)
{
	GAction *action;
	int tmp;

	tmp = bacon_video_widget_get_aspect_ratio (theater->bvw);
	tmp++;
	if (tmp > BVW_RATIO_DVB)
		tmp = BVW_RATIO_AUTO;

	action = g_action_map_lookup_action (G_ACTION_MAP (theater), "aspect-ratio");
	g_action_change_state (action, g_variant_new ("i", tmp));
}

void
theater_object_show_help (theaterObject *theater)
{
	GError *error = NULL;

	if (gtk_show_uri (gtk_widget_get_screen (theater->win), "help:theater", gtk_get_current_event_time (), &error) == FALSE) {
		theater_object_show_error (theater, _("theater could not display the help contents."), error->message);
		g_error_free (error);
	}
}

void
theater_object_show_keyboard_shortcuts (theaterObject *theater)
{
	GtkBuilder *builder;

	if (theater->shortcuts_win) {
		gtk_window_present (theater->shortcuts_win);
		return;
	}

	builder = theater_interface_load ("shortcuts.ui", FALSE, NULL, NULL);
	theater->shortcuts_win = GTK_WINDOW (gtk_builder_get_object (builder, "shortcuts-theater"));
	gtk_window_set_transient_for (theater->shortcuts_win, GTK_WINDOW (theater->win));

	g_signal_connect (theater->shortcuts_win, "destroy",
			  G_CALLBACK (gtk_widget_destroyed), &theater->shortcuts_win);

	gtk_widget_show (GTK_WIDGET (theater->shortcuts_win));
	g_object_unref (builder);
}

/* This is called in the main thread */
static void
theater_object_drop_files_finished (theaterPlaylist *playlist, GAsyncResult *result, theaterObject *theater)
{
	char *mrl, *subtitle;

	/* Reconnect the playlist's changed signal (which was disconnected below in theater_object_drop_files(). */
	g_signal_connect (G_OBJECT (playlist), "changed", G_CALLBACK (playlist_changed_cb), theater);
	mrl = theater_playlist_get_current_mrl (playlist, &subtitle);
	theater_object_set_mrl_and_play (theater, mrl, subtitle);
	g_free (mrl);
	g_free (subtitle);

	g_object_unref (theater);
}

static gboolean
theater_object_drop_files (theaterObject      *theater,
			 GtkSelectionData *data,
			 int               drop_type)
{
	char **list;
	guint i, len;
	GList *p, *file_list, *mrl_list = NULL;

	list = g_uri_list_extract_uris ((const char *) gtk_selection_data_get_data (data));
	file_list = NULL;

	for (i = 0; list[i] != NULL; i++) {
		char *filename;

		if (list[i] == NULL)
			continue;

		filename = theater_create_full_path (list[i]);
		file_list = g_list_prepend (file_list,
					    filename ? filename : g_strdup (list[i]));
	}
	g_strfreev (list);

	if (file_list == NULL)
		return FALSE;

	if (drop_type != 1)
		file_list = g_list_sort (file_list, (GCompareFunc) strcmp);
	else
		file_list = g_list_reverse (file_list);

	/* How many files? Check whether those could be subtitles */
	len = g_list_length (file_list);
	if (len == 1 || (len == 2 && drop_type == 1)) {
		if (theater_uri_is_subtitle (file_list->data) != FALSE) {
			theater_playlist_set_current_subtitle (theater->playlist, file_list->data);
			goto bail;
		}
	}

	/* The function that calls us knows better if we should be doing something with the changed playlist... */
	g_signal_handlers_disconnect_by_func (G_OBJECT (theater->playlist), playlist_changed_cb, theater);
	theater_playlist_clear (theater->playlist);

	/* Add each MRL to the playlist asynchronously */
	for (p = file_list; p != NULL; p = p->next) {
		const char *filename, *title;

		filename = p->data;
		title = NULL;

		/* Super _NETSCAPE_URL trick */
		if (drop_type == 1) {
			p = p->next;
			if (p != NULL) {
				if (g_str_has_prefix (p->data, "File:") != FALSE)
					title = (char *)p->data + 5;
				else
					title = p->data;
			}
		}

		/* Add the MRL data to the list of MRLs to add to the playlist */
		mrl_list = g_list_prepend (mrl_list, theater_playlist_mrl_data_new (filename, title));
	}

	/* Add the MRLs to the playlist asynchronously and in order. We need to reconnect playlist's "changed" signal once all of the add-MRL
	 * operations have completed. */
	if (mrl_list != NULL) {
		theater_playlist_add_mrls (theater->playlist, g_list_reverse (mrl_list), TRUE, NULL,
		                         (GAsyncReadyCallback) theater_object_drop_files_finished, g_object_ref (theater));
	}

bail:
	g_list_free_full (file_list, g_free);

	return TRUE;
}

static void
drop_video_cb (GtkWidget          *widget,
	       GdkDragContext     *context,
	       gint                x,
	       gint                y,
	       GtkSelectionData   *data,
	       guint               info,
	       guint               _time,
	       theater              *theater)
{
	GtkWidget *source_widget;
	GdkDragAction action = gdk_drag_context_get_selected_action (context);

	source_widget = gtk_drag_get_source_widget (context);

	/* Drop of video on itself */
	if (source_widget && widget == source_widget && action == GDK_ACTION_MOVE) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	theater_object_drop_files (theater, data, info);
	gtk_drag_finish (context, TRUE, FALSE, _time);
	return;
}

static void
back_button_clicked_cb (GtkButton   *button,
			theaterObject *theater)
{
	if (g_strcmp0 (theater_object_get_main_page (theater), "player") == 0) {
		theater_playlist_clear (theater->playlist);
		gtk_window_unfullscreen (GTK_WINDOW (theater->win));
		theater_object_set_main_page (theater, "grilo");
	} else {
		theater_grilo_back_button_clicked (theater_GRILO (theater->grilo));
	}
}

static void
on_got_redirect (BaconVideoWidget *bvw, const char *mrl, theaterObject *theater)
{
	char *new_mrl;

	if (strstr (mrl, "://") != NULL) {
		new_mrl = NULL;
	} else {
		GFile *old_file, *parent, *new_file;
		char *old_mrl;

		/* Get the parent for the current MRL, that's our base */
		old_mrl = theater_playlist_get_current_mrl (theater_PLAYLIST (theater->playlist), NULL);
		old_file = g_file_new_for_uri (old_mrl);
		g_free (old_mrl);
		parent = g_file_get_parent (old_file);
		g_object_unref (old_file);

		/* Resolve the URL */
		new_file = g_file_get_child (parent, mrl);
		g_object_unref (parent);

		new_mrl = g_file_get_uri (new_file);
		g_object_unref (new_file);
	}

	bacon_video_widget_close (theater->bvw);
	emit_file_closed (theater);
	theater->has_played_emitted = FALSE;
	g_application_mark_busy (G_APPLICATION (theater));
	bacon_video_widget_open (theater->bvw, new_mrl ? new_mrl : mrl);
	emit_file_opened (theater, new_mrl ? new_mrl : mrl);
	g_application_unmark_busy (G_APPLICATION (theater));
	if (bacon_video_widget_play (bvw, NULL) != FALSE) {
		theater_file_has_played (theater, theater->mrl);
		theater->has_played_emitted = TRUE;
	}
	g_free (new_mrl);
}

static void
on_channels_change_event (BaconVideoWidget *bvw, theaterObject *theater)
{
	gchar *name;

	theater_sublang_update (theater);
	update_media_menu_items (theater);

	/* updated stream info (new song) */
	name = theater_get_nice_name_for_stream (theater);

	if (name != NULL) {
		update_mrl_label (theater, name);
		theater_playlist_set_title
			(theater_PLAYLIST (theater->playlist), name);
		g_free (name);
	}
}

static void
on_playlist_change_name (theaterPlaylist *playlist, theaterObject *theater)
{
	char *name;

	name = theater_playlist_get_current_title (playlist);
	if (name != NULL) {
		update_mrl_label (theater, name);
		g_free (name);
	}
}

static void
on_got_metadata_event (BaconVideoWidget *bvw, theaterObject *theater)
{
        char *name;

	name = theater_get_nice_name_for_stream (theater);

	if (name != NULL) {
		theater_playlist_set_title
			(theater_PLAYLIST (theater->playlist), name);
		g_free (name);
	}

	theater_sublang_update (theater);
	update_buttons (theater);
	on_playlist_change_name (theater_PLAYLIST (theater->playlist), theater);
}

static void
on_error_event (BaconVideoWidget *bvw, char *message,
                gboolean playback_stopped, theaterObject *theater)
{
	/* Clear the seek if it's there, we only want to try and seek
	 * the first file, even if it's not there */
	theater_playlist_steal_current_starttime (theater->playlist);
	theater->pause_start = FALSE;

	if (playback_stopped)
		play_pause_set_label (theater, STATE_STOPPED);

	theater_object_show_error (theater, _("An error occurred"), message);
}

static void
on_buffering_event (BaconVideoWidget *bvw, gdouble percentage, theaterObject *theater)
{
	//FIXME show that somehow
}

static void
on_download_buffering_event (BaconVideoWidget *bvw, gdouble level, theaterObject *theater)
{
	update_fill (theater, level);
}

static void
update_fill (theaterObject *theater, gdouble level)
{
	if (level < 0.0) {
		gtk_range_set_show_fill_level (GTK_RANGE (theater->seek), FALSE);
	} else {
		gtk_range_set_fill_level (GTK_RANGE (theater->seek), level * 65535.0f);
		gtk_range_set_show_fill_level (GTK_RANGE (theater->seek), TRUE);
	}
}

static void
update_seekable (theaterObject *theater)
{
	gboolean seekable;
	gboolean notify;

	seekable = bacon_video_widget_is_seekable (theater->bvw);
	notify = (theater->seekable == seekable);
	theater->seekable = seekable;

	/* Check if the stream is seekable */
	gtk_widget_set_sensitive (theater->seek, seekable);

	if (seekable != FALSE) {
		gint64 starttime;

		starttime = theater_playlist_steal_current_starttime (theater->playlist);
		if (starttime != 0) {
			bacon_video_widget_seek_time (theater->bvw,
						      starttime * 1000, FALSE, NULL);
			if (theater->pause_start) {
				theater_object_pause (theater);
				theater->pause_start = FALSE;
			}
		}
	}

	if (notify)
		g_object_notify (G_OBJECT (theater), "seekable");
}

static void
update_slider_visibility (theaterObject *theater,
			  gint64 stream_length)
{
	if (theater->stream_length == stream_length)
		return;
	if (theater->stream_length > 0 && stream_length > 0)
		return;
	if (stream_length != 0)
		gtk_range_set_range (GTK_RANGE (theater->seek), 0., 65535.);
	else
		gtk_range_set_range (GTK_RANGE (theater->seek), 0., 0.);
}

static void
update_current_time (BaconVideoWidget *bvw,
		     gint64            current_time,
		     gint64            stream_length,
		     double            current_position,
		     gboolean          seekable,
		     theaterObject      *theater)
{
	update_slider_visibility (theater, stream_length);

	if (theater->seek_lock == FALSE) {
		gtk_adjustment_set_value (theater->seekadj,
					  current_position * 65535);

		if (stream_length == 0 && theater->mrl != NULL) {
			bacon_time_label_set_time (theater->time_label,
						   current_time, -1);
			bacon_time_label_set_time (theater->time_rem_label,
						   current_time, -1);
		} else {
			bacon_time_label_set_time (theater->time_label,
						   current_time,
						   stream_length);
			bacon_time_label_set_time (theater->time_rem_label,
						   current_time,
						   stream_length);
		}
	}

	if (theater->stream_length != stream_length) {
		g_object_notify (G_OBJECT (theater), "stream-length");
		theater->stream_length = stream_length;
	}
}

static void
volume_button_value_changed_cb (GtkScaleButton *button, gdouble value, theaterObject *theater)
{
	theater->muted = FALSE;
	bacon_video_widget_set_volume (theater->bvw, value);
}

static void
update_volume_sliders (theaterObject *theater)
{
	double volume;

	volume = bacon_video_widget_get_volume (theater->bvw);

	g_signal_handlers_block_by_func (theater->volume, volume_button_value_changed_cb, theater);
	gtk_scale_button_set_value (GTK_SCALE_BUTTON (theater->volume), volume);
	g_signal_handlers_unblock_by_func (theater->volume, volume_button_value_changed_cb, theater);
}

static void
property_notify_cb_volume (BaconVideoWidget *bvw, GParamSpec *spec, theaterObject *theater)
{
	update_volume_sliders (theater);
}

static void
property_notify_cb_seekable (BaconVideoWidget *bvw, GParamSpec *spec, theaterObject *theater)
{
	update_seekable (theater);
}

gboolean
seek_slider_pressed_cb (GtkWidget *widget, GdkEventButton *event, theaterObject *theater)
{
	/* HACK: we want the behaviour you get with the left button, so we
	 * mangle the event.  clicking with other buttons moves the slider in
	 * step increments, clicking with the left button moves the slider to
	 * the location of the click.
	 */
	event->button = GDK_BUTTON_PRIMARY;

	theater->seek_lock = TRUE;
	bacon_video_widget_mark_popup_busy (theater->bvw, "seek started");

	return FALSE;
}

void
seek_slider_changed_cb (GtkAdjustment *adj, theaterObject *theater)
{
	double pos;
	gint _time;

	if (theater->seek_lock == FALSE)
		return;

	pos = gtk_adjustment_get_value (adj) / 65535;
	_time = bacon_video_widget_get_stream_length (theater->bvw);

	bacon_time_label_set_time (theater->time_label,
				   pos * _time, _time);
	bacon_time_label_set_time (theater->time_rem_label,
				   pos * _time, _time);

	if (bacon_video_widget_can_direct_seek (theater->bvw) != FALSE)
		theater_object_seek (theater, pos);
}

gboolean
seek_slider_released_cb (GtkWidget *widget, GdkEventButton *event, theaterObject *theater)
{
	GtkAdjustment *adj;
	gdouble val;

	/* HACK: see seek_slider_pressed_cb */
	event->button = GDK_BUTTON_PRIMARY;

	/* set to FALSE here to avoid triggering a final seek when
	 * syncing the adjustments while being in direct seek mode */
	theater->seek_lock = FALSE;
	bacon_video_widget_unmark_popup_busy (theater->bvw, "seek started");

	/* sync both adjustments */
	adj = gtk_range_get_adjustment (GTK_RANGE (widget));
	val = gtk_adjustment_get_value (adj);

	if (bacon_video_widget_can_direct_seek (theater->bvw) == FALSE)
		theater_object_seek (theater, val / 65535.0);

	return FALSE;
}

gboolean
theater_object_open_files (theaterObject *theater, char **list)
{
	GSList *slist = NULL;
	int i, retval;

	for (i = 0 ; list[i] != NULL; i++)
		slist = g_slist_prepend (slist, list[i]);

	slist = g_slist_reverse (slist);
	retval = theater_object_open_files_list (theater, slist);
	g_slist_free (slist);

	return retval;
}

static gboolean
theater_object_open_files_list (theaterObject *theater, GSList *list)
{
	GSList *l;
	GList *mrl_list = NULL;
	gboolean changed;
	gboolean cleared;

	changed = FALSE;
	cleared = FALSE;

	if (list == NULL)
		return changed;

	g_application_mark_busy (G_APPLICATION (theater));

	for (l = list ; l != NULL; l = l->next)
	{
		char *filename;
		char *data = l->data;

		if (data == NULL)
			continue;

		/* Ignore relatives paths that start with "--", tough luck */
		if (data[0] == '-' && data[1] == '-')
			continue;

		/* Get the subtitle part out for our tests */
		filename = theater_create_full_path (data);
		if (filename == NULL)
			filename = g_strdup (data);

		if (g_file_test (filename, G_FILE_TEST_IS_REGULAR)
				|| strstr (filename, "#") != NULL
				|| strstr (filename, "://") != NULL
				|| g_str_has_prefix (filename, "dvd:") != FALSE
				|| g_str_has_prefix (filename, "vcd:") != FALSE
				|| g_str_has_prefix (filename, "dvb:") != FALSE)
		{
			if (cleared == FALSE)
			{
				/* The function that calls us knows better
				 * if we should be doing something with the 
				 * changed playlist ... */
				g_signal_handlers_disconnect_by_func
					(G_OBJECT (theater->playlist),
					 playlist_changed_cb, theater);
				changed = theater_playlist_clear (theater->playlist);
				bacon_video_widget_close (theater->bvw);
				emit_file_closed (theater);
				theater->has_played_emitted = FALSE;
				cleared = TRUE;
			}

			if (g_str_has_prefix (filename, "dvb:/") != FALSE) {
				mrl_list = g_list_prepend (mrl_list, theater_playlist_mrl_data_new (data, NULL));
				changed = TRUE;
			} else {
				mrl_list = g_list_prepend (mrl_list, theater_playlist_mrl_data_new (filename, NULL));
				changed = TRUE;
			}
		}

		g_free (filename);
	}

	/* Add the MRLs to the playlist asynchronously and in order */
	if (mrl_list != NULL)
		theater_playlist_add_mrls (theater->playlist, g_list_reverse (mrl_list), FALSE, NULL, NULL, NULL);

	g_application_unmark_busy (G_APPLICATION (theater));

	/* ... and reconnect because we're nice people */
	if (cleared != FALSE)
	{
		g_signal_connect (G_OBJECT (theater->playlist),
				"changed", G_CALLBACK (playlist_changed_cb),
				theater);
	}

	return changed;
}

void
show_controls (theaterObject *theater, gboolean was_fullscreen)
{
	GtkWidget *bvw_box;

	if (theater->bvw == NULL)
		return;

	bvw_box = GTK_WIDGET (gtk_builder_get_object (theater->xml, "tmw_bvw_box"));

	if (theater->controls_visibility == theater_CONTROLS_VISIBLE) {
		theater_object_save_size (theater);
	} else {
		 /* We won't show controls in fullscreen */
		gtk_container_set_border_width (GTK_CONTAINER (bvw_box), 0);
	}
}

/**
 * theater_object_next_angle:
 * @theater: a #theaterObject
 *
 * Switches to the next angle, if watching a DVD. If not watching a DVD, this is a
 * no-op.
 **/
void
theater_object_next_angle (theaterObject *theater)
{
	bacon_video_widget_set_next_angle (theater->bvw);
}

/**
 * theater_object_remote_command:
 * @theater: a #theaterObject
 * @cmd: a #theaterRemoteCommand
 * @url: an MRL to play, or %NULL
 *
 * Executes the specified @cmd on this instance of theater. If @cmd
 * is an operation requiring an MRL, @url is required; it can be %NULL
 * otherwise.
 *
 * If theater's fullscreened and the operation is executed correctly,
 * the controls will appear as if the user had moved the mouse.
 **/
void
theater_object_remote_command (theaterObject *theater, theaterRemoteCommand cmd, const char *url)
{
	switch (cmd) {
	case theater_REMOTE_COMMAND_PLAY:
		theater_object_play (theater);
		break;
	case theater_REMOTE_COMMAND_PLAYPAUSE:
		theater_object_play_pause (theater);
		break;
	case theater_REMOTE_COMMAND_PAUSE:
		theater_object_pause (theater);
		break;
	case theater_REMOTE_COMMAND_STOP:
		theater_object_stop (theater);
		break;
	case theater_REMOTE_COMMAND_SEEK_FORWARD: {
		double offset = 0;

		if (url != NULL)
			offset = g_ascii_strtod (url, NULL);
		if (offset == 0) {
			theater_object_seek_relative (theater, SEEK_FORWARD_OFFSET * 1000, FALSE);
		} else {
			theater_object_seek_relative (theater, offset * 1000, FALSE);
		}
		break;
	}
	case theater_REMOTE_COMMAND_SEEK_BACKWARD: {
		double offset = 0;

		if (url != NULL)
			offset = g_ascii_strtod (url, NULL);
		if (offset == 0)
			theater_object_seek_relative (theater, SEEK_BACKWARD_OFFSET * 1000, FALSE);
		else
			theater_object_seek_relative (theater,  - (offset * 1000), FALSE);
		break;
	}
	case theater_REMOTE_COMMAND_VOLUME_UP:
		theater_object_set_volume_relative (theater, VOLUME_UP_OFFSET);
		break;
	case theater_REMOTE_COMMAND_VOLUME_DOWN:
		theater_object_set_volume_relative (theater, VOLUME_DOWN_OFFSET);
		break;
	case theater_REMOTE_COMMAND_NEXT:
		theater_object_seek_next (theater);
		break;
	case theater_REMOTE_COMMAND_PREVIOUS:
		theater_object_seek_previous (theater);
		break;
	case theater_REMOTE_COMMAND_FULLSCREEN:
		if (g_strcmp0 (theater_object_get_main_page (theater), "player") == 0)
			theater_object_action_fullscreen_toggle (theater);
		break;
	case theater_REMOTE_COMMAND_QUIT:
		theater_object_exit (theater);
		break;
	case theater_REMOTE_COMMAND_ENQUEUE:
		g_assert (url != NULL);
		if (!theater_uri_is_subtitle (url))
			theater_playlist_add_mrl (theater->playlist, url, NULL, TRUE, NULL, NULL, NULL);
		else
			theater_object_set_next_subtitle (theater, url);
		break;
	case theater_REMOTE_COMMAND_REPLACE:
		if (url == NULL ||
		    !theater_uri_is_subtitle (url)) {
			theater_playlist_clear (theater->playlist);
			if (url == NULL) {
				bacon_video_widget_close (theater->bvw);
				emit_file_closed (theater);
				theater->has_played_emitted = FALSE;
				theater_object_set_mrl (theater, NULL, NULL);
				break;
			}
			theater_playlist_add_mrl (theater->playlist, url, NULL, TRUE, NULL, NULL, NULL);
		} else if (theater->mrl != NULL) {
			theater_playlist_set_current_subtitle (theater->playlist, url);
		} else {
			theater_object_set_next_subtitle (theater, url);
		}
		break;
	case theater_REMOTE_COMMAND_SHOW:
		gtk_window_present_with_time (GTK_WINDOW (theater->win), GDK_CURRENT_TIME);
		break;
	case theater_REMOTE_COMMAND_UP:
		bacon_video_widget_dvd_event (theater->bvw,
				BVW_DVD_ROOT_MENU_UP);
		break;
	case theater_REMOTE_COMMAND_DOWN:
		bacon_video_widget_dvd_event (theater->bvw,
				BVW_DVD_ROOT_MENU_DOWN);
		break;
	case theater_REMOTE_COMMAND_LEFT:
		bacon_video_widget_dvd_event (theater->bvw,
				BVW_DVD_ROOT_MENU_LEFT);
		break;
	case theater_REMOTE_COMMAND_RIGHT:
		bacon_video_widget_dvd_event (theater->bvw,
				BVW_DVD_ROOT_MENU_RIGHT);
		break;
	case theater_REMOTE_COMMAND_SELECT:
		bacon_video_widget_dvd_event (theater->bvw,
				BVW_DVD_ROOT_MENU_SELECT);
		break;
	case theater_REMOTE_COMMAND_DVD_MENU:
		bacon_video_widget_dvd_event (theater->bvw,
				BVW_DVD_ROOT_MENU);
		break;
	case theater_REMOTE_COMMAND_ZOOM_UP:
		theater_object_set_zoom (theater, TRUE);
		break;
	case theater_REMOTE_COMMAND_ZOOM_DOWN:
		theater_object_set_zoom (theater, FALSE);
		break;
	case theater_REMOTE_COMMAND_EJECT:
		theater_object_eject (theater);
		break;
	case theater_REMOTE_COMMAND_PLAY_DVD:
		if (g_strcmp0 (theater_object_get_main_page (theater), "player") == 0)
			back_button_clicked_cb (NULL, theater);
		theater_grilo_set_current_page (theater_GRILO (theater->grilo), theater_GRILO_PAGE_RECENT);
		break;
	case theater_REMOTE_COMMAND_MUTE:
		theater_object_volume_toggle_mute (theater);
		break;
	case theater_REMOTE_COMMAND_TOGGLE_ASPECT:
		theater_object_toggle_aspect_ratio (theater);
		break;
	case theater_REMOTE_COMMAND_UNKNOWN:
	default:
		break;
	}
}

/**
 * theater_object_remote_set_setting:
 * @theater: a #theaterObject
 * @setting: a #theaterRemoteSetting
 * @value: the new value for the setting
 *
 * Sets @setting to @value on this instance of theater.
 **/
void theater_object_remote_set_setting (theaterObject *theater,
				      theaterRemoteSetting setting,
				      gboolean value)
{
	GAction *action;

	switch (setting) {
	case theater_REMOTE_SETTING_REPEAT:
		action = g_action_map_lookup_action (G_ACTION_MAP (theater), "repeat");
		break;
	default:
		g_assert_not_reached ();
	}

	g_simple_action_set_state (G_SIMPLE_ACTION (action),
				   g_variant_new_boolean (value));
}

/**
 * theater_object_remote_get_setting:
 * @theater: a #theaterObject
 * @setting: a #theaterRemoteSetting
 *
 * Returns the value of @setting for this instance of theater.
 *
 * Return value: %TRUE if the setting is enabled, %FALSE otherwise
 **/
gboolean
theater_object_remote_get_setting (theaterObject        *theater,
				 theaterRemoteSetting  setting)
{
	GAction *action;
	GVariant *v;
	gboolean ret;

	action = NULL;

	switch (setting) {
	case theater_REMOTE_SETTING_REPEAT:
		action = g_action_map_lookup_action (G_ACTION_MAP (theater), "repeat");
		break;
	default:
		g_assert_not_reached ();
	}

	v = g_action_get_state (action);
	ret = g_variant_get_boolean (v);
	g_variant_unref (v);

	return ret;
}

static void
playlist_changed_cb (GtkWidget *playlist, theaterObject *theater)
{
	char *mrl, *subtitle;

	update_buttons (theater);
	mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);

	if (mrl == NULL)
		return;

	if (theater_playlist_get_playing (theater->playlist) == theater_PLAYLIST_STATUS_NONE) {
		if (theater->pause_start)
			theater_object_set_mrl (theater, mrl, subtitle);
		else
			theater_object_set_mrl_and_play (theater, mrl, subtitle);
	}

	theater->pause_start = FALSE;

	g_free (mrl);
	g_free (subtitle);
}

static void
item_activated_cb (GtkWidget *playlist, theaterObject *theater)
{
	theater_object_seek (theater, 0);
}

static void
current_removed_cb (GtkWidget *playlist, theaterObject *theater)
{
	char *mrl, *subtitle;

	/* Set play button status */
	play_pause_set_label (theater, STATE_STOPPED);
	mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);

	if (mrl == NULL) {
		g_free (subtitle);
		subtitle = NULL;
		theater_playlist_set_at_start (theater->playlist);
		update_buttons (theater);
		mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);
	} else {
		update_buttons (theater);
	}

	theater_object_set_mrl_and_play (theater, mrl, subtitle);
	g_free (mrl);
	g_free (subtitle);
}

static void
subtitle_changed_cb (GtkWidget *playlist, theaterObject *theater)
{
	char *mrl, *subtitle;

	mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);
	bacon_video_widget_set_text_subtitle (theater->bvw, subtitle);

	g_free (mrl);
	g_free (subtitle);
}

static void
playlist_repeat_toggle_cb (theaterPlaylist *playlist, GParamSpec *pspec, theaterObject *theater)
{
	GAction *action;
	gboolean repeat;

	repeat = theater_playlist_get_repeat (playlist);
	action = g_action_map_lookup_action (G_ACTION_MAP (theater), "repeat");
	g_simple_action_set_state (G_SIMPLE_ACTION (action),
				   g_variant_new_boolean (repeat));
}

/**
 * theater_object_is_fullscreen:
 * @theater: a #theaterObject
 *
 * Returns %TRUE if theater is fullscreened.
 *
 * Return value: %TRUE if theater is fullscreened
 **/
gboolean
theater_object_is_fullscreen (theaterObject *theater)
{
	g_return_val_if_fail (theater_IS_OBJECT (theater), FALSE);

	return (theater->controls_visibility == theater_CONTROLS_FULLSCREEN);
}

/**
 * theater_object_is_playing:
 * @theater: a #theaterObject
 *
 * Returns %TRUE if theater is playing a stream.
 *
 * Return value: %TRUE if theater is playing a stream
 **/
gboolean
theater_object_is_playing (theaterObject *theater)
{
	g_return_val_if_fail (theater_IS_OBJECT (theater), FALSE);

	if (theater->bvw == NULL)
		return FALSE;

	return bacon_video_widget_is_playing (theater->bvw) != FALSE;
}

/**
 * theater_object_is_paused:
 * @theater: a #theaterObject
 *
 * Returns %TRUE if playback is paused.
 *
 * Return value: %TRUE if playback is paused, %FALSE otherwise
 **/
gboolean
theater_object_is_paused (theaterObject *theater)
{
	g_return_val_if_fail (theater_IS_OBJECT (theater), FALSE);

	return theater->state == STATE_PAUSED;
}

/**
 * theater_object_is_seekable:
 * @theater: a #theaterObject
 *
 * Returns %TRUE if the current stream is seekable.
 *
 * Return value: %TRUE if the current stream is seekable
 **/
gboolean
theater_object_is_seekable (theaterObject *theater)
{
	g_return_val_if_fail (theater_IS_OBJECT (theater), FALSE);

	if (theater->bvw == NULL)
		return FALSE;

	return bacon_video_widget_is_seekable (theater->bvw) != FALSE;
}

static gboolean
event_is_touch (GdkEventButton *event)
{
	GdkDevice *device;

	device = gdk_event_get_device ((GdkEvent *) event);
	return (gdk_device_get_source (device) == GDK_SOURCE_TOUCHSCREEN);
}

static gboolean
on_video_button_press_event (BaconVideoWidget *bvw, GdkEventButton *event,
		theaterObject *theater)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
		gtk_widget_grab_focus (GTK_WIDGET (bvw));
		return TRUE;
	} else if (event->type == GDK_2BUTTON_PRESS &&
		   event->button == 1 &&
		   event_is_touch (event) == FALSE) {
		theater_object_action_fullscreen_toggle (theater);
		return TRUE;
	} else if (event->type == GDK_BUTTON_PRESS && event->button == 2) {
		theater_object_play_pause (theater);
		return TRUE;
	}

	return FALSE;
}

static gboolean
on_eos_event (GtkWidget *widget, theaterObject *theater)
{
	reset_seek_status (theater);

	if (bacon_video_widget_get_logo_mode (theater->bvw) != FALSE)
		return FALSE;

	if (theater_playlist_has_next_mrl (theater->playlist) == FALSE &&
	    theater_playlist_get_repeat (theater->playlist) == FALSE &&
	    (theater_playlist_get_last (theater->playlist) != 0 ||
	     theater_object_is_seekable (theater) == FALSE)) {
		char *mrl, *subtitle;

		/* Set play button status */
		theater_playlist_set_at_start (theater->playlist);
		update_buttons (theater);
		bacon_video_widget_stop (theater->bvw);
		play_pause_set_label (theater, STATE_STOPPED);
		mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);
		theater_object_set_mrl (theater, mrl, subtitle);
		bacon_video_widget_pause (theater->bvw);
		g_free (mrl);
		g_free (subtitle);
	} else {
		if (theater_playlist_get_last (theater->playlist) == 0 &&
		    theater_object_is_seekable (theater)) {
			if (theater_playlist_get_repeat (theater->playlist) != FALSE) {
				theater_object_seek_time (theater, 0, FALSE);
				theater_object_play (theater);
			} else {
				theater_object_pause (theater);
				theater_object_seek_time (theater, 0, FALSE);
			}
		} else {
			theater_object_seek_next (theater);
		}
	}

	return FALSE;
}

static void
theater_object_handle_seek (theaterObject *theater, GdkEventKey *event, gboolean is_forward)
{
	if (is_forward != FALSE) {
		if (event->state & GDK_SHIFT_MASK)
			theater_object_seek_relative (theater, SEEK_FORWARD_SHORT_OFFSET * 1000, FALSE);
		else if (event->state & GDK_CONTROL_MASK)
			theater_object_seek_relative (theater, SEEK_FORWARD_LONG_OFFSET * 1000, FALSE);
		else
			theater_object_seek_relative (theater, SEEK_FORWARD_OFFSET * 1000, FALSE);
	} else {
		if (event->state & GDK_SHIFT_MASK)
			theater_object_seek_relative (theater, SEEK_BACKWARD_SHORT_OFFSET * 1000, FALSE);
		else if (event->state & GDK_CONTROL_MASK)
			theater_object_seek_relative (theater, SEEK_BACKWARD_LONG_OFFSET * 1000, FALSE);
		else
			theater_object_seek_relative (theater, SEEK_BACKWARD_OFFSET * 1000, FALSE);
	}
}

static gboolean
theater_object_handle_key_press (theaterObject *theater, GdkEventKey *event)
{
	GdkModifierType mask;
	gboolean retval;
	gboolean switch_rtl = FALSE;

	retval = TRUE;

	mask = event->state & gtk_accelerator_get_default_mod_mask ();

	switch (event->keyval) {
	case GDK_KEY_A:
	case GDK_KEY_a:
		theater_object_toggle_aspect_ratio (theater);
		break;
	case GDK_KEY_AudioCycleTrack:
		bacon_video_widget_set_next_language (theater->bvw);
		break;
	case GDK_KEY_AudioPrev:
	case GDK_KEY_Back:
	case GDK_KEY_B:
	case GDK_KEY_b:
		theater_object_seek_previous (theater);
		bacon_video_widget_show_popup (theater->bvw);
		break;
	case GDK_KEY_C:
	case GDK_KEY_c:
		bacon_video_widget_dvd_event (theater->bvw,
				BVW_DVD_CHAPTER_MENU);
		break;
	case GDK_KEY_F5:
		/* Start presentation button */
		theater_object_set_fullscreen (theater, TRUE);
		theater_object_play_pause (theater);
		break;
	case GDK_KEY_F11:
	case GDK_KEY_f:
	case GDK_KEY_F:
		theater_object_action_fullscreen_toggle (theater);
		break;
	case GDK_KEY_CycleAngle:
	case GDK_KEY_g:
	case GDK_KEY_G:
		theater_object_next_angle (theater);
		break;
	case GDK_KEY_H:
	case GDK_KEY_h:
		theater_object_show_keyboard_shortcuts (theater);
		break;
	case GDK_KEY_question:
		theater_object_show_keyboard_shortcuts (theater);
		break;
	case GDK_KEY_M:
	case GDK_KEY_m:
		bacon_video_widget_dvd_event (theater->bvw, BVW_DVD_ROOT_MENU);
		break;
	case GDK_KEY_AudioNext:
	case GDK_KEY_Forward:
	case GDK_KEY_N:
	case GDK_KEY_n:
	case GDK_KEY_End:
		theater_object_seek_next (theater);
		bacon_video_widget_show_popup (theater->bvw);
		break;
	case GDK_KEY_OpenURL:
		theater_object_set_fullscreen (theater, FALSE);
		theater_object_open_location (theater);
		break;
	case GDK_KEY_O:
	case GDK_KEY_o:
	case GDK_KEY_Open:
		theater_object_set_fullscreen (theater, FALSE);
		theater_object_open (theater);
		break;
	case GDK_KEY_AudioPlay:
	case GDK_KEY_p:
	case GDK_KEY_P:
		theater_object_play_pause (theater);
		break;
	case GDK_KEY_comma:
	case GDK_KEY_FrameBack:
		theater_object_pause (theater);
		bacon_video_widget_step (theater->bvw, FALSE, NULL);
		break;
	case GDK_KEY_period:
	case GDK_KEY_FrameForward:
		theater_object_pause (theater);
		bacon_video_widget_step (theater->bvw, TRUE, NULL);
		break;
	case GDK_KEY_AudioPause:
	case GDK_KEY_Pause:
	case GDK_KEY_AudioStop:
		theater_object_pause (theater);
		break;
	case GDK_KEY_q:
	case GDK_KEY_Q:
		theater_object_exit (theater);
		break;
	case GDK_KEY_r:
	case GDK_KEY_R:
	case GDK_KEY_ZoomIn:
		theater_object_set_zoom (theater, TRUE);
		break;
	case GDK_KEY_Subtitle:
		bacon_video_widget_set_next_subtitle (theater->bvw);
		break;
	case GDK_KEY_t:
	case GDK_KEY_T:
	case GDK_KEY_ZoomOut:
		theater_object_set_zoom (theater, FALSE);
		break;
	case GDK_KEY_Eject:
		theater_object_eject (theater);
		break;
	case GDK_KEY_Escape:
		if (mask == GDK_SUPER_MASK)
			bacon_video_widget_dvd_event (theater->bvw, BVW_DVD_ROOT_MENU);
		else
			theater_object_set_fullscreen (theater, FALSE);
		break;
	case GDK_KEY_space:
	case GDK_KEY_Return:
		if (mask != GDK_CONTROL_MASK) {
			GtkWidget *focus = gtk_window_get_focus (GTK_WINDOW (theater->win));
			if (theater_object_is_fullscreen (theater) != FALSE || focus == NULL ||
			    focus == GTK_WIDGET (theater->bvw) || focus == theater->seek) {
				if (event->keyval == GDK_KEY_space) {
					theater_object_play_pause (theater);
				} else if (bacon_video_widget_has_menus (theater->bvw) != FALSE) {
					bacon_video_widget_dvd_event (theater->bvw, BVW_DVD_ROOT_MENU_SELECT);
				}
			} else
				retval = FALSE;
		}
		break;
	case GDK_KEY_Left:
	case GDK_KEY_Right:
		switch_rtl = TRUE;
		/* fall through */
	case GDK_KEY_Page_Up:
	case GDK_KEY_Page_Down:
		if (bacon_video_widget_has_menus (theater->bvw) == FALSE) {
			gboolean is_forward;

			is_forward = (event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_Page_Up);
			/* Switch direction in RTL environment */
			if (switch_rtl && gtk_widget_get_direction (theater->win) == GTK_TEXT_DIR_RTL)
				is_forward = !is_forward;

			if (theater_object_is_seekable (theater)) {
				theater_object_handle_seek (theater, event, is_forward);
				bacon_video_widget_show_popup (theater->bvw);
			}
		} else {
			if (event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_Page_Down)
				bacon_video_widget_dvd_event (theater->bvw, BVW_DVD_ROOT_MENU_LEFT);
			else
				bacon_video_widget_dvd_event (theater->bvw, BVW_DVD_ROOT_MENU_RIGHT);
		}
		break;
	case GDK_KEY_Home:
		theater_object_seek (theater, 0);
		bacon_video_widget_show_popup (theater->bvw);
		break;
	case GDK_KEY_Up:
		if (bacon_video_widget_has_menus (theater->bvw) != FALSE)
			bacon_video_widget_dvd_event (theater->bvw, BVW_DVD_ROOT_MENU_UP);
		else if (mask == GDK_SHIFT_MASK)
			theater_object_set_volume_relative (theater, VOLUME_UP_SHORT_OFFSET);
		else
			theater_object_set_volume_relative (theater, VOLUME_UP_OFFSET);
		break;
	case GDK_KEY_Down:
		if (bacon_video_widget_has_menus (theater->bvw) != FALSE)
			bacon_video_widget_dvd_event (theater->bvw, BVW_DVD_ROOT_MENU_DOWN);
		else if (mask == GDK_SHIFT_MASK)
			theater_object_set_volume_relative (theater, VOLUME_DOWN_SHORT_OFFSET);
		else
			theater_object_set_volume_relative (theater, VOLUME_DOWN_OFFSET);
		break;
	case GDK_KEY_Select:
		if (bacon_video_widget_has_menus (theater->bvw) != FALSE)
			bacon_video_widget_dvd_event (theater->bvw, BVW_DVD_ROOT_MENU_SELECT);
		break;
	case GDK_KEY_0:
		if (mask == GDK_CONTROL_MASK)
			theater_object_set_zoom (theater, FALSE);
		break;
	case GDK_KEY_Menu:
	case GDK_KEY_F10:
		bacon_video_widget_show_popup (theater->bvw);
		if (theater->controls_visibility != theater_CONTROLS_FULLSCREEN) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (theater->gear_button),
						      !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (theater->gear_button)));
		} else {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (theater->fullscreen_gear_button),
						      !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (theater->fullscreen_gear_button)));
		}
		break;
	case GDK_KEY_Time:
		bacon_video_widget_show_popup (theater->bvw);
		break;
	case GDK_KEY_equal:
		if (mask == GDK_CONTROL_MASK)
			theater_object_set_zoom (theater, TRUE);
		break;
	case GDK_KEY_hyphen:
		if (mask == GDK_CONTROL_MASK)
			theater_object_set_zoom (theater, FALSE);
		break;
	case GDK_KEY_plus:
	case GDK_KEY_KP_Add:
		if (mask != GDK_CONTROL_MASK) {
			theater_object_seek_next (theater);
			bacon_video_widget_show_popup (theater->bvw);
		} else {
			theater_object_set_zoom (theater, TRUE);
		}
		break;
	case GDK_KEY_minus:
	case GDK_KEY_KP_Subtract:
		if (mask != GDK_CONTROL_MASK) {
			theater_object_seek_previous (theater);
			bacon_video_widget_show_popup (theater->bvw);
		} else {
			theater_object_set_zoom (theater, FALSE);
		}
		break;
	case GDK_KEY_KP_Up:
	case GDK_KEY_KP_8:
		bacon_video_widget_dvd_event (theater->bvw, 
					      BVW_DVD_ROOT_MENU_UP);
		break;
	case GDK_KEY_KP_Down:
	case GDK_KEY_KP_2:
		bacon_video_widget_dvd_event (theater->bvw, 
					      BVW_DVD_ROOT_MENU_DOWN);
		break;
	case GDK_KEY_KP_Right:
	case GDK_KEY_KP_6:
		bacon_video_widget_dvd_event (theater->bvw, 
					      BVW_DVD_ROOT_MENU_RIGHT);
		break;
	case GDK_KEY_KP_Left:
	case GDK_KEY_KP_4:
		bacon_video_widget_dvd_event (theater->bvw, 
					      BVW_DVD_ROOT_MENU_LEFT);
		break;
	case GDK_KEY_KP_Begin:
	case GDK_KEY_KP_5:
		bacon_video_widget_dvd_event (theater->bvw,
					      BVW_DVD_ROOT_MENU_SELECT);
	default:
		retval = FALSE;
	}

	return retval;
}

static void
on_seek_requested_event (BaconVideoWidget *bvw,
			 gboolean          forward,
			 theaterObject      *theater)
{
	gint64 offset;

	offset = forward ? SEEK_FORWARD_OFFSET * 1000 : SEEK_BACKWARD_OFFSET * 1000;
	theater_object_seek_relative (theater, offset, FALSE);
}

static void
on_track_skip_requested_event (BaconVideoWidget *bvw,
			       gboolean          is_forward,
			       theaterObject      *theater)
{
	theater_object_direction (theater, is_forward ? theater_PLAYLIST_DIRECTION_NEXT : theater_PLAYLIST_DIRECTION_PREVIOUS);
}

static void
on_volume_change_requested_event (BaconVideoWidget *bvw,
				  gboolean          increase,
				  theaterObject      *theater)
{
	theater_object_set_volume_relative (theater, increase ? VOLUME_UP_OFFSET : VOLUME_DOWN_OFFSET);
}

gboolean
window_key_press_event_cb (GtkWidget *win, GdkEventKey *event, theaterObject *theater)
{
	/* Shortcuts disabled? */
	if (theater->disable_kbd_shortcuts != FALSE)
		return FALSE;

	/* Handle Quit */
	if (event->state & GDK_CONTROL_MASK &&
	    event->type == GDK_KEY_PRESS &&
	    (event->keyval == GDK_KEY_Q ||
	     event->keyval == GDK_KEY_q)) {
		return theater_object_handle_key_press (theater, event);
	}

	/* Handle back/quit */
	if (event->state & GDK_CONTROL_MASK &&
	    event->type == GDK_KEY_PRESS &&
	    (event->keyval == GDK_KEY_W ||
	     event->keyval == GDK_KEY_w)) {
		if (theater_grilo_get_show_back_button (theater_GRILO (theater->grilo)) ||
		    g_str_equal (theater_object_get_main_page (theater), "player"))
			back_button_clicked_cb (NULL, theater);
		else
			theater_object_exit (theater);
		return FALSE;
	}

	/* Check whether we're in the player panel */
	if (!g_str_equal (theater_object_get_main_page (theater), "player"))
		return FALSE;

	/* Special case Eject, Open, Open URI,
	 * seeking and zoom keyboard shortcuts */
	if (event->state != 0
			&& (event->state & GDK_CONTROL_MASK))
	{
		switch (event->keyval) {
		case GDK_KEY_E:
		case GDK_KEY_e:
		case GDK_KEY_O:
		case GDK_KEY_o:
		case GDK_KEY_L:
		case GDK_KEY_l:
		case GDK_KEY_q:
		case GDK_KEY_Q:
		case GDK_KEY_Right:
		case GDK_KEY_Left:
		case GDK_KEY_plus:
		case GDK_KEY_KP_Add:
		case GDK_KEY_minus:
		case GDK_KEY_KP_Subtract:
		case GDK_KEY_0:
		case GDK_KEY_equal:
		case GDK_KEY_hyphen:
			if (event->type == GDK_KEY_PRESS)
				return theater_object_handle_key_press (theater, event);
		default:
			break;
		}
	}

	if (event->state != 0 && (event->state & GDK_SUPER_MASK)) {
		switch (event->keyval) {
		case GDK_KEY_Escape:
			if (event->type == GDK_KEY_PRESS)
				return theater_object_handle_key_press (theater, event);
		default:
			break;
		}
	}


	/* If we have modifiers, and either Ctrl, Mod1 (Alt), or any
	 * of Mod3 to Mod5 (Mod2 is num-lock...) are pressed, we
	 * let Gtk+ handle the key */
	if (event->state != 0
			&& ((event->state & GDK_CONTROL_MASK)
			|| (event->state & GDK_MOD1_MASK)
			|| (event->state & GDK_MOD3_MASK)
			|| (event->state & GDK_MOD4_MASK)))
		return FALSE;

	if (event->type == GDK_KEY_PRESS)
		return theater_object_handle_key_press (theater, event);

	return FALSE;
}

static void
update_media_menu_items (theaterObject *theater)
{
	GMount *mount;
	gboolean playing;

	playing = theater_playing_dvd (theater->mrl);

	theater_object_set_sensitivity2 ("dvd-root-menu", playing);
	theater_object_set_sensitivity2 ("dvd-title-menu", playing);
	theater_object_set_sensitivity2 ("dvd-audio-menu", playing);
	theater_object_set_sensitivity2 ("dvd-angle-menu", playing);
	theater_object_set_sensitivity2 ("dvd-chapter-menu", playing);

	theater_object_set_sensitivity2 ("next-angle",
				       bacon_video_widget_has_angles (theater->bvw));

	mount = theater_get_mount_for_media (theater->mrl);
	theater_object_set_sensitivity2 ("eject", mount != NULL);
	if (mount != NULL)
		g_object_unref (mount);
}

static void
update_buttons (theaterObject *theater)
{
	theater_object_set_sensitivity2 ("previous-chapter",
				       theater_object_can_seek_previous (theater));
	theater_object_set_sensitivity2 ("next-chapter",
				       theater_object_can_seek_next (theater));
}

void
theater_setup_window (theaterObject *theater)
{
	GKeyFile *keyfile;
	int w, h;
	char *filename;
	GError *err = NULL;
	GtkWidget *vbox;
	GdkRGBA black;

	filename = g_build_filename (theater_dot_dir (), "state.ini", NULL);
	keyfile = g_key_file_new ();
	if (g_key_file_load_from_file (keyfile, filename,
			G_KEY_FILE_NONE, NULL) == FALSE) {
		w = DEFAULT_WINDOW_W;
		h = DEFAULT_WINDOW_H;
		theater->maximised = TRUE;
		g_free (filename);
	} else {
		g_free (filename);

		w = g_key_file_get_integer (keyfile, "State", "window_w", &err);
		if (err != NULL) {
			w = 0;
			g_error_free (err);
			err = NULL;
		}

		h = g_key_file_get_integer (keyfile, "State", "window_h", &err);
		if (err != NULL) {
			h = 0;
			g_error_free (err);
			err = NULL;
		}

		theater->maximised = g_key_file_get_boolean (keyfile, "State",
				"maximised", &err);
		if (err != NULL) {
			g_error_free (err);
			err = NULL;
		}
	}

	if (w > 0 && h > 0 && theater->maximised == FALSE) {
		gtk_window_set_default_size (GTK_WINDOW (theater->win),
				w, h);
		theater->window_w = w;
		theater->window_h = h;
	} else if (theater->maximised != FALSE) {
		gtk_window_maximize (GTK_WINDOW (theater->win));
	}

	/* Set the vbox to be completely black */
	vbox = GTK_WIDGET (gtk_builder_get_object (theater->xml, "tmw_bvw_box"));
	gdk_rgba_parse (&black, "Black");
	gtk_widget_override_background_color (vbox, (GTK_STATE_FLAG_FOCUSED << 1), &black);

	/* Headerbar */
	theater->header = g_object_new (theater_TYPE_MAIN_TOOLBAR,
				      "show-search-button", TRUE,
				      "show-select-button", TRUE,
				      "show-close-button", TRUE,
				      "title", _("Videos"),
				      NULL);
	g_signal_connect (G_OBJECT (theater->header), "back-clicked",
			  G_CALLBACK (back_button_clicked_cb), theater);
	gtk_window_set_titlebar (GTK_WINDOW (theater->win), theater->header);

	return;
}

static void
popup_menu_shown_cb (GtkToggleButton *button,
		     theaterObject     *theater)
{
	if (gtk_toggle_button_get_active (button))
		bacon_video_widget_mark_popup_busy (theater->bvw, "toolbar/go menu visible");
	else
		bacon_video_widget_unmark_popup_busy (theater->bvw, "toolbar/go menu visible");
}

static void
volume_button_menu_shown_cb (GObject     *popover,
			     GParamSpec  *pspec,
			     theaterObject *theater)
{
	if (gtk_widget_is_visible (GTK_WIDGET (popover)))
		bacon_video_widget_mark_popup_busy (theater->bvw, "volume menu visible");
	else
		bacon_video_widget_unmark_popup_busy (theater->bvw, "volume menu visible");
}

static void
update_add_button_visibility (GObject     *gobject,
			      GParamSpec  *pspec,
			      theaterObject *theater)
{
	theaterMainToolbar *bar = theater_MAIN_TOOLBAR (gobject);

	if (theater_main_toolbar_get_search_mode (bar) ||
	    theater_main_toolbar_get_select_mode (bar)) {
		gtk_widget_hide (theater->add_button);
	} else {
		gtk_widget_set_visible (theater->add_button,
					theater_grilo_get_current_page (theater_GRILO (theater->grilo))  == theater_GRILO_PAGE_RECENT);
	}
}

static GtkWidget *
create_control_button (theaterObject *theater,
		       const gchar *action_name,
		       const gchar *icon_name,
		       const gchar *tooltip_text)
{
	GtkWidget *button, *image;

	button = gtk_button_new ();
	gtk_actionable_set_action_name (GTK_ACTIONABLE (button), action_name);
	image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	gtk_button_set_image (GTK_BUTTON (button), image);
	gtk_widget_set_valign (GTK_WIDGET (button), GTK_ALIGN_CENTER);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "image-button");
	if (g_str_equal (action_name, "app.play")) {
		g_object_set (G_OBJECT (image),
			      "margin-start", 16,
			      "margin-end", 16,
			      NULL);
		theater->play_button = button;
	}

	gtk_button_set_label (GTK_BUTTON (button), NULL);
	gtk_widget_set_tooltip_text (button, tooltip_text);
	atk_object_set_name (gtk_widget_get_accessible (button), tooltip_text);

	gtk_widget_show_all (button);

	return button;
}

void
theater_callback_connect (theaterObject *theater)
{
	GtkWidget *item;
	GtkBox *box;
	GAction *gaction;
	GMenuModel *menu;
	GtkPopover *popover;

	/* Menu items */
	gaction = g_action_map_lookup_action (G_ACTION_MAP (theater), "repeat");
	g_simple_action_set_state (G_SIMPLE_ACTION (gaction),
				   g_variant_new_boolean (theater_playlist_get_repeat (theater->playlist)));

	/* Controls */
	box = g_object_get_data (theater->controls, "controls_box");
	gtk_widget_insert_action_group (GTK_WIDGET (box), "app", G_ACTION_GROUP (theater));

	/* Previous */
	item = create_control_button (theater, "app.previous-chapter",
				      "media-skip-backward-symbolic",
				      _("Previous Chapter/Movie"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Play/Pause */
	item = create_control_button (theater, "app.play",
				      "media-playback-start-symbolic",
				      _("Play / Pause"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Next */
	item = create_control_button (theater, "app.next-chapter",
				      "media-skip-forward-symbolic",
				      _("Next Chapter/Movie"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Seekbar */
	g_signal_connect (theater->seek, "button-press-event",
			  G_CALLBACK (seek_slider_pressed_cb), theater);
	g_signal_connect (theater->seek, "button-release-event",
			  G_CALLBACK (seek_slider_released_cb), theater);
	g_signal_connect (theater->seekadj, "value-changed",
			  G_CALLBACK (seek_slider_changed_cb), theater);

	/* Volume */
	g_signal_connect (theater->volume, "value-changed",
			  G_CALLBACK (volume_button_value_changed_cb), theater);
	item = gtk_scale_button_get_popup (GTK_SCALE_BUTTON (theater->volume));
	g_signal_connect (G_OBJECT (item), "notify::visible",
			  G_CALLBACK (volume_button_menu_shown_cb), theater);

	/* Go button */
	item = g_object_get_data (theater->controls, "go_button");
	menu = (GMenuModel *) gtk_builder_get_object (theater->xml, "gomenu");
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (item), menu);
	popover = gtk_menu_button_get_popover (GTK_MENU_BUTTON (item));
	gtk_popover_set_transitions_enabled (GTK_POPOVER (popover), FALSE);
	gtk_widget_set_size_request (GTK_WIDGET (popover), 175, -1);
	g_signal_connect (G_OBJECT (item), "toggled",
			  G_CALLBACK (popup_menu_shown_cb), theater);

	/* Main menu */
	item = theater->main_menu_button = theater_interface_create_header_button (theater->header,
									       gtk_menu_button_new (),
									       "open-menu-symbolic",
									       GTK_PACK_END);
	gtk_container_child_set (GTK_CONTAINER (theater->header), theater->main_menu_button,
				 "position", 0,
				 NULL);
	menu = (GMenuModel *) gtk_builder_get_object (theater->xml, "appmenu");
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (item), menu);
	gtk_widget_show (item);

	/* Player menu */
	item = theater->gear_button = theater_interface_create_header_button (theater->header,
									  gtk_menu_button_new (),
									  "view-more-symbolic",
									  GTK_PACK_END);
	menu = (GMenuModel *) gtk_builder_get_object (theater->xml, "playermenu");
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (item), menu);
	popover = gtk_menu_button_get_popover (GTK_MENU_BUTTON (item));
	gtk_popover_set_transitions_enabled (GTK_POPOVER (popover), FALSE);
	g_signal_connect (G_OBJECT (item), "toggled",
			  G_CALLBACK (popup_menu_shown_cb), theater);

	/* Add button */
	item = theater->add_button = theater_interface_create_header_button (theater->header,
									 gtk_menu_button_new (),
									 "list-add-symbolic",
									 GTK_PACK_START);
	menu = (GMenuModel *) gtk_builder_get_object (theater->xml, "addmenu");
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (item), menu);
	gtk_widget_show (item);

	g_signal_connect (G_OBJECT (theater->header), "notify::search-mode",
			  G_CALLBACK (update_add_button_visibility), theater);
	g_signal_connect (G_OBJECT (theater->header), "notify::select-mode",
			  G_CALLBACK (update_add_button_visibility), theater);

	/* Fullscreen button */
	item = theater->fullscreen_button = theater_interface_create_header_button (theater->header,
										gtk_button_new (),
										"view-fullscreen-symbolic",
										GTK_PACK_END);
	gtk_actionable_set_action_name (GTK_ACTIONABLE (item), "app.fullscreen");

	/* Connect the keys */
	gtk_widget_add_events (theater->win, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

	/* Set sensitivity of the toolbar buttons */
	theater_object_set_sensitivity2 ("play", FALSE);
	theater_object_set_sensitivity2 ("next-chapter", FALSE);
	theater_object_set_sensitivity2 ("previous-chapter", FALSE);

	/* Volume */
	g_signal_connect (G_OBJECT (theater->bvw), "notify::volume",
			G_CALLBACK (property_notify_cb_volume), theater);
	g_signal_connect (G_OBJECT (theater->bvw), "notify::seekable",
			G_CALLBACK (property_notify_cb_seekable), theater);
	update_volume_sliders (theater);
}

void
playlist_widget_setup (theaterObject *theater)
{
	theater->playlist = theater_PLAYLIST (theater_playlist_new ());

	if (theater->playlist == NULL)
		theater_object_exit (theater);

#if 0
	{
		GtkWidget *window;

		window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_default_size (GTK_WINDOW (window), 500, 400);
		gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (theater->playlist));
		gtk_widget_show_all (window);
	}
#endif
	g_signal_connect (G_OBJECT (theater->playlist), "active-name-changed",
			  G_CALLBACK (on_playlist_change_name), theater);
	g_signal_connect (G_OBJECT (theater->playlist), "item-activated",
			  G_CALLBACK (item_activated_cb), theater);
	g_signal_connect (G_OBJECT (theater->playlist),
			  "changed", G_CALLBACK (playlist_changed_cb),
			  theater);
	g_signal_connect (G_OBJECT (theater->playlist),
			  "current-removed", G_CALLBACK (current_removed_cb),
			  theater);
	g_signal_connect (G_OBJECT (theater->playlist),
			  "notify::repeat",
			  G_CALLBACK (playlist_repeat_toggle_cb),
			  theater);
	g_signal_connect (G_OBJECT (theater->playlist),
			  "subtitle-changed",
			  G_CALLBACK (subtitle_changed_cb),
			  theater);
}

static void
grilo_show_back_button_changed (theaterGrilo  *grilo,
				GParamSpec  *spec,
				theaterObject *theater)
{
	if (g_strcmp0 (theater_object_get_main_page (theater), "grilo") == 0) {
		g_object_set (theater->header,
			      "show-back-button", theater_grilo_get_show_back_button (theater_GRILO (theater->grilo)),
			      NULL);
	}
}

static void
grilo_current_page_changed (theaterGrilo  *grilo,
			    GParamSpec  *spec,
			    theaterObject *theater)
{
	if (g_strcmp0 (theater_object_get_main_page (theater), "grilo") == 0) {
		theaterGriloPage page;

		page = theater_grilo_get_current_page (theater_GRILO (theater->grilo));
		gtk_widget_set_visible (theater->add_button,
					page == theater_GRILO_PAGE_RECENT);
	}
}

void
grilo_widget_setup (theaterObject *theater)
{
	theater->grilo = theater_grilo_new (theater, theater->header);
	g_signal_connect (G_OBJECT (theater->grilo), "notify::show-back-button",
			  G_CALLBACK (grilo_show_back_button_changed), theater);
	g_signal_connect (G_OBJECT (theater->grilo), "notify::current-page",
			  G_CALLBACK (grilo_current_page_changed), theater);
	gtk_stack_add_named (GTK_STACK (theater->stack),
			     theater->grilo,
			     "grilo");
	gtk_stack_set_visible_child_name (GTK_STACK (theater->stack), "grilo");
}

static void
add_fullscreen_toolbar (theaterObject *theater)
{
	GtkWidget *container, *item;
	GMenuModel *menu;

	container = GTK_WIDGET (bacon_video_widget_get_header_controls_object (theater->bvw));

	theater->fullscreen_header = g_object_new (theater_TYPE_MAIN_TOOLBAR,
						 "show-search-button", FALSE,
						 "show-select-button", FALSE,
						 "show-back-button", TRUE,
						 NULL);
	g_object_bind_property (theater->header, "title",
				theater->fullscreen_header, "title", 0);
	g_object_bind_property (theater->header, "subtitle",
				theater->fullscreen_header, "subtitle", 0);
	g_signal_connect (G_OBJECT (theater->fullscreen_header), "back-clicked",
			  G_CALLBACK (back_button_clicked_cb), theater);

	item = theater_interface_create_header_button (theater->fullscreen_header,
						     gtk_button_new (),
						     "view-restore-symbolic",
						     GTK_PACK_END);
	gtk_actionable_set_action_name (GTK_ACTIONABLE (item), "app.fullscreen");

	item = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_header_bar_pack_end (GTK_HEADER_BAR (theater->fullscreen_header), item);
	gtk_style_context_add_class (gtk_widget_get_style_context (item), "header-bar-separator");

	item = theater_interface_create_header_button (theater->fullscreen_header,
						     gtk_menu_button_new (),
						     "view-more-symbolic",
						     GTK_PACK_END);
	menu = (GMenuModel *) gtk_builder_get_object (theater->xml, "playermenu");
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (item), menu);
	g_signal_connect (G_OBJECT (item), "toggled",
			  G_CALLBACK (popup_menu_shown_cb), theater);
	theater->fullscreen_gear_button = item;

	gtk_container_add (GTK_CONTAINER (container), theater->fullscreen_header);
	gtk_widget_show_all (theater->fullscreen_header);
}

void
video_widget_create (theaterObject *theater)
{
	GError *err = NULL;
	GtkContainer *container;
	BaconVideoWidget **bvw;
	GdkWindow *window;
	gboolean fullscreen;

	theater->bvw = BACON_VIDEO_WIDGET (bacon_video_widget_new (&err));

	if (theater->bvw == NULL) {
		theater_object_show_error_and_exit (_("theater could not startup."), err != NULL ? err->message : _("No reason."), theater);
		if (err != NULL)
			g_error_free (err);
	}

	window = gtk_widget_get_window (theater->win);

	fullscreen = window && ((gdk_window_get_state (window) & GDK_WINDOW_STATE_FULLSCREEN) != 0);
	bacon_video_widget_set_fullscreen (theater->bvw, fullscreen);
	theater->controls = bacon_video_widget_get_controls_object (theater->bvw);

	g_signal_connect_after (G_OBJECT (theater->bvw),
			"button-press-event",
			G_CALLBACK (on_video_button_press_event),
			theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			"eos",
			G_CALLBACK (on_eos_event),
			theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			"got-redirect",
			G_CALLBACK (on_got_redirect),
			theater);
	g_signal_connect (G_OBJECT(theater->bvw),
			"channels-change",
			G_CALLBACK (on_channels_change_event),
			theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			"tick",
			G_CALLBACK (update_current_time),
			theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			"got-metadata",
			G_CALLBACK (on_got_metadata_event),
			theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			"buffering",
			G_CALLBACK (on_buffering_event),
			theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			"download-buffering",
			G_CALLBACK (on_download_buffering_event),
			theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			"error",
			G_CALLBACK (on_error_event),
			theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			  "seek-requested",
			  G_CALLBACK (on_seek_requested_event),
			  theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			  "track-skip-requested",
			  G_CALLBACK (on_track_skip_requested_event),
			  theater);
	g_signal_connect (G_OBJECT (theater->bvw),
			  "volume-change-requested",
			  G_CALLBACK (on_volume_change_requested_event),
			  theater);

	container = GTK_CONTAINER (gtk_builder_get_object (theater->xml, "tmw_bvw_box"));
	gtk_container_add (container,
			GTK_WIDGET (theater->bvw));

	add_fullscreen_toolbar (theater);

	/* Events for the widget video window as well */
	gtk_widget_add_events (GTK_WIDGET (theater->bvw),
			GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
	g_signal_connect (G_OBJECT(theater->bvw), "key_press_event",
			G_CALLBACK (window_key_press_event_cb), theater);
	g_signal_connect (G_OBJECT(theater->bvw), "key_release_event",
			G_CALLBACK (window_key_press_event_cb), theater);

	g_signal_connect (G_OBJECT (theater->bvw), "drag_data_received",
			G_CALLBACK (drop_video_cb), theater);
	gtk_drag_dest_set (GTK_WIDGET (theater->bvw), GTK_DEST_DEFAULT_ALL,
			   target_table, G_N_ELEMENTS (target_table),
			   GDK_ACTION_MOVE);

	bvw = &(theater->bvw);
	g_object_add_weak_pointer (G_OBJECT (theater->bvw),
				   (gpointer *) bvw);

	gtk_widget_show (GTK_WIDGET (theater->bvw));

	/* FIXME: Otherwise we get a visible but
	 * not realized widget ?!?! */
	gtk_widget_realize (GTK_WIDGET (theater->bvw));
}

/**
 * theater_object_get_supported_content_types:
 *
 * Get the full list of file content types which theater supports playing.
 *
 * Return value: (array zero-terminated=1) (transfer none): a %NULL-terminated array of the content types theater supports
 * Since: 3.1.5
 */
const gchar * const *
theater_object_get_supported_content_types (void)
{
	return mime_types;
}

/**
 * theater_object_get_supported_uri_schemes:
 *
 * Get the full list of URI schemes which theater supports accessing.
 *
 * Return value: (array zero-terminated=1) (transfer none): a %NULL-terminated array of the URI schemes theater supports
 * Since: 3.1.5
 */
const gchar * const *
theater_object_get_supported_uri_schemes (void)
{
	return uri_schemes;
}

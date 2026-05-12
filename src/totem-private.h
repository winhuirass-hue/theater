/* 
 * Copyright (C) 2001-2002 Bastien Nocera <hadess@hadess.net>
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

#ifndef __theater_PRIVATE_H__
#define __theater_PRIVATE_H__

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "theater-playlist.h"
#include "backend/bacon-video-widget.h"
#include "backend/bacon-time-label.h"
#include "theater-open-location.h"
#include "theater-plugins-engine.h"

#define theater_signal_block_by_data(obj, data) (g_signal_handlers_block_matched (obj, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data))
#define theater_signal_unblock_by_data(obj, data) (g_signal_handlers_unblock_matched (obj, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data))

#define theater_set_sensitivity(xml, name, state)					\
	{									\
		GtkWidget *widget;						\
		widget = GTK_WIDGET (gtk_builder_get_object (xml, name));	\
		gtk_widget_set_sensitive (widget, state);			\
	}
#define theater_controls_set_sensitivity(name, state) gtk_widget_set_sensitive (g_object_get_data (theater->controls, name), state)

#define theater_object_set_sensitivity2(name, state)					\
	{										\
		GAction *__action;							\
		__action = g_action_map_lookup_action (G_ACTION_MAP (theater), name);	\
		g_simple_action_set_enabled (G_SIMPLE_ACTION (__action), state);	\
	}

typedef enum {
	theater_CONTROLS_UNDEFINED,
	theater_CONTROLS_VISIBLE,
	theater_CONTROLS_FULLSCREEN
} ControlsVisibility;

typedef enum {
	STATE_PLAYING,
	STATE_PAUSED,
	STATE_STOPPED
} theaterStates;

struct _theaterObject {
	GtkApplication parent;

	/* Control window */
	GtkBuilder *xml;
	GtkWidget *win;
	GtkWidget *stack;
	BaconVideoWidget *bvw;
	GtkWidget *prefs;
	GtkBuilder *prefs_xml;
	GtkWindow *shortcuts_win;

	GtkWidget *grilo;

	GObject *controls;
	GtkWidget *play_button;
	BaconTimeLabel *time_label;
	BaconTimeLabel *time_rem_label;
	GtkWidget *header;

	GtkWidget *fullscreen_header;
	GtkWidget *fullscreen_gear_button;

	/* Plugins */
	GtkWidget *plugins;
	theaterPluginsEngine *engine;

	/* Seek */
	GtkWidget *seek;
	GtkAdjustment *seekadj;
	gboolean seek_lock;
	gboolean seekable;

	/* Volume */
	GtkWidget *volume;
	gboolean volume_sensitive;
	gboolean muted;
	double prev_volume;

	/* Subtitles/Languages menus */
	gboolean updating_menu;
	GList *subtitles_list;
	GList *languages_list;

	/* controls management */
	ControlsVisibility controls_visibility;

	/* Stream info */
	gint64 stream_length;

	/* Monitor for playlist unmounts and drives/volumes monitoring */
	GVolumeMonitor *monitor;
	gboolean drives_changed;

	/* session */
	gboolean pause_start;
	guint save_timeout_id;

	/* Window State */
	int window_w, window_h;
	gboolean maximised;

	/* Toolbar state */
	char *title;
	char *subtitle;
	char *search_string;
	gboolean select_mode;
	GObject *custom_title;
	GtkWidget *fullscreen_button;
	GtkWidget *gear_button;
	GtkWidget *add_button;
	GtkWidget *main_menu_button;

	char *player_title;

	/* other */
	char *mrl;
	char *next_subtitle;
	theaterPlaylist *playlist;
	GSettings *settings;
	theaterStates state;
	theaterOpenLocation *open_location;
	gboolean disable_kbd_shortcuts;
	gboolean has_played_emitted;
};

GtkWidget *theater_volume_create (void);

#define SEEK_FORWARD_OFFSET 60
#define SEEK_BACKWARD_OFFSET -15

#define VOLUME_DOWN_OFFSET (-0.08)
#define VOLUME_UP_OFFSET (0.08)

#define VOLUME_DOWN_SHORT_OFFSET (-0.02)
#define VOLUME_UP_SHORT_OFFSET (0.02)

#define ZOOM_IN_OFFSET 0.01
#define ZOOM_OUT_OFFSET -0.01

void	theater_object_open			(theater *theater);
void	theater_object_open_location		(theater *theater);
void	theater_object_eject			(theater *theater);
void	theater_object_set_zoom			(theater *theater, gboolean zoom);
void	theater_object_show_help			(theater *theater);
void	theater_object_show_keyboard_shortcuts	(theater *theater);
void	theater_object_show_properties		(theater *theater);
void    theater_object_set_mrl			(theaterObject *theater,
						 const char *mrl,
						 const char *subtitle);
gboolean theater_object_open_files		(theater *theater, char **list);
G_GNUC_NORETURN void theater_object_show_error_and_exit (const char *title, const char *reason, theater *theater);

void	show_controls				(theater *theater, gboolean was_fullscreen);

void	theater_setup_window			(theater *theater);
void	theater_callback_connect			(theater *theater);
void	playlist_widget_setup			(theater *theater);
void	grilo_widget_setup			(theater *theater);
void	video_widget_create			(theater *theater);
void    theater_object_plugins_init		(theaterObject *theater);
void    theater_object_plugins_shutdown		(theaterObject *theater);
void	theater_object_set_fullscreen		(theaterObject *theater, gboolean state);
void	theater_object_set_volume_relative	(theaterObject *theater, double off_pct);
void	theater_object_volume_toggle_mute		(theaterObject *theater);
void	theater_object_set_main_page		(theaterObject *theater,
						 const char  *page_id);
const char * theater_object_get_main_page		(theater *theater);
void	theater_object_add_items_to_playlist	(theaterObject *theater,
						 GList       *items);

/* Signal emission */
void	theater_file_has_played			(theaterObject *theater,
						 const char *mrl);

#endif /* __theater_PRIVATE_H__ */

/* 
 * Copyright (C) 2001,2002,2003,2004,2005 Bastien Nocera <hadess@hadess.net>
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

#ifndef __theater_H__
#define __theater_H__

#include <glib-object.h>
#include <gtk/gtk.h>

/**
 * theater_GSETTINGS_SCHEMA:
 *
 * The GSettings schema under which all theater settings are stored.
 **/
#define theater_GSETTINGS_SCHEMA "org.gnome.theater"

G_BEGIN_DECLS

/**
 * theaterRemoteCommand:
 * @theater_REMOTE_COMMAND_UNKNOWN: unknown command
 * @theater_REMOTE_COMMAND_PLAY: play the current stream
 * @theater_REMOTE_COMMAND_PAUSE: pause the current stream
 * @theater_REMOTE_COMMAND_STOP: stop playing the current stream
 * @theater_REMOTE_COMMAND_PLAYPAUSE: toggle play/pause on the current stream
 * @theater_REMOTE_COMMAND_NEXT: play the next playlist item
 * @theater_REMOTE_COMMAND_PREVIOUS: play the previous playlist item
 * @theater_REMOTE_COMMAND_SEEK_FORWARD: seek forwards in the current stream
 * @theater_REMOTE_COMMAND_SEEK_BACKWARD: seek backwards in the current stream
 * @theater_REMOTE_COMMAND_VOLUME_UP: increase the volume
 * @theater_REMOTE_COMMAND_VOLUME_DOWN: decrease the volume
 * @theater_REMOTE_COMMAND_FULLSCREEN: toggle fullscreen mode
 * @theater_REMOTE_COMMAND_QUIT: quit the instance of theater
 * @theater_REMOTE_COMMAND_ENQUEUE: enqueue a new playlist item
 * @theater_REMOTE_COMMAND_REPLACE: replace an item in the playlist
 * @theater_REMOTE_COMMAND_SHOW: show the theater instance
 * @theater_REMOTE_COMMAND_UP: go up (DVD controls)
 * @theater_REMOTE_COMMAND_DOWN: go down (DVD controls)
 * @theater_REMOTE_COMMAND_LEFT: go left (DVD controls)
 * @theater_REMOTE_COMMAND_RIGHT: go right (DVD controls)
 * @theater_REMOTE_COMMAND_SELECT: select the current item (DVD controls)
 * @theater_REMOTE_COMMAND_DVD_MENU: go to the DVD menu
 * @theater_REMOTE_COMMAND_ZOOM_UP: increase the zoom level
 * @theater_REMOTE_COMMAND_ZOOM_DOWN: decrease the zoom level
 * @theater_REMOTE_COMMAND_EJECT: eject the current disc
 * @theater_REMOTE_COMMAND_PLAY_DVD: play a DVD in a drive
 * @theater_REMOTE_COMMAND_MUTE: toggle mute
 * @theater_REMOTE_COMMAND_TOGGLE_ASPECT: toggle the aspect ratio
 *
 * Represents a command which can be sent to a running theater instance remotely.
 **/
typedef enum {
	theater_REMOTE_COMMAND_UNKNOWN = 0,
	theater_REMOTE_COMMAND_PLAY,
	theater_REMOTE_COMMAND_PAUSE,
	theater_REMOTE_COMMAND_STOP,
	theater_REMOTE_COMMAND_PLAYPAUSE,
	theater_REMOTE_COMMAND_NEXT,
	theater_REMOTE_COMMAND_PREVIOUS,
	theater_REMOTE_COMMAND_SEEK_FORWARD,
	theater_REMOTE_COMMAND_SEEK_BACKWARD,
	theater_REMOTE_COMMAND_VOLUME_UP,
	theater_REMOTE_COMMAND_VOLUME_DOWN,
	theater_REMOTE_COMMAND_FULLSCREEN,
	theater_REMOTE_COMMAND_QUIT,
	theater_REMOTE_COMMAND_ENQUEUE,
	theater_REMOTE_COMMAND_REPLACE,
	theater_REMOTE_COMMAND_SHOW,
	theater_REMOTE_COMMAND_UP,
	theater_REMOTE_COMMAND_DOWN,
	theater_REMOTE_COMMAND_LEFT,
	theater_REMOTE_COMMAND_RIGHT,
	theater_REMOTE_COMMAND_SELECT,
	theater_REMOTE_COMMAND_DVD_MENU,
	theater_REMOTE_COMMAND_ZOOM_UP,
	theater_REMOTE_COMMAND_ZOOM_DOWN,
	theater_REMOTE_COMMAND_EJECT,
	theater_REMOTE_COMMAND_PLAY_DVD,
	theater_REMOTE_COMMAND_MUTE,
	theater_REMOTE_COMMAND_TOGGLE_ASPECT
} theaterRemoteCommand;

/**
 * theaterRemoteSetting:
 * @theater_REMOTE_SETTING_REPEAT: whether repeat is enabled
 *
 * Represents a boolean setting or preference on a remote theater instance.
 **/
typedef enum {
	theater_REMOTE_SETTING_REPEAT
} theaterRemoteSetting;

GType theater_remote_command_get_type	(void);
GQuark theater_remote_command_quark	(void);
#define theater_TYPE_REMOTE_COMMAND	(theater_remote_command_get_type())
#define theater_REMOTE_COMMAND		theater_remote_command_quark ()

GType theater_remote_setting_get_type	(void);
GQuark theater_remote_setting_quark	(void);
#define theater_TYPE_REMOTE_SETTING	(theater_remote_setting_get_type())
#define theater_REMOTE_SETTING		theater_remote_setting_quark ()

#define theater_TYPE_OBJECT              (theater_object_get_type ())
#define theater_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), theater_object_get_type (), theaterObject))
#define theater_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), theater_object_get_type (), theaterObjectClass))
#define theater_IS_OBJECT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE (obj, theater_object_get_type ()))
#define theater_IS_OBJECT_CLASS(klass)   (G_CHECK_INSTANCE_GET_CLASS ((klass), theater_object_get_type ()))

/**
 * theater:
 *
 * The #theater object is a handy synonym for #theaterObject, and the two can be used interchangably.
 **/

/**
 * theaterObject:
 *
 * All the fields in the #theaterObject structure are private and should never be accessed directly.
 **/
typedef struct _theaterObject theater;
typedef struct _theaterObject theaterObject;

typedef struct {
	GtkApplicationClass parent_class;

	void (*file_opened)			(theaterObject *theater, const char *mrl);
	void (*file_closed)			(theaterObject *theater);
	void (*file_has_played)			(theaterObject *theater, const char *mrl);
	void (*metadata_updated)		(theaterObject *theater,
						 const char *artist,
						 const char *title,
						 const char *album,
						 guint track_num);
	char * (*get_user_agent)		(theaterObject *theater,
						 const char  *mrl);
	char * (*get_text_subtitle)		(theaterObject *theater,
						 const char  *mrl);
} theaterObjectClass;

GType	theater_object_get_type			(void);

void	theater_object_exit			(theaterObject *theater) G_GNUC_NORETURN;
void	theater_object_play			(theaterObject *theater);
void	theater_object_stop			(theaterObject *theater);
void	theater_object_play_pause			(theaterObject *theater);
void	theater_object_pause			(theaterObject *theater);
gboolean theater_object_can_seek_next		(theaterObject *theater);
void	theater_object_seek_next			(theaterObject *theater);
gboolean theater_object_can_seek_previous		(theaterObject *theater);
void	theater_object_seek_previous		(theaterObject *theater);
void	theater_object_seek_time			(theaterObject *theater, gint64 msec, gboolean accurate);
void	theater_object_seek_relative		(theaterObject *theater, gint64 offset, gboolean accurate);
double	theater_object_get_volume			(theaterObject *theater);
void	theater_object_set_volume			(theaterObject *theater, double volume);

void	theater_object_next_angle			(theaterObject *theater);

void    theater_object_show_error			(theaterObject *theater,
						 const char *title,
						 const char *reason);

gboolean theater_object_is_fullscreen		(theaterObject *theater);
gboolean theater_object_is_playing		(theaterObject *theater);
gboolean theater_object_is_paused			(theaterObject *theater);
gboolean theater_object_is_seekable		(theaterObject *theater);
GtkWindow *theater_object_get_main_window		(theaterObject *theater);
GMenu *theater_object_get_menu_section		(theaterObject *theater,
						 const char  *id);
void theater_object_empty_menu_section		(theaterObject *theater,
						 const char  *id);

float		theater_object_get_rate		(theaterObject *theater);
gboolean	theater_object_set_rate		(theaterObject *theater, float rate);

GtkWidget *theater_object_get_video_widget	(theaterObject *theater);

/* Database handling */
void	theater_object_add_to_view		(theaterObject *theater,
						 GFile       *file,
						 const char  *title);

/* Current media information */
char *	theater_object_get_short_title		(theaterObject *theater);
gint64	theater_object_get_current_time		(theaterObject *theater);

/* Playlist handling */
guint	theater_object_get_playlist_length	(theaterObject *theater);
int	theater_object_get_playlist_pos		(theaterObject *theater);
char *	theater_object_get_title_at_playlist_pos	(theaterObject *theater,
						 guint playlist_index);
void	theater_object_clear_playlist		(theaterObject *theater);
void	theater_object_add_to_playlist		(theaterObject *theater,
						 const char  *uri,
						 const char  *display_name,
						 gboolean     play);
char *  theater_object_get_current_mrl		(theaterObject *theater);
void	theater_object_set_current_subtitle	(theaterObject *theater,
						 const char *subtitle_uri);
/* Remote actions */
void    theater_object_remote_command		(theaterObject *theater,
						 theaterRemoteCommand cmd,
						 const char *url);
void	theater_object_remote_set_setting		(theaterObject *theater,
						 theaterRemoteSetting setting,
						 gboolean value);
gboolean theater_object_remote_get_setting	(theaterObject *theater,
						 theaterRemoteSetting setting);

const gchar * const *theater_object_get_supported_content_types (void);
const gchar * const *theater_object_get_supported_uri_schemes (void);

#endif /* __theater_H__ */

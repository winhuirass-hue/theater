/* theater-playlist.h: Simple playlist dialog

   Copyright (C) 2002, 2003, 2004, 2005 Bastien Nocera <hadess@hadess.net>

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#ifndef theater_PLAYLIST_H
#define theater_PLAYLIST_H

#include <gtk/gtk.h>
#include <theater-pl-parser.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define theater_TYPE_PLAYLIST            (theater_playlist_get_type ())
#define theater_PLAYLIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), theater_TYPE_PLAYLIST, theaterPlaylist))
#define theater_PLAYLIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), theater_TYPE_PLAYLIST, theaterPlaylistClass))
#define theater_IS_PLAYLIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), theater_TYPE_PLAYLIST))
#define theater_IS_PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), theater_TYPE_PLAYLIST))

typedef enum {
	theater_PLAYLIST_STATUS_NONE,
	theater_PLAYLIST_STATUS_PLAYING,
	theater_PLAYLIST_STATUS_PAUSED
} theaterPlaylistStatus;

typedef enum {
	theater_PLAYLIST_DIRECTION_NEXT,
	theater_PLAYLIST_DIRECTION_PREVIOUS
} theaterPlaylistDirection;

typedef enum {
	theater_PLAYLIST_DIALOG_SELECTED,
	theater_PLAYLIST_DIALOG_PLAYING
} theaterPlaylistSelectDialog;


typedef struct theaterPlaylist	       theaterPlaylist;
typedef struct theaterPlaylistClass      theaterPlaylistClass;
typedef struct theaterPlaylistPrivate    theaterPlaylistPrivate;

typedef void (*theaterPlaylistForeachFunc) (theaterPlaylist *playlist,
					  const gchar   *filename,
					  const gchar   *uri,
					  gpointer       user_data);

struct theaterPlaylist {
	GtkBox parent;
	theaterPlaylistPrivate *priv;
};

struct theaterPlaylistClass {
	GtkBoxClass parent_class;

	void (*changed) (theaterPlaylist *playlist);
	void (*item_activated) (theaterPlaylist *playlist);
	void (*active_name_changed) (theaterPlaylist *playlist);
	void (*current_removed) (theaterPlaylist *playlist);
	void (*subtitle_changed) (theaterPlaylist *playlist);
	void (*item_added) (theaterPlaylist *playlist, const gchar *filename, const gchar *uri);
	void (*item_removed) (theaterPlaylist *playlist, const gchar *filename, const gchar *uri);
};

GType    theater_playlist_get_type (void);
GtkWidget *theater_playlist_new      (void);

/* The application is responsible for checking that the mrl is correct
 * @display_name is if you have a preferred display string for the mrl,
 * NULL otherwise
 */
void theater_playlist_add_mrl (theaterPlaylist *playlist,
                             const char *mrl,
                             const char *display_name,
                             gboolean cursor,
                             GCancellable *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer user_data);
gboolean theater_playlist_add_mrl_finish (theaterPlaylist *playlist,
                                        GAsyncResult  *result,
                                        GError       **error);
gboolean theater_playlist_add_mrl_sync (theaterPlaylist *playlist,
                                      const char *mrl);

typedef struct theaterPlaylistMrlData theaterPlaylistMrlData;

theaterPlaylistMrlData *theater_playlist_mrl_data_new (const gchar *mrl,
                                                   const gchar *display_name);
void theater_playlist_mrl_data_free (theaterPlaylistMrlData *data);

void theater_playlist_add_mrls (theaterPlaylist *self,
                              GList *mrls,
                              gboolean cursor,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer user_data);
gboolean theater_playlist_add_mrls_finish (theaterPlaylist *self,
                                         GAsyncResult *result,
                                         GError **error);

void theater_playlist_save_session_playlist (theaterPlaylist *playlist,
					   GFile         *output,
					   gint64         starttime);
void theater_playlist_select_subtitle_dialog (theaterPlaylist *playlist,
					    theaterPlaylistSelectDialog mode);

/* theater_playlist_clear doesn't emit the current_removed signal, even if it does
 * because the caller should know what to do after it's done with clearing */
gboolean   theater_playlist_clear (theaterPlaylist *playlist);
void       theater_playlist_clear_with_g_mount (theaterPlaylist *playlist,
					      GMount *mount);
char      *theater_playlist_get_current_mrl (theaterPlaylist *playlist,
					   char **subtitle);
char      *theater_playlist_get_current_title (theaterPlaylist *playlist);
char      *theater_playlist_get_current_content_type (theaterPlaylist *playlist);
gint64     theater_playlist_steal_current_starttime (theaterPlaylist *playlist);
char      *theater_playlist_get_title (theaterPlaylist *playlist,
				     guint title_index);

gboolean   theater_playlist_set_title (theaterPlaylist *playlist,
				     const char *title);
void       theater_playlist_set_current_subtitle (theaterPlaylist *playlist,
						const char *subtitle_uri);

#define    theater_playlist_has_direction(playlist, direction) (direction == theater_PLAYLIST_DIRECTION_NEXT ? theater_playlist_has_next_mrl (playlist) : theater_playlist_has_previous_mrl (playlist))
gboolean   theater_playlist_has_previous_mrl (theaterPlaylist *playlist);
gboolean   theater_playlist_has_next_mrl (theaterPlaylist *playlist);

#define    theater_playlist_set_direction(playlist, direction) (direction == theater_PLAYLIST_DIRECTION_NEXT ? theater_playlist_set_next (playlist) : theater_playlist_set_previous (playlist))
void       theater_playlist_set_previous (theaterPlaylist *playlist);
void       theater_playlist_set_next (theaterPlaylist *playlist);

gboolean   theater_playlist_get_repeat (theaterPlaylist *playlist);
void       theater_playlist_set_repeat (theaterPlaylist *playlist, gboolean repeat);

gboolean   theater_playlist_set_playing (theaterPlaylist *playlist, theaterPlaylistStatus state);
theaterPlaylistStatus theater_playlist_get_playing (theaterPlaylist *playlist);

void       theater_playlist_set_at_start (theaterPlaylist *playlist);
void       theater_playlist_set_at_end (theaterPlaylist *playlist);

int        theater_playlist_get_current (theaterPlaylist *playlist);
int        theater_playlist_get_last (theaterPlaylist *playlist);
void       theater_playlist_set_current (theaterPlaylist *playlist, guint current_index);

G_END_DECLS

#endif /* theater_PLAYLIST_H */

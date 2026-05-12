/* theater-session.c

   Copyright (C) 2004 Bastien Nocera

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

#include "config.h"

#include "theater.h"
#include "theater-private.h"
#include "theater-session.h"
#include "theater-uri.h"

static GFile *session_file = NULL;

static GFile *
get_session_file (void)
{
	char *path;

	if (session_file)
		return session_file;

	path = g_build_filename (theater_dot_dir (), "session_state.xspf", NULL);
	session_file = g_file_new_for_path (path);
	g_free (path);

	return session_file;
}

static char *
get_session_filename (void)
{
	return g_file_get_uri (get_session_file ());
}

gboolean
theater_session_try_restore (theater *theater)
{
	char *uri;
	char *mrl, *subtitle;

	theater_signal_block_by_data (theater->playlist, theater);
	theater->pause_start = TRUE;

	/* Possibly the only place in theater where it makes sense to add an MRL to the playlist synchronously, since we haven't yet entered
	 * the GTK+ main loop, and thus can't freeze the application. */
	uri = get_session_filename ();
	if (theater_playlist_add_mrl_sync (theater->playlist, uri) == FALSE) {
		theater->pause_start = FALSE;
		theater_signal_unblock_by_data (theater->playlist, theater);
		theater_object_set_mrl (theater, NULL, NULL);
		g_free (uri);
		return FALSE;
	}
	g_free (uri);

	theater_signal_unblock_by_data (theater->playlist, theater);

	subtitle = NULL;
	mrl = theater_playlist_get_current_mrl (theater->playlist, &subtitle);

	if (mrl != NULL)
		theater_object_set_main_page (theater, "player");

	theater_object_set_mrl (theater, mrl, subtitle);

	/* We do the seeking after being told that the stream is seekable,
	 * not straight away */

	g_free (mrl);
	g_free (subtitle);

	return TRUE;
}

void
theater_session_save (theater *theater)
{
	GFile *file;
	gint64 curr = -1;

	if (theater->bvw == NULL)
		return;

	file = get_session_file ();
	if (!theater_playing_dvd (theater->mrl))
		curr = bacon_video_widget_get_current_time (theater->bvw);
	theater_playlist_save_session_playlist (theater->playlist, file, curr / 1000);
}

void
theater_session_cleanup (theater *theater)
{
	g_file_delete (get_session_file (), NULL, NULL);
	g_clear_object (&session_file);
}

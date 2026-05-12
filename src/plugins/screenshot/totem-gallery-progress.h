/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2008 Philip Withnall <philip@tecnocode.co.uk>
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
 */

#ifndef theater_GALLERY_PROGRESS_H
#define theater_GALLERY_PROGRESS_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define theater_TYPE_GALLERY_PROGRESS		(theater_gallery_progress_get_type ())
#define theater_GALLERY_PROGRESS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), theater_TYPE_GALLERY_PROGRESS, theaterGalleryProgress))
#define theater_GALLERY_PROGRESS_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), theater_TYPE_GALLERY_PROGRESS, theaterGalleryProgressClass))
#define theater_IS_GALLERY_PROGRESS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), theater_TYPE_GALLERY_PROGRESS))
#define theater_IS_GALLERY_PROGRESS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), theater_TYPE_GALLERY_PROGRESS))
#define theater_GALLERY_PROGRESS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), theater_TYPE_GALLERY_PROGRESS, theaterGalleryProgressClass))

typedef struct _theaterGalleryProgressPrivate	theaterGalleryProgressPrivate;

typedef struct {
	GtkDialog parent;
	theaterGalleryProgressPrivate *priv;
} theaterGalleryProgress;

typedef struct {
	GtkDialogClass parent;
} theaterGalleryProgressClass;

GType theater_gallery_progress_get_type (void);
theaterGalleryProgress *theater_gallery_progress_new (GPid child_pid, const gchar *output_filename);
void theater_gallery_progress_run (theaterGalleryProgress *self, gint stdout_fd);

G_END_DECLS

#endif /* !theater_GALLERY_PROGRESS_H */

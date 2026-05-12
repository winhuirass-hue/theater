/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Philip Withnall <philip@tecnocode.co.uk>
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
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Bastien Nocera <hadess@hadess.net>, Philip Withnall <philip@tecnocode.co.uk>
 */

#ifndef theater_OPEN_LOCATION_H
#define theater_OPEN_LOCATION_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define theater_TYPE_OPEN_LOCATION		(theater_open_location_get_type ())
#define theater_OPEN_LOCATION(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), theater_TYPE_OPEN_LOCATION, theaterOpenLocation))
#define theater_OPEN_LOCATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), theater_TYPE_OPEN_LOCATION, theaterOpenLocationClass))
#define theater_IS_OPEN_LOCATION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), theater_TYPE_OPEN_LOCATION))
#define theater_IS_OPEN_LOCATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), theater_TYPE_OPEN_LOCATION))

typedef struct theaterOpenLocation		theaterOpenLocation;
typedef struct theaterOpenLocationClass		theaterOpenLocationClass;
typedef struct theaterOpenLocationPrivate		theaterOpenLocationPrivate;

struct theaterOpenLocation {
	GtkDialog parent;
	theaterOpenLocationPrivate *priv;
};

struct theaterOpenLocationClass {
	GtkDialogClass parent_class;
};

GType theater_open_location_get_type		(void);
GtkWidget *theater_open_location_new		(void);
char *theater_open_location_get_uri		(theaterOpenLocation *open_location);

G_END_DECLS

#endif /* theater_OPEN_LOCATION_H */

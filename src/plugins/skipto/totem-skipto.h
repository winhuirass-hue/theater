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
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Bastien Nocera <hadess@hadess.net>, Philip Withnall <philip@tecnocode.co.uk>
 */

#ifndef theater_SKIPTO_H
#define theater_SKIPTO_H

#include <gtk/gtk.h>

#include "theater.h"

G_BEGIN_DECLS

#define theater_TYPE_SKIPTO		(theater_skipto_get_type ())
#define theater_SKIPTO(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), theater_TYPE_SKIPTO, theaterSkipto))
#define theater_SKIPTO_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), theater_TYPE_SKIPTO, theaterSkiptoClass))
#define theater_IS_SKIPTO(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), theater_TYPE_SKIPTO))
#define theater_IS_SKIPTO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), theater_TYPE_SKIPTO))

GType theater_skipto_register_type	(GTypeModule *module);

typedef struct theaterSkipto		theaterSkipto;
typedef struct theaterSkiptoClass		theaterSkiptoClass;
typedef struct theaterSkiptoPrivate	theaterSkiptoPrivate;

struct theaterSkipto {
	GtkDialog parent;
	theaterSkiptoPrivate *priv;
};

struct theaterSkiptoClass {
	GtkDialogClass parent_class;
};

GType theater_skipto_get_type	(void);
GtkWidget *theater_skipto_new	(theaterObject *theater);
gint64 theater_skipto_get_range	(theaterSkipto *skipto);
void theater_skipto_update_range	(theaterSkipto *skipto, gint64 _time);
void theater_skipto_set_seekable	(theaterSkipto *skipto, gboolean seekable);
void theater_skipto_set_current	(theaterSkipto *skipto, gint64 _time);

G_END_DECLS

#endif /* theater_SKIPTO_H */

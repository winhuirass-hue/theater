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
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Philip Withnall <philip@tecnocode.co.uk>
 */

#ifndef theater_TIME_ENTRY_H
#define theater_TIME_ENTRY_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define theater_TYPE_TIME_ENTRY		(theater_time_entry_get_type ())
#define theater_TIME_ENTRY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), theater_TYPE_TIME_ENTRY, theaterTimeEntry))
#define theater_TIME_ENTRY_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), theater_TYPE_TIME_ENTRY, theaterTimeEntryClass))
#define theater_IS_TIME_ENTRY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), theater_TYPE_TIME_ENTRY))
#define theater_IS_TIME_ENTRY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), theater_TYPE_TIME_ENTRY))
#define theater_TIME_ENTRY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), theater_TYPE_TIME_ENTRY, theaterTimeEntryClass))

typedef struct theaterTimeEntryPrivate	theaterTimeEntryPrivate;

typedef struct {
	GtkSpinButton parent;
	theaterTimeEntryPrivate *priv;
} theaterTimeEntry;

typedef struct {
	GtkSpinButtonClass parent;
} theaterTimeEntryClass;

GType theater_time_entry_get_type (void);
GtkWidget *theater_time_entry_new (GtkAdjustment *adjustment, gdouble climb_rate);

G_END_DECLS

#endif /* !theater_TIME_ENTRY_H */

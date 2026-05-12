/* -*- Mode: C; indent-tabs-mode: t -*- */

/*
 * Copyright (C) 2010, 2011 Igalia S.L. <info@igalia.com>
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
 * The theater project hereby grant permission for non-GPL compatible GStreamer
 * plugins to be used and distributed together with GStreamer and theater. This
 * permission are above and beyond the permissions granted by the GPL license
 * theater is covered by.
 *
 * See license_change file for details.
 */

#ifndef theater_GRILO_H
#define theater_GRILO_H

#include <gtk/gtk.h>
#include <theater.h>

G_BEGIN_DECLS

#define theater_TYPE_GRILO                 (theater_grilo_get_type ())
#define theater_GRILO(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), theater_TYPE_GRILO, theaterGrilo))
#define theater_GRILO_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), theater_TYPE_GRILO, theaterGriloClass))
#define theater_IS_GRILO(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), theater_TYPE_GRILO))
#define theater_IS_GRILO_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), theater_TYPE_GRILO))
#define theater_GRILO_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), theater_TYPE_GRILO, theaterGriloClass))

typedef struct _theaterGrilo        theaterGrilo;
typedef struct _theaterGriloPrivate theaterGriloPrivate;
typedef struct _theaterGriloClass   theaterGriloClass;

struct _theaterGrilo
{
  /*< private >*/
  GtkBox parent;

  theaterGriloPrivate *priv;
};

struct _theaterGriloClass
{
  GtkBoxClass parent_class;
};

typedef enum{
  theater_GRILO_PAGE_RECENT,
  theater_GRILO_PAGE_CHANNELS
} theaterGriloPage;

GType           theater_grilo_get_type              (void) G_GNUC_CONST;
GtkWidget*      theater_grilo_new                   (theaterObject *theater,
                                                   GtkWidget   *header);
void            theater_grilo_start                 (theaterGrilo  *self);
void            theater_grilo_pause                 (theaterGrilo  *self);
void            theater_grilo_back_button_clicked   (theaterGrilo  *self);
gboolean        theater_grilo_get_show_back_button  (theaterGrilo  *self);
void            theater_grilo_set_current_page      (theaterGrilo     *self,
                                                   theaterGriloPage  page);
theaterGriloPage  theater_grilo_get_current_page      (theaterGrilo     *self);
gboolean        theater_grilo_add_item_to_recent    (theaterGrilo     *self,
                                                   const char     *uri,
                                                   const char     *title,
                                                   gboolean        is_web);

G_END_DECLS

#endif /* theater_GRILO_H */

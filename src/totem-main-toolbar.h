/* GTK - The GIMP Toolkit
 * Copyright (C) 2013-2014 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 2013.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __theater_MAIN_TOOLBAR_H__
#define __theater_MAIN_TOOLBAR_H__

#include <gtk/gtkbox.h>

G_BEGIN_DECLS

#define theater_TYPE_MAIN_TOOLBAR                 (theater_main_toolbar_get_type ())
#define theater_MAIN_TOOLBAR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), theater_TYPE_MAIN_TOOLBAR, theaterMainToolbar))
#define theater_MAIN_TOOLBAR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), theater_TYPE_MAIN_TOOLBAR, theaterMainToolbarClass))
#define theater_IS_MAIN_TOOLBAR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), theater_TYPE_MAIN_TOOLBAR))
#define theater_IS_MAIN_TOOLBAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), theater_TYPE_MAIN_TOOLBAR))
#define theater_MAIN_TOOLBAR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), theater_TYPE_MAIN_TOOLBAR, theaterMainToolbarClass))

typedef struct _theaterMainToolbar        theaterMainToolbar;
typedef struct _theaterMainToolbarPrivate theaterMainToolbarPrivate;
typedef struct _theaterMainToolbarClass   theaterMainToolbarClass;

struct _theaterMainToolbar
{
  /*< private >*/
  GtkHeaderBar parent;

  theaterMainToolbarPrivate *priv;
};

struct _theaterMainToolbarClass
{
  GtkHeaderBarClass parent_class;
};

GType           theater_main_toolbar_get_type              (void) G_GNUC_CONST;
GtkWidget*      theater_main_toolbar_new                   (void);
void            theater_main_toolbar_set_search_mode       (theaterMainToolbar *bar,
							  gboolean          search_mode);
gboolean        theater_main_toolbar_get_search_mode       (theaterMainToolbar *bar);
void            theater_main_toolbar_set_select_mode       (theaterMainToolbar *bar,
							  gboolean          select_mode);
gboolean        theater_main_toolbar_get_select_mode       (theaterMainToolbar *bar);
void            theater_main_toolbar_set_title             (theaterMainToolbar *bar,
							  const char       *title);
const char *    theater_main_toolbar_get_title             (theaterMainToolbar *bar);
void            theater_main_toolbar_set_subtitle          (theaterMainToolbar *bar,
							  const char       *subtitle);
const char *    theater_main_toolbar_get_subtitle          (theaterMainToolbar *bar);
void            theater_main_toolbar_set_search_string     (theaterMainToolbar *bar,
						          const char       *search_string);
const char *    theater_main_toolbar_get_search_string     (theaterMainToolbar *bar);
void            theater_main_toolbar_set_n_selected        (theaterMainToolbar *bar,
							  guint             n_selected);
guint           theater_main_toolbar_get_n_selected        (theaterMainToolbar *bar);
void            theater_main_toolbar_set_custom_title      (theaterMainToolbar *bar,
							  GtkWidget        *title_widget);
GtkWidget *     theater_main_toolbar_get_custom_title      (theaterMainToolbar *bar);
void            theater_main_toolbar_set_select_menu_model (theaterMainToolbar *bar,
							  GMenuModel       *model);
GMenuModel *    theater_main_toolbar_get_select_menu_model (theaterMainToolbar *bar);
void            theater_main_toolbar_pack_start            (theaterMainToolbar *bar,
							  GtkWidget        *child);
void            theater_main_toolbar_pack_end              (theaterMainToolbar *bar,
							  GtkWidget        *child);

G_END_DECLS

#endif /* __theater_MAIN_TOOLBAR_H__ */

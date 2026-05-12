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

#ifndef __theater_SELECTION_TOOLBAR_H__
#define __theater_SELECTION_TOOLBAR_H__

#include <gtk/gtkbox.h>

G_BEGIN_DECLS

#define theater_TYPE_SELECTION_TOOLBAR                 (theater_selection_toolbar_get_type ())
#define theater_SELECTION_TOOLBAR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), theater_TYPE_SELECTION_TOOLBAR, theaterSelectionToolbar))
#define theater_SELECTION_TOOLBAR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), theater_TYPE_SELECTION_TOOLBAR, theaterSelectionToolbarClass))
#define theater_IS_SELECTION_TOOLBAR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), theater_TYPE_SELECTION_TOOLBAR))
#define theater_IS_SELECTION_TOOLBAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), theater_TYPE_SELECTION_TOOLBAR))
#define theater_SELECTION_TOOLBAR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), theater_TYPE_SELECTION_TOOLBAR, theaterSelectionToolbarClass))

typedef struct _theaterSelectionToolbar        theaterSelectionToolbar;
typedef struct _theaterSelectionToolbarPrivate theaterSelectionToolbarPrivate;
typedef struct _theaterSelectionToolbarClass   theaterSelectionToolbarClass;

struct _theaterSelectionToolbar
{
  /*< private >*/
  GtkActionBar parent;

  theaterSelectionToolbarPrivate *priv;
};

struct _theaterSelectionToolbarClass
{
  GtkActionBarClass parent_class;
};

GType           theater_selection_toolbar_get_type               (void) G_GNUC_CONST;
GtkWidget*      theater_selection_toolbar_new                    (void);

void            theater_selection_toolbar_set_n_selected         (theaterSelectionToolbar *bar,
                                                               guint                  n_selected);
guint           theater_selection_toolbar_get_n_selected         (theaterSelectionToolbar *bar);

void            theater_selection_toolbar_set_show_delete_button (theaterSelectionToolbar *bar,
                                                                gboolean               show_delete_button);
gboolean        theater_selection_toolbar_get_show_delete_button (theaterSelectionToolbar *bar);

void            theater_selection_toolbar_set_delete_button_sensitive (theaterSelectionToolbar *bar,
                                                                     gboolean               sensitive);
gboolean        theater_selection_toolbar_get_delete_button_sensitive (theaterSelectionToolbar *bar);

G_END_DECLS

#endif /* __theater_SELECTION_TOOLBAR_H__ */

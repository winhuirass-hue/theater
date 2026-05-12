/*
 * Copyright (c) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#ifndef __theater_SEARCH_ENTRY_H__
#define __theater_SEARCH_ENTRY_H__

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define theater_TYPE_SEARCH_ENTRY theater_search_entry_get_type()
#define theater_SEARCH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), theater_TYPE_SEARCH_ENTRY, theaterSearchEntry))
#define theater_SEARCH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), theater_TYPE_SEARCH_ENTRY, theaterSearchEntryClass))
#define theater_IS_SEARCH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), theater_TYPE_SEARCH_ENTRY))
#define theater_IS_SEARCH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), theater_TYPE_SEARCH_ENTRY))
#define theater_SEARCH_ENTRY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), theater_TYPE_SEARCH_ENTRY, theaterSearchEntryClass))

typedef struct _theaterSearchEntry theaterSearchEntry;
typedef struct _theaterSearchEntryClass theaterSearchEntryClass;
typedef struct _theaterSearchEntryPrivate theaterSearchEntryPrivate;

struct _theaterSearchEntry
{
	GtkBox parent;

	theaterSearchEntryPrivate *priv;
};

struct _theaterSearchEntryClass
{
	GtkBoxClass parent_class;
};

GType theater_search_entry_get_type              (void) G_GNUC_CONST;

theaterSearchEntry *theater_search_entry_new       (void);

void theater_search_entry_add_source             (theaterSearchEntry *entry,
						const gchar      *id,
						const gchar      *label,
						int               priority);
void theater_search_entry_remove_source          (theaterSearchEntry *self,
						const gchar      *id);

const char *theater_search_entry_get_selected_id (theaterSearchEntry *self);
gboolean    theater_search_entry_set_selected_id (theaterSearchEntry *self,
						const char       *id);

const char *theater_search_entry_get_text        (theaterSearchEntry *self);
GtkEntry   *theater_search_entry_get_entry       (theaterSearchEntry *self);

G_END_DECLS

#endif /* __theater_SEARCH_ENTRY_H__ */

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Plugin engine for theater, heavily based on the code from Rhythmbox,
 * which is based heavily on the code from theater.
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 *               2006 James Livingston  <jrl@ids.org.au>
 *               2007 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Sunday 13th May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef __theater_PLUGINS_ENGINE_H__
#define __theater_PLUGINS_ENGINE_H__

#include <glib.h>
#include <libpeas/peas-engine.h>
#include <theater.h>

G_BEGIN_DECLS

#define theater_TYPE_PLUGINS_ENGINE              (theater_plugins_engine_get_type ())
#define theater_PLUGINS_ENGINE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), theater_TYPE_PLUGINS_ENGINE, theaterPluginsEngine))
#define theater_PLUGINS_ENGINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), theater_TYPE_PLUGINS_ENGINE, theaterPluginsEngineClass))
#define theater_IS_PLUGINS_ENGINE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), theater_TYPE_PLUGINS_ENGINE))
#define theater_IS_PLUGINS_ENGINE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), theater_TYPE_PLUGINS_ENGINE))
#define theater_PLUGINS_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), theater_TYPE_PLUGINS_ENGINE, theaterPluginsEngineClass))

typedef struct _theaterPluginsEngine		theaterPluginsEngine;
typedef struct _theaterPluginsEnginePrivate	theaterPluginsEnginePrivate;
typedef struct _theaterPluginsEngineClass		theaterPluginsEngineClass;

struct _theaterPluginsEngine
{
	PeasEngine parent;
	theaterPluginsEnginePrivate *priv;
};

struct _theaterPluginsEngineClass
{
	PeasEngineClass parent_class;
};

GType			theater_plugins_engine_get_type			(void) G_GNUC_CONST;
theaterPluginsEngine	*theater_plugins_engine_get_default		(theaterObject *theater);
void			theater_plugins_engine_shut_down			(theaterPluginsEngine *self);

G_END_DECLS

#endif  /* __theater_PLUGINS_ENGINE_H__ */


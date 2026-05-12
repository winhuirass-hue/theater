
/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * mx-aspect-frame.h: A container that respect the aspect ratio of its child
 *
 * Copyright 2010, 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef __theater_ASPECT_FRAME_H__
#define __theater_ASPECT_FRAME_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define theater_TYPE_ASPECT_FRAME theater_aspect_frame_get_type()

#define theater_ASPECT_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  theater_TYPE_ASPECT_FRAME, theaterAspectFrame))

#define theater_ASPECT_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  theater_TYPE_ASPECT_FRAME, theaterAspectFrameClass))

#define theater_IS_ASPECT_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  theater_TYPE_ASPECT_FRAME))

#define theater_IS_ASPECT_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  theater_TYPE_ASPECT_FRAME))

#define theater_ASPECT_FRAME_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  theater_TYPE_ASPECT_FRAME, theaterAspectFrameClass))

typedef struct _theaterAspectFrame theaterAspectFrame;
typedef struct _theaterAspectFrameClass theaterAspectFrameClass;
typedef struct _theaterAspectFramePrivate theaterAspectFramePrivate;

struct _theaterAspectFrame
{
  ClutterActor parent;

  theaterAspectFramePrivate *priv;
};

struct _theaterAspectFrameClass
{
  ClutterActorClass parent_class;
};

GType           theater_aspect_frame_get_type     (void) G_GNUC_CONST;

ClutterActor *  theater_aspect_frame_new          (void);

void            theater_aspect_frame_set_child    (theaterAspectFrame *frame,
						 ClutterActor     *child);

void            theater_aspect_frame_set_expand   (theaterAspectFrame *frame,
                                                 gboolean          expand);
gboolean        theater_aspect_frame_get_expand   (theaterAspectFrame *frame);

void            theater_aspect_frame_set_rotation (theaterAspectFrame *frame,
						 gdouble           rotation);
void            theater_aspect_frame_set_internal_rotation
						(theaterAspectFrame *frame,
						 gdouble           rotation);
gdouble         theater_aspect_frame_get_rotation (theaterAspectFrame *frame);

G_END_DECLS

#endif /* __theater_ASPECT_FRAME_H__ */

gst_inc = include_directories('.')

libtheater_gst_helpers_deps = [
  gst_dep,
  gst_tag_dep,
]

libtheater_gst_helpers = static_library(
  'theatergsthelpers',
  sources: 'theater-gst-helpers.c',
  dependencies: libtheater_gst_helpers_deps
)

libtheater_gst_helpers_dep = declare_dependency(
  link_with: libtheater_gst_helpers,
  include_directories: gst_inc,
  dependencies: libtheater_gst_helpers_deps
)

libtheater_gst_pixbuf_helpers_deps = [
  dependency('gdk-pixbuf-2.0'),
  gst_tag_dep,
  gst_video_dep
]

libtheater_gst_pixbuf_helpers = static_library(
  'theatergstpixbufhelpers',
  sources: 'theater-gst-pixbuf-helpers.c',
  dependencies: libtheater_gst_pixbuf_helpers_deps
)

libtheater_gst_pixbuf_helpers_dep = declare_dependency(
  link_with: libtheater_gst_pixbuf_helpers,
  include_directories: gst_inc,
  dependencies: libtheater_gst_pixbuf_helpers_deps
)

libtheater_time_helpers = static_library(
  'theatertimehelpers',
  sources: 'theater-time-helpers.c',
  dependencies: [ glib_dep, m_dep ]
)

libtheater_time_helpers_dep = declare_dependency(
  link_with: libtheater_time_helpers,
  include_directories: gst_inc,
  dependencies: glib_dep
)

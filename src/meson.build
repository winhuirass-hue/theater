src_inc = include_directories('.')
plugins_inc = include_directories('plugins')

subdir('gst')
subdir('backend')

enum_headers = files(
  'backend/bacon-time-label.h',
  'backend/bacon-video-controls-actor.h',
  'backend/bacon-video-spinner-actor.h',
  'backend/bacon-video-widget-gst-missing-plugins.h',
  'backend/bacon-video-widget.h',
  'backend/clock.h',
  'backend/theater-aspect-frame.h',
  'icon-helpers.h',
  'theater-grilo.h',
  'theater-interface.h',
  'theater-main-toolbar.h',
  'theater-menu.h',
  'theater-open-location.h',
  'theater-options.h',
  'theater-playlist.h',
  'theater-preferences.h',
  'theater-private.h',
  'theater-profile.h',
  'theater-resources.h',
  'theater-search-entry.h',
  'theater-selection-toolbar.h',
  'theater-session.h',
  'theater-subtitle-encoding.h',
  'theater-uri.h',
  'theater.h'
)

gnome.mkenums(
  'org.gnome.theater.enums.xml',
  sources: enum_headers,
  comments: '<!-- @comment@ -->',
  fhead: '<schemalist>',
  vhead: '  <@type@ id="org.gnome.theater.@EnumName@">',
  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
  vtail: '  </@type@>',
  ftail: '</schemalist>',
  install_header: true,
  install_dir: theater_schemadir
)

theater_common_incs = [
  top_inc,
  plugins_inc
]

theater_common_deps = libbacon_video_widget_deps + python_deps + [
  glib_dep,
  gio_dep,
  gtk_dep,
  dependency('gmodule-2.0'),
  theater_plparser_dep,
  gir_dep,
  peas_dep,
  peas_gtk_dep,
  dependency('grilo-0.3', version: grilo_req_version),
  dependency('grilo-pls-0.3', version: grilo_req_version),
  dependency('gnome-desktop-3.0')
]

theater_common_cflags = common_flags + warn_flags + [
  '-DDBUS_API_SUBJECT_TO_CHANGE',
  '-DGNOMELOCALEDIR="@0@"'.format(theater_localedir),
  '-DDATADIR="@0@"'.format(theater_datadir),
  '-DLIBEXECDIR="@0@"'.format(theater_libexecdir),
  '-Dtheater_PLUGIN_DIR="@0@"'.format(theater_pluginsdir),
  '-DBINDIR="@0@"'.format(theater_bindir),
  '-DGTKBUILDERDIR=""'
]

headers = files(
  'plugins/theater-plugin.h',
  'plugins/theater-dirs.h',
  'theater-interface.h',
  'theater.h'
)

install_headers(
  headers,
  subdir: theater_api_path
)

libtheater_player_sources = files(
  'theater-interface.c',
)

libtheater_player_deps = theater_common_deps + [
  libtheater_time_helpers_dep
]

libtheater_player = static_library(
  'theater_player',
  sources: libtheater_player_sources,
  include_directories: top_inc,
  dependencies: libtheater_player_deps,
  c_args: theater_common_cflags + [
    '-DG_LOG_DOMAIN="theater"'
  ]
)

libtheater_player_dep = declare_dependency(
  link_with: libtheater_player,
  include_directories: src_inc,
  dependencies: libtheater_player_deps
)

libtheater_sources = files(
  'plugins/theater-dirs.c',
  'plugins/theater-plugins-engine.c',
  'icon-helpers.c',
  'theater-grilo.c',
  'theater-main-toolbar.c',
  'theater-menu.c',
  'theater-object.c',
  'theater-open-location.c',
  'theater-options.c',
  'theater-playlist.c',
  'theater-preferences.c',
  'theater-search-entry.c',
  'theater-selection-toolbar.c',
  'theater-session.c',
  'theater-subtitle-encoding.c',
  'theater-uri.c'
)

libtheater_sources += gnome.compile_resources(
  'griloresources',
  'grilo.gresource.xml',
  export: true,
  source_dir: '.',
  c_name: '_theater'
)

libtheater_deps = [
  libtheater_player_dep,
  libbacon_video_widget_dep,
  libgd_dep
]

libtheater = shared_library(
  'theater',
  sources: libtheater_sources + gen_sources,
  version: libversion,
  soversion: soversion,
  include_directories: theater_common_incs,
  dependencies: libtheater_deps,
  c_args: theater_common_cflags + [
    '-DG_LOG_DOMAIN="theater"'
  ],
  install: true,
  install_dir: theater_libdir
)

libtheater_dep = declare_dependency(
  link_with: libtheater,
  include_directories: src_inc,
  dependencies: libtheater_deps
)

pkg.generate(
  libraries: libtheater,
  version: theater_version,
  name: 'theater',
  description: 'theater Movie Player plugin API',
  filebase: meson.project_name(),
  subdirs: theater_api_path,
  requires: [
    'gio-2.0 ' + glib_req_version,
    'gtk+-3.0 ' + gtk_req_version,
    'theater-plparser ' + theater_plparser_req_version
  ],
  variables: [
    'exec_prefix=' + theater_libexecdir,
    'pluginsdir=' + theater_pluginsdir
  ],
  install_dir: join_paths(theater_libdir, 'pkgconfig')
)

theater = executable(
  meson.project_name(),
  'theater.c',
  include_directories: theater_common_incs,
  dependencies: libtheater_dep,
  c_args: theater_common_cflags + [
    '-DG_LOG_DOMAIN="theater"'
  ],
  install: true,
  install_dir: theater_bindir
)

theater_video_thumbnailer_sources = files(
  'theater-resources.c',
  'theater-video-thumbnailer.c'
)

theater_video_thumbnailer_deps = [
  theater_plparser_dep,
  gst_tag_dep,
  gst_video_dep,
  m_dep,
  libtheater_gst_helpers_dep,
  libtheater_gst_pixbuf_helpers_dep,
]

executable(
  'theater-video-thumbnailer',
  theater_video_thumbnailer_sources,
  include_directories: top_inc,
  dependencies: theater_video_thumbnailer_deps,
  c_args: theater_common_cflags + [
    '-DG_LOG_DOMAIN="theaterVideoThumbnailer"'
  ],
  install: true,
  install_dir: theater_bindir
)

theater_gallery_thumbnailer_sources = files(
  'theater-gallery-thumbnailer.c'
)

theater_gallery_thumbnailer_deps = [
  dependency('gdk-3.0'),
  theater_plparser_dep,
  gst_tag_dep,
  gst_video_dep,
  m_dep,
  libtheater_gst_helpers_dep,
  libtheater_gst_pixbuf_helpers_dep,
  libtheater_time_helpers_dep
]

executable(
  'theater-gallery-thumbnailer',
  theater_gallery_thumbnailer_sources,
  include_directories: top_inc,
  dependencies: theater_gallery_thumbnailer_deps,
  c_args: theater_common_cflags + [
    '-DG_LOG_DOMAIN="theaterGalleryThumbnailer"'
  ],
  install: true,
  install_dir: theater_libexecdir
)

test_icons_sources = files(
  'icon-helpers.c',
  'test-icons.c'
)

executable(
  'test-icons',
  test_icons_sources,
  dependencies: [
    libtheater_player_dep,
    libbacon_video_widget_dep
  ],
  c_args: theater_common_cflags
)

gir_sources = libtheater_sources + libtheater_player_sources + headers

gir_incs = [
  'Gio-2.0',
  'GLib-2.0',
  'GObject-2.0',
  'Gtk-3.0',
  'theaterPlParser-1.0'
]

gir_extra_args = [
  '--c-include=theater.h',
  '--pkg-export=theater'
]

gir_dir = join_paths(theater_datadir, 'gir-' + theater_api_version)
typelib_dir = join_paths(theater_libdir, 'girepository-' + theater_api_version)

libtheater_gir = gnome.generate_gir(
  libtheater,
  sources: gir_sources,
  nsversion: theater_api_version,
  namespace: theater_gir_ns,
  symbol_prefix: theater_gir_ns.to_lower(),
  identifier_prefix: theater_gir_ns,
  includes: gir_incs,
  include_directories: src_inc,
  install: true,
  install_dir_gir: gir_dir,
  install_dir_typelib: typelib_dir,
  extra_args: gir_extra_args
)

tests = [ 'test-theater' ]

foreach test_name : tests
  exe = executable(test_name, '@0@.c'.format(test_name),
                   include_directories: [src_inc, top_inc],
                   dependencies: libtheater_dep)

  test(test_name, exe)
endforeach

subdir('plugins')

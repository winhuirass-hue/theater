doc_module = meson.project_name()

private_headers = [
  'bacon-resize.h',
  'bacon-time-label.h',
  'bacon-video-controls-actor.h',
  'bacon-video-spinner-actor.h',
  'bacon-video-widget-common.h',
  'bacon-video-widget-gst-missing-plugins.h',
  'bacon-video-widget-properties.h',
  'clock.h',
  'debug.h',
  'gd-fullscreen-filter.h',
  'gd-tagged-entry.h',
  'gnome-screenshot-widget.h',
  'gsd-media-keys-window.h',
  'gstscreenshot.h',
  'icon-helpers.h',
  'screenshot-filename-builder.h',
  'theater-aspect-frame.h',
  'theater-dnd-menu.h',
  'theater-dvb-setup.h',
  'theater-edit-chapter.h',
  'theater-fullscreen.h',
  'theater-gallery-progress.h',
  'theater-gallery.h',
  'theater-grilo.h',
  'theater-gst-helpers.h',
  'theater-gst-pixbuf-helpers.h',
  'theater-main-toolbar.h',
  'theater-menu.h',
  'theater-module.h',
  'theater-open-location.h',
  'theater-options.h',
  'theater-playlist.h',
  'theater-plugins-engine.h',
  'theater-plugin-manager.h',
  'theater-preferences.h',
  'theater-private.h',
  'theater-profile.h',
  'theater-python-plugin.h',
  'theater-resources.h',
  'theater-screenshot-plugin.h',
  'theater-screenshot.h',
  'theater-search-entry.h',
  'theater-selection-toolbar.h',
  'theater-session.h',
  'theater-skipto-plugin.h',
  'theater-skipto.h',
  'theater-statusbar.h',
  'theater-subtitle-encoding.h',
  'theater-time-entry.h',
  'theater-time-helpers.h',
  'theater-time-label.h',
  'theater-tracker-widget.h',
  'theater-uri.h',
  'video-utils.h'
]

version_conf = configuration_data()
version_conf.set('theater_VERSION_MAJOR', theater_major_version)
version_conf.set('theater_VERSION_MINOR', theater_minor_version)
version_conf.set('theater_VERSION_MICRO', theater_micro_version)

version_xml = 'version.xml'

configure_file(
  input: version_xml + '.in',
  output: version_xml,
  configuration: version_conf
)

doc_path = join_paths(theater_prefix, gnome.gtkdoc_html_dir(doc_module))

gnome.gtkdoc(
  doc_module,
  main_xml: doc_module + '-docs.xml',
  src_dir: join_paths(meson.source_root(), 'src'),
  dependencies: libtheater_dep,
  scan_args: '--ignore-headers=' + ' '.join(private_headers),
  mkdb_args: '--name-space=' + doc_module,
  fixxref_args: '--html-dir=' + doc_path,
  install: true,
  install_dir: doc_path
)

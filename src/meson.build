headers = [
  'colpicker-application.h',
  'colpicker-color-item.h',
  'colpicker-color-row.h',
  'colpicker-color-selection.h',
  'colpicker-color-store.h',
  'colpicker-hsv.h',
  'colpicker-window.h',
  'utils.h'
]

sources = [
  'colpicker-application.c',
  'colpicker-color-item.c',
  'colpicker-color-row.c',
  'colpicker-color-selection.c',
  'colpicker-color-store.c',
  'colpicker-hsv.c',
  'colpicker-window.c',
  'main.c',
  'utils.c'
]

sources += gnome.compile_resources(
  'colpicker-resources', join_paths(meson.source_root(), 'data', 'colpicker.gresource.xml'),
  source_dir: join_paths(meson.source_root(), 'data'),
  c_name: 'colpicker'
)

sources += gnome.genmarshal(
  'colpicker-marshalers',
  sources: join_paths(meson.source_root(), 'data', 'colpicker-marshalers.list'),
  prefix: '_colpicker_marshal'
)

add_project_arguments('-DGDK_DISABLE_DEPRECATED', language : 'c')
add_project_arguments('-DGTK_DISABLE_DEPRECATED', language : 'c')

executable(
  meson.project_name(),
  sources + headers,
  include_directories: [
    include_directories('../data', '..')
  ],
  dependencies: [
    dep_gtk,
    dep_libportal,
    dep_lm
  ],
  install: true,
  install_dir: bindir
)

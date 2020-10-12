install_data('org.gnome.design.ColorPicker.svg',
  install_dir: join_paths(datadir, 'icons', 'hicolor', 'scalable', 'apps'))
install_data('org.gnome.design.ColorPicker-symbolic.svg',
  install_dir: join_paths(datadir, 'icons', 'hicolor', 'symbolic', 'apps'))
  
install_man('gnome-color-picker.1')

desktop_file = i18n.merge_file(
  input: 'org.gnome.design.ColorPicker.desktop.in',
  output: 'org.gnome.design.ColorPicker.desktop',
  type: 'desktop',
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(datadir, 'applications')
)
desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [
      desktop_file.full_path()
    ]
  )
endif

appdata_file = i18n.merge_file(
  input: 'org.gnome.design.ColorPicker.appdata.xml.in',
  output: 'org.gnome.design.ColorPicker.appdata.xml',
  type: 'xml',
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(datadir, 'metainfo')
)
appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test(
    'validate-appdata', appstream_util,
    args: [
      'validate-relax', '--nonet', appdata_file.full_path()
    ]
  )
endif

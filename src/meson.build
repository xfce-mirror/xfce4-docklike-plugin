plugin_sources = [
  'AppInfos.cpp',
  'AppInfos.hpp',
  'Dock.cpp',
  'Dock.hpp',
  'Group.cpp',
  'Group.hpp',
  'GroupMenu.cpp',
  'GroupMenu.hpp',
  'GroupMenuItem.cpp',
  'GroupMenuItem.hpp',
  'GroupWindow.cpp',
  'GroupWindow.hpp',
  'Helpers.cpp',
  'Helpers.hpp',
  'Plugin.cpp',
  'Plugin.hpp',
  'register.c',
  'Settings.cpp',
  'Settings.hpp',
  'SettingsDialog.cpp',
  'SettingsDialog.hpp',
  'State.ipp',
  'Store.ipp',
  'Theme.cpp',
  'Theme.hpp',
  'Xfw.cpp',
  'Xfw.hpp',
  xfce_revision_h,
]

if enable_x11
  plugin_sources += [
    'Hotkeys.cpp',
    'Hotkeys.hpp',
  ]
endif

plugin_sources += gnome.compile_resources(
  'docklike-plugin-resources',
  '_gresources.xml',
)

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_library(
  'docklike',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-docklike-plugin'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    cario,
    gio_unix,
    glib,
    gtk,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
    libxfce4windowing,
    libxfce4windowingui,
    wayland_deps,
    x11_deps,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'docklike.desktop.in',
  output: 'docklike.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)

project(
  'xfce4-docklike-plugin',
  'c',
  'cpp',
version : '0.5.0-dev',
  license : 'GPL-3.0-or-later',
  meson_version : '>= 0.54.0',
  default_options : ['c_std=gnu11', 'cpp_std=gnu++11', 'buildtype=debugoptimized', 'warning_level=2']
)

project_namespace = 'panel-plugins'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
copyright_year = '2025'

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'glib': '>= 2.58.0',
  'gtk': '>= 3.24.0',
  'cairo': '>= 1.16.0',
  'xfce4': '>= 4.16.0',
  'libxfce4windowing': '>= 4.19.4',
  'x11': '>= 1.6.7',
  'xi': '>= 1.2.0',
  'gtk-layer-shell': '>= 0.7.0',
}

gio_unix = dependency('gio-unix-2.0', version: dependency_versions['glib'])
glib = dependency('glib-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
cairo = dependency('cairo', version: dependency_versions['cairo'])
libxfce4panel = dependency('libxfce4panel-2.0', version: dependency_versions['xfce4'])
libxfce4ui = dependency('libxfce4ui-2', version: dependency_versions['xfce4'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['xfce4'])
libxfce4windowing = dependency('libxfce4windowing-0', version: dependency_versions['libxfce4windowing'])
libxfce4windowingui = dependency('libxfce4windowingui-0', version: dependency_versions['libxfce4windowing'])

# Feature: 'x11'
x11_deps = []
x11_deps += dependency('gdk-x11-3.0', version: dependency_versions['gtk'], required: get_option('x11'))
x11_deps += dependency('x11', version: dependency_versions['x11'], required: get_option('x11'))
x11_deps += dependency('xi', version: dependency_versions['xi'], required: get_option('x11'))
x11_deps += dependency('libxfce4windowing-x11-0', version: dependency_versions['libxfce4windowing'], required: get_option('x11'))

# Feature: 'wayland'
wayland_deps = []
wayland_deps += dependency('gdk-wayland-3.0', version: dependency_versions['gtk'], required: get_option('wayland'))
wayland_deps += dependency('gtk-layer-shell-0', version: dependency_versions['gtk-layer-shell'], required: get_option('wayland'))

feature_cflags = []

enable_x11 = not get_option('x11').disabled()
foreach dep : x11_deps
  enable_x11 = enable_x11 and dep.found()
endforeach
if enable_x11
  feature_cflags += '-DENABLE_X11=1'
endif

enable_wayland = not get_option('wayland').disabled()
foreach dep : wayland_deps
  enable_wayland = enable_wayland and dep.found()
endforeach
if enable_wayland
  feature_cflags += '-DENABLE_WAYLAND=1'
endif

if not enable_x11 and not enable_wayland
  error('Either both X11 and Wayland support was disabled, or required dependencies are missing. One of the two must be enabled.')
endif

extra_cflags = []
extra_cxxflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wno-unused-parameter',
  '-Wno-missing-field-initializers',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cxxflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

extra_cflags_check = extra_cxxflags_check + [
  '-Wold-style-definition',
  '-Wnested-externs',
  '-Wno-declaration-after-statement',
  '-Werror=implicit-function-declaration'
]

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DCOPYRIGHT_YEAR="@0@"'.format(copyright_year),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(cxx.get_supported_arguments(extra_cxxflags_check), language: 'cpp')
add_project_arguments(extra_cflags, language: ['c', 'cpp'])
add_project_arguments(feature_cflags, language: ['c', 'cpp'])

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

subdir('src')
subdir('po')

AM_CXXFLAGS = \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DG_LOG_DOMAIN=\"docklike\"

plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libdocklike.la

libdocklike_la_SOURCES =							\
		Theme.cpp Theme.hpp							\
		AppInfos.cpp AppInfos.hpp					\
		Settings.cpp Settings.hpp					\
		Group.cpp Group.hpp							\
		GroupMenu.cpp GroupMenu.hpp					\
		GroupMenuItem.cpp GroupMenuItem.hpp			\
		GroupWindow.cpp GroupWindow.hpp				\
		Helpers.cpp Helpers.hpp						\
		Plugin.cpp Plugin.hpp						\
		Dock.cpp Dock.hpp							\
		Xfw.cpp Xfw.hpp							\
		SettingsDialog.cpp SettingsDialog.hpp		\
		State.tpp 									\
		Store.tpp									\
		register.c									\
		_gresources.xml _dialogs.xml				\
		_gresources.c

if ENABLE_X11
libdocklike_la_SOURCES += \
	Hotkeys.cpp Hotkeys.hpp
endif

_gresources.c: _gresources.xml _dialogs.xml
	glib-compile-resources --generate-source --target _gresources.c _gresources.xml

libdocklike_la_CPPFLAGS = \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(LIBXFCE4WINDOWING_CFLAGS) \
	$(LIBXFCE4WINDOWINGUI_CFLAGS) \
	$(GTK_LAYER_SHELL_CFLAGS)

libdocklike_la_LDFLAGS = -avoid-version -module -no-undefined -export-symbols-regex '^xfce_panel_module_(preinit|init|construct)'

libdocklike_la_LIBADD = \
	$(LIBXFCE4PANEL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(GIO_UNIX_LIBS) \
	$(GTK_LIBS) \
	$(CAIRO_LIBS) \
	$(LIBXFCE4WINDOWING_LIBS) \
	$(LIBXFCE4WINDOWINGUI_LIBS) \
	$(GTK_LAYER_SHELL_LIBS)

if ENABLE_X11
libdocklike_la_CPPFLAGS += \
	$(LIBX11_CFLAGS)

libdocklike_la_LIBADD += \
	$(LIBX11_LIBS)
endif

pluginsdir = $(datadir)/xfce4/panel/plugins
plugins_in_files = docklike.desktop.in
plugins_DATA = $(plugins_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(plugins_in_files)

CLEANFILES = $(NULL)
DISTCLEANFILES = docklike.desktop

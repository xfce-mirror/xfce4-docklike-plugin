/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include "AppInfos.hpp"
#include "Dock.hpp"
#include "Settings.hpp"
#include "SettingsDialog.hpp"
#include "Theme.hpp"

#include <gtk/gtk.h>
#include <libxfce4windowing/libxfce4windowing.h>

#include <iostream>
#include <string>
#include <vector>

extern "C"
{
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
	void construct(XfcePanelPlugin* xfPlugin);
}

#define HELP_WEBSITE "https://docs.xfce.org/panel-plugins/xfce4-docklike-plugin/start"

namespace Plugin
{
	extern XfcePanelPlugin* mXfPlugin;
	extern GdkDevice* mPointer;
	extern GdkDisplay* mDisplay;

	void aboutDialog();
	void remoteEvent(gchar* name, GValue* value);
} // namespace Plugin

#endif // PLUGIN_HPP

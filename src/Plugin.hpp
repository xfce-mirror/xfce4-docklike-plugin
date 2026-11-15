/*
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

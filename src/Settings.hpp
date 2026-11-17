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

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "Dock.hpp"
#include "LauncherEntry.hpp"
#include "State.ipp"
#include "Store.ipp"
#include "Theme.hpp"

#include <glib.h>
#include <libxfce4panel/libxfce4panel.h>

class Dock;
class LauncherEntry;
class Theme;

class Settings
{
public:
	explicit Settings(XfcePanelPlugin* xfPlugin);
	void setDock(Dock* dock) { mDock = dock; };
	void setLauncherEntry(LauncherEntry* launcherEntry) { mLauncherEntry = launcherEntry; };
	void setTheme(Theme* theme) { mTheme = theme; };

	const int minIconSize = 16;
	const int maxIconSize = 128;
	const int defPreviewWidth = 288;
	const int defPreviewHeight = 162;

	State<bool> forceIconSize;
	State<int> iconSize;

	State<bool> noWindowsListIfSingle;
	State<bool> onlyDisplayVisible;
	State<bool> onlyDisplayScreen;
	State<bool> showPreviews;
	State<int> previewWidth;
	State<int> previewHeight;
	State<bool> showWindowCount;
	State<bool> disableLauncherCounts;
	State<int> middleButtonBehavior;

	State<int> indicatorOrientation;
	State<int> indicatorStyle;
	State<int> inactiveIndicatorStyle;
	State<bool> indicatorColorFromTheme;
	State<std::shared_ptr<GdkRGBA>> indicatorColor;
	State<std::shared_ptr<GdkRGBA>> inactiveColor;

	State<bool> keyComboActive;
	State<bool> keyComboMinimize;
	State<bool> keyAloneActive;

	State<std::list<std::string>> pinnedAppList;
	State<std::pair<std::list<std::string>, std::list<std::string>>> userSetApps;

	// HIDDEN SETTINGS:
	State<int> dockSize;
	State<int> previewSleep;

private:
	void saveFile();

	Dock* mDock = nullptr;
	LauncherEntry* mLauncherEntry = nullptr;
	Theme* mTheme = nullptr;
	Store::AutoPtr<gchar> mPath;
	Store::AutoPtr<GKeyFile> mFile;
};

#endif // SETTINGS_HPP

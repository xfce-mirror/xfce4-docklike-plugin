/*
 * Docklike Taskbar - A modern, minimalist taskbar for XFCE
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <glib.h>

#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "Dock.hpp"
#include "Helpers.hpp"
#include "Hotkeys.hpp"
#include "Plugin.hpp"
#include "State.tpp"

namespace Settings
{
	void init();

	void saveFile();

	extern std::string mPath;
	extern GKeyFile* mFile;

	extern State<bool> forceIconSize;
	extern State<int> iconSize;

	extern State<bool> noWindowsListIfSingle;
	extern State<bool> onlyDisplayVisible;
	extern State<bool> onlyDisplayScreen;
	extern State<bool> showPreviews;

	extern State<int> indicatorOrientation;
	extern State<int> indicatorStyle;
	extern State<GdkRGBA*> indicatorColor;
	extern State<GdkRGBA*> inactiveColor;

	extern State<bool> keyComboActive;
	extern State<bool> keyAloneActive;

	extern State<std::list<std::string>> pinnedAppList;

	// HIDDEN SETTINGS:
	extern State<bool> showWindowCount;
	extern State<int> dockSize;
}; // namespace Settings

#endif

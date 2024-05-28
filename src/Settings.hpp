/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "Dock.hpp"
#include "Helpers.hpp"
#include "Plugin.hpp"
#include "State.tpp"

#include <glib.h>

#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace Settings
{
	void init();
	void finalize();

	void saveFile();

	const int minIconSize = 16;
	const int maxIconSize = 128;

	extern State<bool> forceIconSize;
	extern State<int> iconSize;

	extern State<bool> noWindowsListIfSingle;
	extern State<bool> onlyDisplayVisible;
	extern State<bool> onlyDisplayScreen;
	extern State<bool> showPreviews;
	extern State<bool> showWindowCount;
	extern State<int> middleButtonBehavior;

	extern State<int> indicatorOrientation;
	extern State<int> indicatorStyle;
	extern State<int> inactiveIndicatorStyle;
	extern State<bool> indicatorColorFromTheme;
	extern State<std::shared_ptr<GdkRGBA>> indicatorColor;
	extern State<std::shared_ptr<GdkRGBA>> inactiveColor;

	extern State<bool> keyComboActive;
	extern State<bool> keyAloneActive;

	extern State<std::list<std::string>> pinnedAppList;

	// HIDDEN SETTINGS:
	extern State<int> dockSize;
	extern State<double> previewScale;
	extern State<int> previewSleep;
}; // namespace Settings

#endif // SETTINGS_HPP

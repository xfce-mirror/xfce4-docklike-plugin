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
#include "Helpers.hpp"
#include "Plugin.hpp"
#include "State.ipp"

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
	extern State<std::pair<std::list<std::string>, std::list<std::string>>> userSetApps;

	// HIDDEN SETTINGS:
	extern State<int> dockSize;
	extern State<double> previewScale;
	extern State<int> previewSleep;
}; // namespace Settings

#endif // SETTINGS_HPP

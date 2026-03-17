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

#ifndef HOTKEYS_HPP
#define HOTKEYS_HPP

#include <string>
#include <vector>

#include <gtk/gtk.h>

namespace Hotkeys
{
	void init();
	void updateSettings();
	void updateCustomKeys();

	// Capture a key combination interactively; returns accel string or "".
	std::string captureKey(GtkWindow* parent);

	// Convert a GTK accel string (e.g. "<Mod4>a") to a human-readable label
	// (e.g. "Super+A"), replacing raw modifier names with friendly ones.
	std::string accelToReadableLabel(const std::string& accel);

	extern bool mXIExtAvailable;
	extern int mGrabbedKeys;

	const int NbHotkeys = 10;
} // namespace Hotkeys

#endif // HOTKEYS_HPP

/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef HOTKEYS_HPP
#define HOTKEYS_HPP

namespace Hotkeys
{
	void init();
	void updateSettings();

	extern bool mXIExtAvailable;
	extern int mGrabbedKeys;

	const int NbHotkeys = 10;
} // namespace Hotkeys

#endif // HOTKEYS_HPP

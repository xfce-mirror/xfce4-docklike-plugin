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

#ifndef DOCK_HPP
#define DOCK_HPP

#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "Plugin.hpp"
#include "Settings.hpp"
#include "Store.ipp"
#include "Xfw.hpp"

#include <gtk/gtk.h>
#include <libxfce4windowing/libxfce4windowing.h>

#include <iostream>
#include <string>

class Group;

namespace Dock
{
	void init();

	Group* prepareGroup(std::shared_ptr<AppInfo> appInfo);

	void moveButton(Group* moving, Group* dest);
	void savePinned();
	void drawGroups();

	void hoverSupered(bool on);
	void activateGroup(int nb, guint32 timestamp);

	void onPanelResize(int size = -1);
	void onPanelOrientationChange(GtkOrientation orientation);

	extern GtkWidget* mBox;
	extern Store::KeyStore<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> mGroups;

	extern int mPanelSize;
	extern int mIconSize;
} // namespace Dock

#endif // DOCK_HPP

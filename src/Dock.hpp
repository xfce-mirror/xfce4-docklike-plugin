/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef DOCK_HPP
#define DOCK_HPP

#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "Plugin.hpp"
#include "Settings.hpp"
#include "Store.tpp"
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

/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef DOCK_HPP
#define DOCK_HPP

#include <gtk/gtk.h>
#include <libwnck/libwnck.h>

#include <iostream>
#include <string>

#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "Plugin.hpp"
#include "Settings.hpp"
#include "Store.tpp"
#include "Wnck.hpp"

class Group;

namespace Dock
{
	void init();

	Group* prepareGroup(AppInfo* appInfo);

	void moveButton(Group* moving, Group* dest);
	void savePinned();
	void drawGroups();

	void hoverSupered(bool on);
	void activateGroup(int nb, guint32 timestamp);

	void onPanelResize(int size = -1);
	void onPanelOrientationChange(GtkOrientation orientation);

	extern GtkWidget* mBox;
	extern Store::KeyStore<AppInfo*, Group*> mGroups;
	extern Help::Gtk::Timeout mDrawTimeout;

	extern int mPanelSize;
	extern int mIconSize;
} // namespace Dock

#endif // DOCK_HPP

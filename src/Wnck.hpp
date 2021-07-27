/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef WNCK_HPP
#define WNCK_HPP

#include <fcntl.h>
#include <gio/gdesktopappinfo.h>
#include <libwnck/libwnck.h>

#include <map>

#include "Group.hpp"
#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "Store.tpp"

class GroupWindow;

namespace Wnck
{
	void init();

	gulong getActiveWindowXID();
	std::string getGroupName(GroupWindow* groupWindow);
	GtkWidget* buildActionMenu(GroupWindow* groupWindow, Group* group);

	void close(GroupWindow* groupWindow, guint32 timestamp);
	void activate(GroupWindow* groupWindow, guint32 timestamp);

	void switchToLastWindow(guint32 timestamp);

	void setActiveWindow();
	void setVisibleGroups();

	extern WnckScreen* mWnckScreen;
	extern Store::KeyStore<gulong, GroupWindow*> mGroupWindows;
} // namespace Wnck

#endif //WNCK_HPP

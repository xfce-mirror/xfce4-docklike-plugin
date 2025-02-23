/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef XFW_HPP
#define XFW_HPP

#include "Group.hpp"
#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "Store.ipp"

#include <fcntl.h>
#include <gio/gdesktopappinfo.h>
#include <libxfce4windowing/libxfce4windowing.h>
#include <libxfce4windowingui/libxfce4windowingui.h>

#include <map>

class GroupWindow;

namespace Xfw
{
	void init();
	void finalize();

	XfwWindow* getActiveWindow();
	std::string getGroupName(GroupWindow* groupWindow);
	GtkWidget* buildActionMenu(GroupWindow* groupWindow, Group* group);

	void close(GroupWindow* groupWindow, guint32 timestamp);
	void activate(GroupWindow* groupWindow, guint32 timestamp);

	void switchToLastWindow(guint32 timestamp);

	void setActiveWindow();
	void setVisibleGroups();

	extern XfwScreen* mXfwScreen;
	extern XfwWorkspaceGroup* mXfwWorkspaceGroup;
	extern Store::KeyStore<XfwWindow*, std::shared_ptr<GroupWindow>> mGroupWindows;
} // namespace Xfw

#endif // XFW_HPP

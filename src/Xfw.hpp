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

	void close(GroupWindow* groupWindow, guint32 timestamp);
	void activate(GroupWindow* groupWindow, guint32 timestamp);

	void switchToLastWindow(guint32 timestamp);

	void setActiveWindow(XfwWindow* previousActiveWindow = nullptr);
	void setVisibleGroups();

	extern XfwScreen* mXfwScreen;
	extern XfwWorkspaceGroup* mXfwWorkspaceGroup;
	extern Store::KeyStore<XfwWindow*, std::shared_ptr<GroupWindow>> mGroupWindows;
} // namespace Xfw

#endif // XFW_HPP

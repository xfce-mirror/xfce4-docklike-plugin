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

#include "AppInfos.hpp"
#include "Dock.hpp"
#include "GroupWindow.hpp"
#include "Settings.hpp"
#include "Store.ipp"

#include <libxfce4windowing/libxfce4windowing.h>
#include <libxfce4windowingui/libxfce4windowingui.h>

struct AppInfo;
class AppInfos;
class GroupWindow;

class Xfw
{
public:
	Xfw(Settings* settings, AppInfos* appInfos);
	~Xfw();
	void setDock(Dock* dock) { mDock = dock; };

	XfwWindow* getActiveWindow();
	static std::string getGroupName(GroupWindow* groupWindow);

	static void close(GroupWindow* groupWindow, guint32 timestamp);
	static void activate(GroupWindow* groupWindow, guint32 timestamp);

	void switchToLastWindow();

	void setActiveWindow(XfwWindow* previousActiveWindow = nullptr);
	void setVisibleGroups();

	XfwScreen* mXfwScreen;
	XfwWorkspaceGroup* mXfwWorkspaceGroup = nullptr;
	Store::KeyStore<XfwWindow*, std::shared_ptr<GroupWindow>> mGroupWindows;

private:
	static std::string getGroupNameSys(XfwWindow* xfwWindow);

	Settings* mSettings;
	AppInfos* mAppInfos;
	Dock* mDock;
	XfwWindow* mPreviousActiveWindow = nullptr;
};

#endif // XFW_HPP

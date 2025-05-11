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

#ifndef GROUP_WINDOW_HPP
#define GROUP_WINDOW_HPP

#include "AppInfos.hpp"
#include "Dock.hpp"
#include "Group.hpp"
#include "GroupMenuItem.hpp"
#include "Helpers.hpp"
#include "Plugin.hpp"
#include "Xfw.hpp"

#include <gtk/gtk.h>
#include <libxfce4windowing/libxfce4windowing.h>

#include <iostream>

class GroupMenuItem;
class Group;

class GroupWindow
{
public:
	GroupWindow(XfwWindow* xfwWindow);
	~GroupWindow();

	void updateState();
	void getInGroup();
	void leaveGroup();
	void onActivate();
	void onUnactivate() const;
	void minimize();
	void activate(guint32 timestamp);
	bool getState(XfwWindowState flagMask) const;

	Group* mGroup;
	GroupMenuItem* mGroupMenuItem;

	XfwWindow* mXfwWindow;

	unsigned short mState{};
	bool mGroupAssociated;
};

#endif // GROUP_WINDOW_HPP

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

#ifndef GROUP_MENU_HPP
#define GROUP_MENU_HPP

#include "Helpers.hpp"

#include <gtk/gtk.h>

#include <iostream>

class Group;
class GroupMenuItem;

class GroupMenu
{
public:
	GroupMenu(Group* dockButton);
	~GroupMenu();

	void add(GroupMenuItem* menuItem);
	void remove(GroupMenuItem* menuItem);

	void popup();
	void updateOrientation();
	void updatePosition(gint wx, gint wy);
	void hide();
	void showPreviewsChanged();

	uint getPointerDistance();

	Group* mGroup;

	GtkWidget* mWindow;
	GtkWidget* mBox;

	bool mVisible;
	bool mMouseHover;

	Help::Gtk::Idle mPopupIdle;
};

#endif // GROUP_MENU_HPP

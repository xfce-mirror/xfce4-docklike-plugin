/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef GROUP_MENU_HPP
#define GROUP_MENU_HPP

#include <gtk/gtk.h>

#include <iostream>

class Group;
class GroupMenuItem;

class GroupMenu
{
  public:
	GroupMenu(Group* dockButton);

	void add(GroupMenuItem* menuItem);
	void remove(GroupMenuItem* menuItem);

	void popup();
	void hide();

	uint getPointerDistance();

	Group* mGroup;

	GtkWidget* mWindow;
	GtkWidget* mBox;

	bool mVisible;
	bool mMouseHover;
};

#endif // GROUP_MENU_HPP

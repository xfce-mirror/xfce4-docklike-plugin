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

#include "Helpers.hpp"

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

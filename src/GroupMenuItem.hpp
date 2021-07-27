/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef GROUP_MENU_ITEM_HPP
#define GROUP_MENU_ITEM_HPP

#include <gtk/gtk.h>

#include <iostream>

#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "Wnck.hpp"

class GroupWindow;

class GroupMenuItem
{
  public:
	GroupMenuItem(GroupWindow* groupWindow);
	~GroupMenuItem();

	void updateLabel();
	void updateIcon();
	void updatePreview();

	GroupWindow* mGroupWindow;

	GtkEventBox* mItem;
	GtkGrid* mGrid;
	GtkImage* mIcon;
	GtkLabel* mLabel;
	GtkButton* mCloseButton;
	GtkImage* mPreview;

	Help::Gtk::Timeout mDragSwitchTimeout;
	Help::Gtk::Timeout mPreviewTimeout;
};

#endif // GROUP_MENU_ITEM_HPP

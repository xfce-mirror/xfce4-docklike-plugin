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

#ifndef GROUP_MENU_ITEM_HPP
#define GROUP_MENU_ITEM_HPP

#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "Xfw.hpp"

#include <gtk/gtk.h>

#include <iostream>

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

	Help::Gtk::Timeout mPreviewTimeout;
};

#endif // GROUP_MENU_ITEM_HPP

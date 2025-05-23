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

#include "Dock.hpp"
#ifdef ENABLE_X11
#include "Hotkeys.hpp"
#endif

namespace Dock
{
	GtkWidget* mBox;
	Store::KeyStore<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> mGroups;

	int mPanelSize;
	int mIconSize;

	void init()
	{
		mBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_name(mBox, "docklike-plugin");

		if (Settings::dockSize)
			gtk_widget_set_size_request(mBox, Settings::dockSize, -1);

		gtk_widget_show(mBox);
		drawGroups();
	}

	Group* prepareGroup(std::shared_ptr<AppInfo> appInfo)
	{
		std::shared_ptr<Group> group = mGroups.get(appInfo);

		if (!group)
		{
			group = std::make_shared<Group>(appInfo, false);
			mGroups.push(appInfo, group);
			gtk_container_add(GTK_CONTAINER(mBox), group->mButton);
		}

		return group.get();
	}

	void moveButton(Group* moving, Group* dest)
	{
		int startpos = Help::Gtk::getChildPosition(GTK_CONTAINER(mBox), moving->mButton);
		int destpos = Help::Gtk::getChildPosition(GTK_CONTAINER(mBox), dest->mButton);

		if (startpos == destpos)
			return;

		gtk_box_reorder_child(GTK_BOX(mBox), moving->mButton, destpos);

		savePinned();
	}

	void savePinned()
	{
		std::list<std::string> pinnedList;
		GList* children = gtk_container_get_children(GTK_CONTAINER(mBox));

		for (GList* child = children; child != nullptr; child = child->next)
		{
			GtkWidget* widget = (GtkWidget*)child->data;
			Group* group = (Group*)g_object_get_data(G_OBJECT(widget), "group");

			if (group->mPinned && g_file_test(group->mAppInfo->mPath.c_str(), G_FILE_TEST_IS_REGULAR))
				pinnedList.push_back(group->mAppInfo->mId);
		}

		Settings::pinnedAppList.set(pinnedList);
		g_list_free(children);
	}

	void drawGroups()
	{
		// Remove old groups
		Xfw::mGroupWindows.clear();
		mGroups.clear();

		// Add pinned groups
		std::list<std::string> pinnedApps = Settings::pinnedAppList;
		std::list<std::string>::iterator it = pinnedApps.begin();

		while (it != pinnedApps.end())
		{
			std::shared_ptr<AppInfo> appInfo = AppInfos::search(Help::String::toLowercase(*it));
			std::shared_ptr<Group> group = std::make_shared<Group>(appInfo, true);

			mGroups.push(appInfo, group);
			gtk_container_add(GTK_CONTAINER(mBox), group->mButton);
			++it;
		}

		// Add open windows
		for (GList* window_l = xfw_screen_get_windows(Xfw::mXfwScreen);
			 window_l != nullptr;
			 window_l = window_l->next)
		{
			XfwWindow* xfwWindow = XFW_WINDOW(window_l->data);
			std::shared_ptr<GroupWindow> groupWindow = Xfw::mGroupWindows.get(xfwWindow);

			if (!groupWindow)
				groupWindow = std::make_shared<GroupWindow>(xfwWindow);
			else
				gtk_container_add(GTK_CONTAINER(mBox), groupWindow->mGroup->mButton);

			Xfw::mGroupWindows.push(xfwWindow, groupWindow);
			groupWindow->updateState();
		}

		gtk_widget_queue_draw(mBox);
	}

	void hoverSupered(bool on)
	{
#ifdef ENABLE_X11
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
		{
			int grabbedKeys = Hotkeys::mGrabbedKeys;
			GList* children = gtk_container_get_children(GTK_CONTAINER(mBox));

			for (GList* child = children; child != nullptr && grabbedKeys; child = child->next)
			{
				GtkWidget* widget = (GtkWidget*)child->data;

				if (!gtk_widget_get_visible(widget))
					continue;

				--grabbedKeys;
			}

			g_list_free(children);
		}
#endif
	}

	void activateGroup(int nb, guint32 timestamp)
	{
		int i = 0;
		GList* children = gtk_container_get_children(GTK_CONTAINER(mBox));

		for (GList* child = children; child != nullptr; child = child->next)
		{
			GtkWidget* widget = (GtkWidget*)child->data;

			if (gtk_widget_get_visible(widget))
			{
				if (i == nb)
				{
					Group* group = (Group*)g_object_get_data(G_OBJECT(widget), "group");

					if (group->mActive)
						group->scrollWindows(timestamp, GDK_SCROLL_DOWN);
					else if (group->mWindowsCount > 0)
						group->activate(timestamp);
					else
						group->mAppInfo->launch();

					return;
				}
				else
					++i;
			}
		}

		g_list_free(children);
	}

	void onPanelResize(int size)
	{
		if (size != -1)
			mPanelSize = size;

		gtk_box_set_spacing(GTK_BOX(mBox), mPanelSize / 10);

		if (Settings::forceIconSize)
			mIconSize = Settings::iconSize;
		else
		{
			if (mPanelSize <= 20)
				mIconSize = mPanelSize - 6;
			else if (mPanelSize <= 28)
				mIconSize = 16;
			else if (mPanelSize <= 38)
				mIconSize = 24;
			else if (mPanelSize <= 41)
				mIconSize = 32;
			else
				mIconSize = mPanelSize * 0.8;
		}

		mGroups.forEach([](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> g) -> void { g.second->resize(); });
	}

	void onPanelOrientationChange(GtkOrientation orientation)
	{
		gtk_orientable_set_orientation(GTK_ORIENTABLE(mBox), orientation);

		if (Settings::dockSize)
		{
			if (orientation == GTK_ORIENTATION_HORIZONTAL)
				gtk_widget_set_size_request(mBox, Settings::dockSize, -1);
			else
				gtk_widget_set_size_request(mBox, -1, Settings::dockSize);
		}
	}
} // namespace Dock

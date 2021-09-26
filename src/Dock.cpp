/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Dock.hpp"

namespace Dock
{
	GtkWidget* mBox;
	Store::KeyStore<AppInfo*, Group*> mGroups;
	Help::Gtk::Timeout mDrawTimeout;

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

		// Redraw the panel items when the AppInfos have changed
		mDrawTimeout.setup(500, []() {
			if (AppInfos::modified)
				drawGroups();
			AppInfos::modified = false;
			return true;
		});
		mDrawTimeout.start();
	}

	Group* prepareGroup(AppInfo* appInfo)
	{
		Group* group = mGroups.get(appInfo);

		if (group == NULL)
		{
			group = new Group(appInfo, false);
			mGroups.push(appInfo, group);
			gtk_container_add(GTK_CONTAINER(mBox), group->mButton);
		}

		return group;
	}

	void moveButton(Group* moving, Group* dest)
	{
		int startpos = Help::Gtk::getChildPosition(GTK_CONTAINER(mBox), moving->mButton);
		int destpos = Help::Gtk::getChildPosition(GTK_CONTAINER(mBox), dest->mButton);

		if (startpos == destpos)
			return;
		if (startpos < destpos)
			--destpos;

		gtk_box_reorder_child(GTK_BOX(mBox), moving->mButton, destpos);

		savePinned();
	}

	void savePinned()
	{
		std::list<std::string> pinnedList;

		for (GList* child = gtk_container_get_children(GTK_CONTAINER(mBox));
			 child != NULL;
			 child = child->next)
		{
			GtkWidget* widget = (GtkWidget*)child->data;
			Group* group = (Group*)g_object_get_data(G_OBJECT(widget), "group");

			if (group->mPinned && g_file_test(group->mAppInfo->path.c_str(), G_FILE_TEST_IS_REGULAR))
				pinnedList.push_back(group->mAppInfo->path);
		}

		Settings::pinnedAppList.set(pinnedList);
	}

	void drawGroups()
	{
		// Remove old groups
		mGroups.forEach([](std::pair<AppInfo*, Group*> g) -> void { 
			gtk_widget_destroy(g.second->mButton);
		});
		mGroups.clear();
		Wnck::mGroupWindows.clear();

		// Add pinned groups
		std::list<std::string> pinnedApps = Settings::pinnedAppList;
		std::list<std::string>::iterator it = pinnedApps.begin();

		while (it != pinnedApps.end())
		{
			AppInfo* appInfo = AppInfos::search(*it);
			Group* group = new Group(appInfo, true);

			mGroups.push(appInfo, group);
			gtk_container_add(GTK_CONTAINER(mBox), group->mButton);
			++it;
		}

		// Add open windows
		for (GList* window_l = wnck_screen_get_windows(Wnck::mWnckScreen);
			 window_l != NULL;
			 window_l = window_l->next)
		{
			WnckWindow* wnckWindow = WNCK_WINDOW(window_l->data);
			gulong windowXID = wnck_window_get_xid(wnckWindow);
			GroupWindow* groupWindow = Wnck::mGroupWindows.get(windowXID);

			if (groupWindow == NULL)
				groupWindow = new GroupWindow(wnckWindow);
			else
				gtk_container_add(GTK_CONTAINER(mBox), groupWindow->mGroup->mButton);

			Wnck::mGroupWindows.push(windowXID, groupWindow);
			groupWindow->updateState();
		}

		gtk_widget_queue_draw(mBox);
	}

	void hoverSupered(bool on)
	{
		int grabbedKeys = Hotkeys::mGrabbedKeys;

		for (GList* child = gtk_container_get_children(GTK_CONTAINER(mBox));
			 child != NULL && grabbedKeys;
			 child = child->next)
		{
			GtkWidget* widget = (GtkWidget*)child->data;

			if (!gtk_widget_get_visible(widget))
				continue;

			Group* group = (Group*)g_object_get_data(G_OBJECT(widget), "group");
			group->mSSuper = on;
			--grabbedKeys;
		}
	}

	void activateGroup(int nb, guint32 timestamp)
	{
		int i = 0;

		for (GList* child = gtk_container_get_children(GTK_CONTAINER(mBox));
			 child != NULL;
			 child = child->next)
		{
			GtkWidget* widget = (GtkWidget*)child->data;

			if (gtk_widget_get_visible(widget))
				if (i == nb)
				{
					Group* group = (Group*)g_object_get_data(G_OBJECT(widget), "group");

					if (group->mSFocus)
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

		mGroups.forEach([](std::pair<AppInfo*, Group*> g) -> void { g.second->resize(); });
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

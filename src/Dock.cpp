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
#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "Settings.hpp"

#include <libxfce4windowing/libxfce4windowing.h>

Dock::Dock(XfcePanelPlugin* xfPlugin, Settings* settings, AppInfos* appInfos, Xfw* xfw) : mXfPlugin(xfPlugin), mSettings(settings), mAppInfos(appInfos), mXfw(xfw)
{
	mBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(mBox, "docklike-plugin");

	if (mSettings->dockSize)
		gtk_widget_set_size_request(mBox, mSettings->dockSize, -1);

	gtk_widget_show(mBox);
	drawGroups();
}

Group* Dock::prepareGroup(std::shared_ptr<AppInfo> appInfo)
{
	std::shared_ptr<Group> group = mGroups.get(appInfo);

	if (!group)
	{
		group = std::make_shared<Group>(appInfo, false, mSettings, mAppInfos, mXfw, this);
		mGroups.push(appInfo, group);
		gtk_container_add(GTK_CONTAINER(mBox), group->mButton);
	}

	return group.get();
}

void Dock::moveButton(Group* moving, Group* dest)
{
	int startpos = Help::Gtk::getChildPosition(GTK_CONTAINER(mBox), moving->mButton);
	int destpos = Help::Gtk::getChildPosition(GTK_CONTAINER(mBox), dest->mButton);

	if (startpos == destpos)
		return;

	gtk_box_reorder_child(GTK_BOX(mBox), moving->mButton, destpos);

	savePinned();
}

void Dock::savePinned()
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

	mSettings->pinnedAppList.set(pinnedList);
	g_list_free(children);
}

void Dock::drawGroups()
{
	// store launcher positions
	int i = 0;
	std::map<int, std::string> AppIds;
	GList* children = gtk_container_get_children(GTK_CONTAINER(mBox));
	for (GList* lp = children; lp != nullptr; lp = lp->next)
	{
		Group* group = (Group*)g_object_get_data(G_OBJECT(lp->data), "group");
		AppIds[i++] = group->mAppInfo->getId();
	}
	g_list_free(children);

	// Remove old groups
	mXfw->mGroupWindows.clear();
	mGroups.clear();

	// Add pinned groups
	std::list<std::string> pinnedApps = mSettings->pinnedAppList;
	std::list<std::string>::iterator it = pinnedApps.begin();

	while (it != pinnedApps.end())
	{
		std::shared_ptr<AppInfo> appInfo = mAppInfos->search(Help::String::toLowercase(*it));
		std::shared_ptr<Group> group = std::make_shared<Group>(appInfo, true, mSettings, mAppInfos, mXfw, this);

		mGroups.push(appInfo, group);
		gtk_container_add(GTK_CONTAINER(mBox), group->mButton);
		++it;
	}

	// Add open windows
	for (GList* window_l = xfw_screen_get_windows(mXfw->mXfwScreen);
		window_l != nullptr;
		window_l = window_l->next)
	{
		XfwWindow* xfwWindow = XFW_WINDOW(window_l->data);
		std::shared_ptr<GroupWindow> groupWindow = mXfw->mGroupWindows.get(xfwWindow);

		if (!groupWindow)
			groupWindow = std::make_shared<GroupWindow>(xfwWindow, mAppInfos, mXfw, this);
		else
			gtk_container_add(GTK_CONTAINER(mBox), groupWindow->mGroup->mButton);

		mXfw->mGroupWindows.push(xfwWindow, groupWindow);
		groupWindow->updateState();
	}

	// restore launcher positions, skipping dead ones
	Store::Map<std::string, GtkWidget*> buttons;
	children = gtk_container_get_children(GTK_CONTAINER(mBox));
	for (GList* lp = children; lp != nullptr; lp = lp->next)
	{
		GtkWidget* button = GTK_WIDGET(lp->data);
		Group* group = (Group*)g_object_get_data(G_OBJECT(button), "group");
		buttons.set(group->mAppInfo->getId(), button);
	}
	g_list_free(children);
	i = 0;
	for (const auto& _it : AppIds)
	{
		GtkWidget* button = buttons.get(_it.second);
		if (button != nullptr)
			gtk_box_reorder_child(GTK_BOX(mBox), button, _it.first - i);
		else
			i++;
	}

	gtk_widget_queue_draw(mBox);
	if (mLauncherEntry != nullptr)
		mLauncherEntry->refreshGroups();
}

void Dock::activateGroup(Group* group)
{
	if (group->mActive)
	{
		if (mSettings->keyComboMinimize || group->mWindowsCount == 1)
			group->minimizeAll();
		else
			group->scrollWindows(GDK_CURRENT_TIME, GDK_SCROLL_DOWN);
	}
	else if (group->mWindowsCount > 0)
		group->activate(GDK_CURRENT_TIME);
	else
		group->mAppInfo->launch();
}

void Dock::activateGroup(int nb)
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
				activateGroup(group);
				return;
			}
			else
				++i;
		}
	}

	g_list_free(children);
}

void Dock::activateGroup(const std::string& appId)
{
	std::shared_ptr<Group> group =
		mGroups.findIf([appId](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> g) -> bool {
			return g.first->mId == appId;
		});
	if (group)
		activateGroup(group.get());
}

void Dock::onPanelResize(int size)
{
	if (size != -1)
		mPanelSize = size;

	gtk_box_set_spacing(GTK_BOX(mBox), mPanelSize / 10);

	if (mSettings->forceIconSize)
		mIconSize = mSettings->iconSize;
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

void Dock::onPanelOrientationChange(GtkOrientation orientation)
{
	gtk_orientable_set_orientation(GTK_ORIENTABLE(mBox), orientation);

	GList* children = gtk_container_get_children(GTK_CONTAINER(mBox));
	for (GList* lp = children; lp != nullptr; lp = lp->next)
	{
		Group* group = (Group*)g_object_get_data(G_OBJECT(lp->data), "group");
		group->onPanelOrientationChange(orientation);
	}
	g_list_free(children);

	if (mSettings->dockSize)
	{
		if (orientation == GTK_ORIENTATION_HORIZONTAL)
			gtk_widget_set_size_request(mBox, mSettings->dockSize, -1);
		else
			gtk_widget_set_size_request(mBox, -1, mSettings->dockSize);
	}
}

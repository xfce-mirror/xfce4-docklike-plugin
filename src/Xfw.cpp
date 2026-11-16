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

#include "Xfw.hpp"
#include "Group.hpp"
#include "Helpers.hpp"

#include <fcntl.h>
#include <gio/gdesktopappinfo.h>
#include <libxfce4ui/libxfce4ui.h>

#include <map>
#include <unordered_set>

Xfw::Xfw(Plugin* plugin) : mPlugin(plugin)
{
	xfw_set_client_type(XFW_CLIENT_TYPE_PAGER);
	mXfwScreen = xfw_screen_get_default();

	g_signal_connect(G_OBJECT(mXfwScreen), "window-opened",
		G_CALLBACK(+[](XfwScreen* screen, XfwWindow* xfwWindow, Xfw* me) {
			std::shared_ptr<GroupWindow> newWindow = std::make_shared<GroupWindow>(xfwWindow, me->mPlugin);
			me->mGroupWindows.pushSecond(xfwWindow, newWindow);
			newWindow->mGroup->updateStyle();

			if (me->mPlugin->mSettings->showPreviews && newWindow->mGroup->mGroupMenu.mVisible)
				newWindow->mGroupMenuItem->mPreviewTimeout.start();
		}),
		this);

	g_signal_connect(G_OBJECT(mXfwScreen), "window-closed",
		G_CALLBACK(+[](XfwScreen* screen, XfwWindow* xfwWindow, Xfw* me) {
			me->mGroupWindows.pop(xfwWindow);
			if (xfwWindow == me->mPreviousActiveWindow)
				me->mPreviousActiveWindow = nullptr;
		}),
		this);

	g_signal_connect(G_OBJECT(mXfwScreen), "active-window-changed",
		G_CALLBACK(+[](XfwScreen* screen, XfwWindow* previousActiveWindow, Xfw* me) {
			XfwWindow* activeXfwWindow = me->getActiveWindow();
			if (activeXfwWindow != nullptr)
			{
				std::shared_ptr<GroupWindow> activeWindow = me->mGroupWindows.get(activeXfwWindow);
				Help::Gtk::cssClassAdd(GTK_WIDGET(activeWindow->mGroupMenuItem->mItem), "active_menu_item");
				gtk_widget_queue_draw(activeWindow->mGroup->mButton);
			}
			if (previousActiveWindow != nullptr)
			{
				std::shared_ptr<GroupWindow> prevWindow = me->mGroupWindows.get(previousActiveWindow);
				if (prevWindow)
				{
					Help::Gtk::cssClassRemove(GTK_WIDGET(prevWindow->mGroupMenuItem->mItem), "active_menu_item");
					gtk_widget_queue_draw(prevWindow->mGroup->mButton);
					me->mPreviousActiveWindow = previousActiveWindow;
				}
			}
			me->setActiveWindow(previousActiveWindow);
		}),
		this);

	// window<->workspace association only works on X11, where there is only one workspace group,
	// but it can be destroyed on wayland, so let's manage this in a minimalist way
	XfwWorkspaceManager* wpManager = xfw_screen_get_workspace_manager(mXfwScreen);
	auto workspaceGroupCreated = +[](XfwWorkspaceManager* manager, XfwWorkspaceGroup* group, Xfw* me) {
		if (me->mXfwWorkspaceGroup == nullptr)
		{
			me->mXfwWorkspaceGroup = XFW_WORKSPACE_GROUP(xfw_workspace_manager_list_workspace_groups(manager)->data);
			g_signal_connect(G_OBJECT(me->mXfwWorkspaceGroup), "active-workspace-changed",
				G_CALLBACK(+[](XfwScreen* screen, XfwWindow* xfwWindow, Xfw* _me) {
					_me->setVisibleGroups();
				}),
				me);
		}
	};
	auto workspaceGroupDestroyed = +[](XfwWorkspaceManager* manager, XfwWorkspaceGroup* group, Xfw* me) {
		if (group == me->mXfwWorkspaceGroup)
		{
			GList* groups = xfw_workspace_manager_list_workspace_groups(manager);
			if (groups == nullptr)
			{
				me->mXfwWorkspaceGroup = nullptr;
			}
			else
			{
				me->mXfwWorkspaceGroup = XFW_WORKSPACE_GROUP(groups->data);
				g_signal_connect(G_OBJECT(me->mXfwWorkspaceGroup), "active-workspace-changed",
					G_CALLBACK(+[](XfwScreen* screen, XfwWindow* xfwWindow, Xfw* _me) {
						_me->setVisibleGroups();
					}),
					me);
			}
		}
	};
	g_signal_connect(wpManager, "workspace-group-created", G_CALLBACK(workspaceGroupCreated), this);
	g_signal_connect(wpManager, "workspace-group-destroyed", G_CALLBACK(workspaceGroupDestroyed), this);
	workspaceGroupDestroyed(wpManager, nullptr, this);
}

Xfw::~Xfw()
{
	// we need to to it manually now, so it isn't done automatically *after* mXfwScreen is released
	mGroupWindows.clear();

	g_signal_handlers_disconnect_by_data(xfw_screen_get_workspace_manager(mXfwScreen), this);
	g_signal_handlers_disconnect_by_data(mXfwWorkspaceGroup, this);
	g_signal_handlers_disconnect_by_data(mXfwScreen, this);
	g_object_unref(mXfwScreen);
}

XfwWindow* Xfw::getActiveWindow()
{
	return xfw_screen_get_active_window(mXfwScreen);
}

// clang-format off
static std::unordered_set<std::string> mInvalidClassIds = {
	// https://gitlab.xfce.org/panel-plugins/xfce4-docklike-plugin/-/issues/75
	"wine",
	// https://gitlab.xfce.org/panel-plugins/xfce4-docklike-plugin/-/issues/21
// In theory, the "crx_*" fix below for #118 should cover all these cases,
// but it doesn't hurt to keep this as well, just in case…
	"brave-browser", "chromium", "google-chrome", "vivaldi-stable", "microsoft-edge",
};
// clang-format on

std::string Xfw::getGroupNameSys(XfwWindow* xfwWindow)
{
	// Xfw method const char *
	const gchar* const* class_ids = xfw_window_get_class_ids(xfwWindow);
	if (!xfce_str_is_empty(class_ids[0]))
	{
		// Chromium browsers use "crx_<appid>" as the instance-id for installed web
		// apps/PWAs, regardless of the browser (fork) name, so prefer it directly.
		// https://gitlab.xfce.org/panel-plugins/xfce4-docklike-plugin/-/issues/118
		if (!xfce_str_is_empty(class_ids[1]) && g_str_has_prefix(class_ids[1], "crx_"))
			return class_ids[1];

		// On X11, the class-id is generally the right app-id, but there is sometimes
		// an inversion with the instance-id. If there is only one 'class_ids' (wayland),
		// it's considered valid anyway
		if (!xfce_str_is_empty(class_ids[1])
			&& mInvalidClassIds.find(Help::String::toLowercase(class_ids[0])) != mInvalidClassIds.end())
			return class_ids[1];
		else
			return class_ids[0];
	}

	// proc/{pid}/cmdline method
	XfwApplicationInstance* instance = xfw_application_get_instance(xfw_window_get_application(xfwWindow), xfwWindow);
	char buffer[512];
	std::string path = "/proc/" + std::to_string(xfw_application_instance_get_pid(instance)) + "/cmdline";
	int fd = open(path.c_str(), O_RDONLY);

	if (fd >= 0)
	{
		int nbr = read(fd, buffer, 512);
		::close(fd);

		std::string exe_out = Help::String::pathBasename(buffer);
		if (exe_out != "python") // ADDIT graphical interpreters here
			return exe_out;

		char* it = buffer;
		while (*it++)
			;

		if (it < buffer + nbr)
		{
			return Help::String::pathBasename(it);
		}
	}

	// fallback : return window's name
	return xfw_window_get_name(xfwWindow);
}

std::string Xfw::getGroupName(GroupWindow* groupWindow)
{
	return Help::String::toLowercase(getGroupNameSys(groupWindow->mXfwWindow));
}

void Xfw::activate(GroupWindow* groupWindow, guint32 timestamp)
{
	XfwWorkspace* workspace = xfw_window_get_workspace(groupWindow->mXfwWindow);
	if (workspace != nullptr)
		xfw_workspace_activate(workspace, nullptr);

	xfw_window_activate(groupWindow->mXfwWindow, nullptr,
		timestamp != GDK_CURRENT_TIME ? timestamp : g_get_monotonic_time() / 1000,
		nullptr);
}

void Xfw::close(GroupWindow* groupWindow, guint32 timestamp)
{
	xfw_window_close(groupWindow->mXfwWindow,
		timestamp != GDK_CURRENT_TIME ? timestamp : g_get_monotonic_time() / 1000,
		nullptr);
}

void Xfw::setActiveWindow(XfwWindow* previousActiveWindow)
{
	XfwWindow* activeWindow = getActiveWindow();
	if (previousActiveWindow != nullptr)
	{
		std::shared_ptr<GroupWindow> groupWindow = mGroupWindows.get(previousActiveWindow);
		if (groupWindow)
			groupWindow->onUnactivate();
	}
	if (activeWindow != nullptr)
		mGroupWindows.get(activeWindow)->onActivate();
}

void Xfw::setVisibleGroups()
{
	for (GList* window_l = xfw_screen_get_windows(mXfwScreen);
		window_l != nullptr;
		window_l = window_l->next)
	{
		XfwWindow* xfwWindow = XFW_WINDOW(window_l->data);
		std::shared_ptr<GroupWindow> groupWindow = mGroupWindows.get(xfwWindow);

		groupWindow->leaveGroup();
		groupWindow->updateState();
	}
}

void Xfw::switchToLastWindow()
{
	if (mPreviousActiveWindow != nullptr)
	{
		std::shared_ptr<GroupWindow> prevWindow = mGroupWindows.get(mPreviousActiveWindow);
		if (prevWindow)
			prevWindow->activate(GDK_CURRENT_TIME);
	}
}

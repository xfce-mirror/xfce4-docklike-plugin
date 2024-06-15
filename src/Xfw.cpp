/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#endif

#include "Xfw.hpp"

namespace Xfw
{
	XfwScreen* mXfwScreen;
	XfwWorkspaceGroup* mXfwWorkspaceGroup;
	Store::KeyStore<XfwWindow*, std::shared_ptr<GroupWindow>> mGroupWindows;

	namespace // private:
	{
		std::string getGroupNameSys(XfwWindow* xfwWindow)
		{
			// Xfw method const char *
			const gchar* const* class_ids = xfw_window_get_class_ids(xfwWindow);
			if (!xfce_str_is_empty(class_ids[0]))
				return class_ids[0];

			// proc/{pid}/cmdline method
			XfwApplicationInstance* instance = xfw_application_get_instance(xfw_window_get_application(xfwWindow), xfwWindow);
			char buffer[512];
			std::string path = "/proc/" + std::to_string(xfw_application_instance_get_pid(instance)) + "/cmdline";
			int fd = open(path.c_str(), O_RDONLY);

			if (fd >= 0)
			{
				int nbr = read(fd, buffer, 512);
				::close(fd);

				char* exe = g_path_get_basename(buffer);
				std::string exe_out = exe;
				g_free(exe);
				if (exe_out != "python") // ADDIT graphical interpreters here
					return exe_out;

				char* it = buffer;
				while (*it++)
					;

				if (it < buffer + nbr)
				{
					gchar* basename = g_path_get_basename(it);
					std::string basename_out = basename;
					g_free(basename);
					return basename_out;
				}
			}

			// fallback : return window's name
			return xfw_window_get_name(xfwWindow);
		}
	} // namespace

	// public:

	void init()
	{
		xfw_set_client_type(XFW_CLIENT_TYPE_PAGER);
		mXfwScreen = xfw_screen_get_default();

		g_signal_connect(G_OBJECT(mXfwScreen), "window-opened",
			G_CALLBACK(+[](XfwScreen* screen, XfwWindow* xfwWindow) {
				std::shared_ptr<GroupWindow> newWindow = std::make_shared<GroupWindow>(xfwWindow);
				mGroupWindows.pushSecond(xfwWindow, newWindow);
				newWindow->mGroup->updateStyle();

				if (Settings::showPreviews && newWindow->mGroup->mGroupMenu.mVisible)
					newWindow->mGroupMenuItem->mPreviewTimeout.start();
			}),
			nullptr);

		g_signal_connect(G_OBJECT(mXfwScreen), "window-closed",
			G_CALLBACK(+[](XfwScreen* screen, XfwWindow* xfwWindow) {
				std::shared_ptr<GroupWindow> groupWindow = mGroupWindows.pop(xfwWindow);
			}),
			nullptr);

		g_signal_connect(G_OBJECT(mXfwScreen), "active-window-changed",
			G_CALLBACK(+[](XfwScreen* screen, XfwWindow* previousActiveWindow) {
				XfwWindow* activeXfwWindow = getActiveWindow();
				if (activeXfwWindow != nullptr)
				{
					std::shared_ptr<GroupWindow> activeWindow = mGroupWindows.get(activeXfwWindow);
					Help::Gtk::cssClassAdd(GTK_WIDGET(activeWindow->mGroupMenuItem->mItem), "active_menu_item");
					gtk_widget_queue_draw(activeWindow->mGroup->mButton);
				}
				if (previousActiveWindow != nullptr)
				{
					std::shared_ptr<GroupWindow> prevWindow = mGroupWindows.get(previousActiveWindow);
					if (prevWindow)
					{
						Help::Gtk::cssClassRemove(GTK_WIDGET(prevWindow->mGroupMenuItem->mItem), "active_menu_item");
						gtk_widget_queue_draw(prevWindow->mGroup->mButton);
					}
				}
				setActiveWindow();
			}),
			nullptr);

		XfwWorkspaceManager* wpManager = xfw_screen_get_workspace_manager(Xfw::mXfwScreen);
		mXfwWorkspaceGroup = XFW_WORKSPACE_GROUP(xfw_workspace_manager_list_workspace_groups(wpManager)->data);
		g_signal_connect(G_OBJECT(mXfwWorkspaceGroup), "active-workspace-changed",
			G_CALLBACK(+[](XfwScreen* screen, XfwWindow* xfwWindow) {
				setVisibleGroups();
			}),
			nullptr);
	}

	void finalize()
	{
		mGroupWindows.clear();
		g_signal_handlers_disconnect_by_data(mXfwWorkspaceGroup, nullptr);
		g_signal_handlers_disconnect_by_data(mXfwScreen, nullptr);
		g_object_unref(mXfwScreen);
	}

	XfwWindow* getActiveWindow()
	{
		return xfw_screen_get_active_window(mXfwScreen);
	}

	std::string getGroupName(GroupWindow* groupWindow)
	{
		return Help::String::toLowercase(getGroupNameSys(groupWindow->mXfwWindow));
	}

	void activate(GroupWindow* groupWindow, guint32 timestamp)
	{
#ifdef ENABLE_X11
		if (!timestamp && GDK_IS_X11_DISPLAY(gdk_display_get_default()))
			timestamp = gdk_x11_get_server_time(gdk_get_default_root_window());
#endif

		XfwWorkspace* workspace = xfw_window_get_workspace(groupWindow->mXfwWindow);
		if (workspace != nullptr)
			xfw_workspace_activate(workspace, NULL);

		xfw_window_activate(groupWindow->mXfwWindow, timestamp, NULL);
	}

	void close(GroupWindow* groupWindow, guint32 timestamp)
	{
#ifdef ENABLE_X11
		if (!timestamp && GDK_IS_X11_DISPLAY(gdk_display_get_default()))
			timestamp = gdk_x11_get_server_time(gdk_get_default_root_window());
#endif

		xfw_window_close(groupWindow->mXfwWindow, timestamp, NULL);
	}

	void setActiveWindow()
	{
		XfwWindow* activeWindow = getActiveWindow();
		if (mGroupWindows.size() > 0)
			mGroupWindows.first()->onUnactivate();
		if (activeWindow != nullptr)
			mGroupWindows.moveToStart(activeWindow)->onActivate();
	}

	void setVisibleGroups()
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

	void switchToLastWindow(guint32 timestamp)
	{
		auto it = mGroupWindows.mList.begin();

		while (it != mGroupWindows.mList.end() && it->second->getState(XFW_WINDOW_STATE_SKIP_TASKLIST))
			++it; // skip dialogs
		if (it != mGroupWindows.mList.end())
			++it; // skip current window

		while (it != mGroupWindows.mList.end())
		{
			if (!it->second->getState(XFW_WINDOW_STATE_SKIP_TASKLIST))
			{
				it->second->activate(timestamp);
				return;
			}
			++it;
		}
	}
} // namespace Xfw

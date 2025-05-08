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

#include <libxfce4ui/libxfce4ui.h>

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

		xfw_window_activate(groupWindow->mXfwWindow, NULL, timestamp, NULL);
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

	GtkWidget* buildActionMenu(GroupWindow* groupWindow, Group* group)
	{
		const bool groupStaysInTaskList = (groupWindow != nullptr && !groupWindow->getState(XFW_WINDOW_STATE_SKIP_TASKLIST));
		std::shared_ptr<AppInfo> appInfo = (groupWindow != nullptr) ? groupWindow->mGroup->mAppInfo : group->mAppInfo;

		GtkWidget* menu = nullptr;
		if (groupStaysInTaskList)
		{
			menu = xfw_window_action_menu_new(groupWindow->mXfwWindow);
		}
		else if (group->mPinned)
		{
			menu = gtk_menu_new();
		}
		else
		{
			// for the cases that the application is not pinned and it is not supposed to
			// show up in the task list, just don't show a menu at all
			return menu;
		}

		if (!appInfo->path.empty())
		{
			const gchar* const* actions = appInfo->get_actions();
			for (int i = 0; actions[i]; i++)
			{
				// Desktop actions get inserted into the menu above all the window manager controls.
				// We need an extra separator only if the application is running.
				if (i == 0 && group->mWindowsCount > 0)
					gtk_menu_shell_insert(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new(), 0);

				GDesktopAppInfo* GDAppInfo = g_desktop_app_info_new_from_filename(appInfo->path.c_str());
				gchar* action_name = g_desktop_app_info_get_action_name(GDAppInfo, actions[i]);
				GtkWidget* actionLauncher = gtk_menu_item_new_with_label(action_name);
				g_free(action_name);
				g_object_unref(GDAppInfo);

				g_object_set_data((GObject*)actionLauncher, "action", (gpointer)actions[i]);
				gtk_menu_shell_insert(GTK_MENU_SHELL(menu), actionLauncher, 0 + i);

				g_signal_connect(G_OBJECT(actionLauncher), "activate",
					G_CALLBACK(+[](GtkMenuItem* menuitem, AppInfo* _appInfo) {
						_appInfo->launch_action((const gchar*)g_object_get_data((GObject*)menuitem, "action"));
					}),
					appInfo.get());
			}

			if (group != nullptr)
			{
				GtkWidget* pinToggle = gtk_check_menu_item_new_with_label(group->mPinned ? _("Pinned to Dock") : _("Pin to Dock"));
				GtkWidget* editLauncher = gtk_menu_item_new_with_label(_("Edit Launcher"));

				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pinToggle), group->mPinned);
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
				gchar* program = g_find_program_in_path("xfce-desktop-item-edit");
#else
				gchar* program = g_find_program_in_path("exo-desktop-item-edit");
#endif
				if (program != nullptr)
				{
					gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), editLauncher);
					g_free(program);
				}

				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), pinToggle);

				g_signal_connect(G_OBJECT(pinToggle), "toggled",
					G_CALLBACK(+[](GtkCheckMenuItem* menuitem, Group* _group) {
						_group->mPinned = !_group->mPinned;
						if (!_group->mPinned)
							_group->updateStyle();
						Dock::savePinned();
					}),
					group);

				g_signal_connect(G_OBJECT(editLauncher), "activate",
					G_CALLBACK(+[](GtkMenuItem* menuitem, AppInfo* _appInfo) {
						_appInfo->edit();
					}),
					appInfo.get());

				if (group->mWindowsCount > 1)
				{
					GtkWidget* closeAll = gtk_menu_item_new_with_label(_("Close All"));

					gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), closeAll);

					g_signal_connect(G_OBJECT(closeAll), "activate",
						G_CALLBACK(+[](GtkMenuItem* menuitem, Group* _group) {
							_group->closeAll();
						}),
						group);
				}
			}
		}
		else if (group->mPinned)
		{
			if (groupStaysInTaskList) // if the window entries exist, add a separator
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

			// when a pinned app loses its icon (typically the app is uninstalled) the user may want to remove it from the dock
			GtkWidget* remove = gtk_menu_item_new_with_label(_("Remove"));
			gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), remove);

			g_signal_connect(G_OBJECT(remove), "activate",
				G_CALLBACK(+[](GtkMenuItem* menuitem, Group* _group) {
					_group->mPinned = false;
					Dock::savePinned();
					Dock::drawGroups();
				}),
				group);
		}

		// even for applications that don't have an icon and aren't pinned
		// (they don't enter in any of the conditions above)
		// we still want to show the window context menu
		gtk_widget_show_all(menu);

		return menu;
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

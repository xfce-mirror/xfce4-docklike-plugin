/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Wnck.hpp"

namespace Wnck
{
	WnckScreen* mWnckScreen;
	Store::KeyStore<gulong, std::shared_ptr<GroupWindow>> mGroupWindows;

	namespace // private:
	{
		std::string getGroupNameSys(WnckWindow* wnckWindow)
		{
			// Wnck method const char *
			const char* buf = wnck_window_get_class_group_name(wnckWindow);
			if (buf != NULL && buf[0] != '\0')
				return buf;

			buf = wnck_window_get_class_instance_name(wnckWindow);
			if (buf != NULL && buf[0] != '\0')
				return buf;

			// proc/{pid}/cmdline method
			char buffer[512];
			std::string path = "/proc/" + std::to_string(wnck_window_get_pid(wnckWindow)) + "/cmdline";
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
			return wnck_window_get_name(wnckWindow);
		}
	} // namespace

	// public:

	void init()
	{
		mWnckScreen = wnck_screen_get_default();

		g_signal_connect(G_OBJECT(mWnckScreen), "window-opened",
			G_CALLBACK(+[](WnckScreen* screen, WnckWindow* wnckWindow) {
				std::shared_ptr<GroupWindow> newWindow = std::make_shared<GroupWindow>(wnckWindow);
				mGroupWindows.pushSecond(wnck_window_get_xid(wnckWindow), newWindow);
				newWindow->mGroup->updateStyle();

				if (Settings::showPreviews && newWindow->mGroup->mGroupMenu.mVisible)
					newWindow->mGroupMenuItem->mPreviewTimeout.start();
			}),
			NULL);

		g_signal_connect(G_OBJECT(mWnckScreen), "window-closed",
			G_CALLBACK(+[](WnckScreen* screen, WnckWindow* wnckWindow) {
				std::shared_ptr<GroupWindow> groupWindow = mGroupWindows.pop(wnck_window_get_xid(wnckWindow));
			}),
			NULL);

		g_signal_connect(G_OBJECT(mWnckScreen), "active-window-changed",
			G_CALLBACK(+[](WnckScreen* screen, WnckWindow* previousActiveWindow) {
				gulong activeXID = getActiveWindowXID();
				if (activeXID)
				{
					std::shared_ptr<GroupWindow> activeWindow = mGroupWindows.get(activeXID);
					Help::Gtk::cssClassAdd(GTK_WIDGET(activeWindow->mGroupMenuItem->mItem), "active_menu_item");
					gtk_widget_queue_draw(activeWindow->mGroup->mButton);
				}
				if (previousActiveWindow != NULL)
				{
					gulong prevXID = wnck_window_get_xid(previousActiveWindow);
					if (prevXID)
					{
						std::shared_ptr<GroupWindow> prevWindow = mGroupWindows.get(prevXID);
						if (prevWindow != NULL)
						{
							prevWindow->mGroup->mSHover = false;
							Help::Gtk::cssClassRemove(GTK_WIDGET(prevWindow->mGroupMenuItem->mItem), "active_menu_item");
							gtk_widget_queue_draw(prevWindow->mGroup->mButton);
						}
					}
				}
				setActiveWindow();
			}),
			NULL);

		g_signal_connect(G_OBJECT(mWnckScreen), "active-workspace-changed",
			G_CALLBACK(+[](WnckScreen* screen, WnckWindow* wnckWindow) {
				setVisibleGroups();
			}),
			NULL);
	}

	gulong getActiveWindowXID()
	{
		WnckWindow* activeWindow = wnck_screen_get_active_window(mWnckScreen);
		if (!WNCK_IS_WINDOW(activeWindow))
			return 0;

		return wnck_window_get_xid(activeWindow);
	}

	std::string getGroupName(GroupWindow* groupWindow)
	{
		return Help::String::toLowercase(getGroupNameSys(groupWindow->mWnckWindow));
	}

	void activate(GroupWindow* groupWindow, guint32 timestamp)
	{
		if (!timestamp)
			timestamp = gdk_x11_get_server_time(gdk_get_default_root_window());

		WnckWorkspace* workspace = wnck_window_get_workspace(groupWindow->mWnckWindow);
		if (workspace != NULL)
			wnck_workspace_activate(workspace, timestamp);

		wnck_window_activate(groupWindow->mWnckWindow, timestamp);
	}

	void close(GroupWindow* groupWindow, guint32 timestamp)
	{
		if (!timestamp)
			timestamp = gdk_x11_get_server_time(gdk_get_default_root_window());

		wnck_window_close(groupWindow->mWnckWindow, timestamp);
	}

	void setActiveWindow()
	{
		gulong activeXID = getActiveWindowXID();
		if (activeXID)
		{
			mGroupWindows.first()->onUnactivate();
			mGroupWindows.moveToStart(activeXID)->onActivate();
		}
	}

	void setVisibleGroups()
	{
		for (GList* window_l = wnck_screen_get_windows(mWnckScreen);
			 window_l != NULL;
			 window_l = window_l->next)
		{
			WnckWindow* wnckWindow = WNCK_WINDOW(window_l->data);
			std::shared_ptr<GroupWindow> groupWindow = mGroupWindows.get(wnck_window_get_xid(wnckWindow));

			groupWindow->leaveGroup();
			groupWindow->updateState();
		}
	}

	GtkWidget* buildActionMenu(GroupWindow* groupWindow, Group* group)
	{
		GtkWidget* menu = (groupWindow != NULL && !groupWindow->getState(WNCK_WINDOW_STATE_SKIP_TASKLIST)) ? wnck_action_menu_new(groupWindow->mWnckWindow) : gtk_menu_new();
		std::shared_ptr<AppInfo> appInfo = (groupWindow != NULL) ? groupWindow->mGroup->mAppInfo : group->mAppInfo;

		if (!appInfo->path.empty())
		{
			const gchar* const* actions = appInfo->get_actions();
			for (int i = 0; actions[i]; i++)
			{
				// Desktop actions get inserted into the menu above all the window manager controls.
				// We need an extra separator only if the application is running.
				if (i == 0 && group->mSOpened)
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

			if (group != NULL)
			{
				GtkWidget* pinToggle = gtk_check_menu_item_new_with_label(group->mPinned ? _("Pinned to Dock") : _("Pin to Dock"));
				GtkWidget* editLauncher = gtk_menu_item_new_with_label(_("Edit Launcher"));

				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pinToggle), group->mPinned);
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

				gchar* program = g_find_program_in_path("exo-desktop-item-edit");
				if (program != NULL)
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

			gtk_widget_show_all(menu);

			return menu;
		}

		menu = gtk_menu_new();
		GtkWidget* remove = gtk_menu_item_new_with_label(_("Remove"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), remove);

		g_signal_connect(G_OBJECT(remove), "activate",
			G_CALLBACK(+[](GtkMenuItem* menuitem, Group* _group) {
				_group->mPinned = FALSE;
				Dock::savePinned();
				Dock::drawGroups();
			}),
			group);

		gtk_widget_show_all(menu);

		return menu;
	}

	void switchToLastWindow(guint32 timestamp)
	{
		auto it = mGroupWindows.mList.begin();

		while (it != mGroupWindows.mList.end() && it->second->getState(WNCK_WINDOW_STATE_SKIP_TASKLIST))
			++it; //skip dialogs
		if (it != mGroupWindows.mList.end())
			++it; //skip current window

		while (it != mGroupWindows.mList.end())
		{
			if (!it->second->getState(WNCK_WINDOW_STATE_SKIP_TASKLIST))
			{
				it->second->activate(timestamp);
				return;
			}
			++it;
		}
	}
} // namespace Wnck

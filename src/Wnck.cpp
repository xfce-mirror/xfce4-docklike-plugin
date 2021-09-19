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
	Store::KeyStore<gulong, GroupWindow*> mGroupWindows;

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

				char* exe = basename(buffer);
				if (strcmp(exe, "python") != 0) // ADDIT graphical interpreters here
					return exe;

				char* it = buffer;
				while (*it++)
					;

				if (it < buffer + nbr)
					return basename(it);
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
				GroupWindow* newWindow = new GroupWindow(wnckWindow);
				mGroupWindows.pushSecond(wnck_window_get_xid(wnckWindow), newWindow);
				newWindow->mGroup->updateStyle();
			}),
			NULL);

		g_signal_connect(G_OBJECT(mWnckScreen), "window-closed",
			G_CALLBACK(+[](WnckScreen* screen, WnckWindow* wnckWindow) {
				GroupWindow* groupWindow = mGroupWindows.pop(wnck_window_get_xid(wnckWindow));
				groupWindow->mGroup->updateStyle();
				delete groupWindow;
			}),
			NULL);

		g_signal_connect(G_OBJECT(mWnckScreen), "active-window-changed",
			G_CALLBACK(+[](WnckScreen* screen, WnckWindow* previousActiveWindow) {
				gulong activeXID = getActiveWindowXID();
				if (activeXID)
				{
					GroupWindow* activeWindow = mGroupWindows.get(activeXID);
					Help::Gtk::cssClassAdd(GTK_WIDGET(activeWindow->mGroupMenuItem->mItem), "active_menu_item");
				}
				if (previousActiveWindow != NULL)
				{
					gulong prevXID = wnck_window_get_xid(previousActiveWindow);
					if (prevXID)
					{
						GroupWindow* prevWindow = mGroupWindows.get(prevXID);
						if (prevWindow != NULL)
						{
							prevWindow->mGroup->mSHover = false;
							Help::Gtk::cssClassRemove(GTK_WIDGET(prevWindow->mGroupMenuItem->mItem), "active_menu_item");
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

		setActiveWindow();
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
			GroupWindow* groupWindow = mGroupWindows.get(wnck_window_get_xid(wnckWindow));

			groupWindow->leaveGroup();
			groupWindow->updateState();
		}
	}

	GtkWidget* buildActionMenu(GroupWindow* groupWindow, Group* group)
	{
		GtkWidget* menu = (groupWindow != NULL && !groupWindow->getState(WNCK_WINDOW_STATE_SKIP_TASKLIST)) ? wnck_action_menu_new(groupWindow->mWnckWindow) : gtk_menu_new();
		AppInfo* appInfo = (groupWindow != NULL) ? groupWindow->mGroup->mAppInfo : group->mAppInfo;

		if (!appInfo->path.empty())
		{
			for (int i = 0; appInfo->actions[i]; i++)
			{
				// Desktop actions get inserted into the menu above all the window manager controls.
				// We need an extra separator only if the application is running.
				if (i == 0 && group->mSOpened)
					gtk_menu_shell_insert(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new(), 0);

				GDesktopAppInfo* GDAppInfo = g_desktop_app_info_new_from_filename(appInfo->path.c_str());
				GtkWidget* actionLauncher = gtk_menu_item_new_with_label(_(g_desktop_app_info_get_action_name(GDAppInfo, appInfo->actions[i])));

				g_object_set_data((GObject*)actionLauncher, "action", (gpointer)appInfo->actions[i]);
				gtk_menu_shell_insert(GTK_MENU_SHELL(menu), actionLauncher, 0 + i);

				g_signal_connect(G_OBJECT(actionLauncher), "activate",
					G_CALLBACK(+[](GtkMenuItem* menuitem, AppInfo* appInfo) {
						appInfo->launch_action((const gchar*)g_object_get_data((GObject*)menuitem, "action"));
					}),
					appInfo);
			}

			if (group != NULL)
			{
				GtkWidget* pinToggle = gtk_check_menu_item_new_with_label(group->mPinned ? _("Pinned to Dock") : _("Pin to Dock"));
				GtkWidget* editLauncher = gtk_menu_item_new_with_label(_("Edit Launcher"));

				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pinToggle), group->mPinned);
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

				if (g_find_program_in_path("exo-desktop-item-edit"))
					gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), editLauncher);

				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), pinToggle);

				g_signal_connect(G_OBJECT(pinToggle), "toggled",
					G_CALLBACK(+[](GtkCheckMenuItem* menuitem, Group* group) {
						group->mPinned = !group->mPinned;
						if (!group->mPinned)
							group->updateStyle();
						Dock::savePinned();
					}),
					group);

				g_signal_connect(G_OBJECT(editLauncher), "activate",
					G_CALLBACK(+[](GtkMenuItem* menuitem, AppInfo* appInfo) {
						appInfo->edit();
					}),
					appInfo);

				if (group->mWindowsCount > 1)
				{
					GtkWidget* closeAll = gtk_menu_item_new_with_label(_("Close All"));

					gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), closeAll);

					g_signal_connect(G_OBJECT(closeAll), "activate",
						G_CALLBACK(+[](GtkMenuItem* menuitem, Group* group) {
							group->closeAll();
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
			G_CALLBACK(+[](GtkMenuItem* menuitem, Group* group)
					   {
						   group->mPinned = FALSE;
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

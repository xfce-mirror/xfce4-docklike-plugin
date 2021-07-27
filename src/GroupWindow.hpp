/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef GROUP_WINDOW_HPP
#define GROUP_WINDOW_HPP

#include <gtk/gtk.h>
#include <libwnck/libwnck.h>

#include <iostream>

#include "AppInfos.hpp"
#include "Dock.hpp"
#include "Group.hpp"
#include "GroupMenuItem.hpp"
#include "Helpers.hpp"
#include "Plugin.hpp"
#include "Wnck.hpp"

class GroupMenuItem;
class Group;

class GroupWindow
{
  public:
	GroupWindow(WnckWindow* wnckWindow);
	~GroupWindow();

	void updateState();
	void getInGroup();
	void leaveGroup();
	void onActivate();
	void onUnactivate() const;
	void minimize();
	void activate(guint32 timestamp);
	bool getState(WnckWindowState flagMask) const;

	Group* mGroup;
	GroupMenuItem* mGroupMenuItem;

	WnckWindow* mWnckWindow;
	GdkMonitor* mMonitor = NULL;

	unsigned short mState{};
	bool mGroupAssociated;
};

#endif // GROUP_WINDOW_HPP

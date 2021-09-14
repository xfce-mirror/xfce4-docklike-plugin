/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "GroupWindow.hpp"

GroupWindow::GroupWindow(WnckWindow* wnckWindow)
{
	mWnckWindow = wnckWindow;
	mGroupMenuItem = new GroupMenuItem(this);
	mGroupAssociated = false;

	std::string groupName = Wnck::getGroupName(this);

	if (PANEL_DEBUG)
		g_print("GROUP: %s\n", groupName.c_str());

	AppInfo* appInfo = AppInfos::search(groupName);
	mGroup = Dock::prepareGroup(appInfo);

	//--------------------------------------------------

	g_signal_connect(G_OBJECT(mWnckWindow), "name-changed",
		G_CALLBACK(+[](WnckWindow* window, GroupWindow* me) {
			me->mGroupMenuItem->updateLabel();
		}),
		this);

	g_signal_connect(G_OBJECT(mWnckWindow), "icon-changed",
		G_CALLBACK(+[](WnckWindow* window, GroupWindow* me) {
			me->mGroupMenuItem->updateIcon();
		}),
		this);

	g_signal_connect(G_OBJECT(mWnckWindow), "state-changed",
		G_CALLBACK(+[](WnckWindow* window, WnckWindowState changed_mask,
						WnckWindowState new_state, GroupWindow* me) {
			me->updateState();
		}),
		this);

	g_signal_connect(G_OBJECT(mWnckWindow), "workspace-changed",
		G_CALLBACK(+[](WnckWindow* window, GroupWindow* me) {
			me->updateState();
		}),
		this);

	g_signal_connect(G_OBJECT(mWnckWindow), "geometry-changed",
		G_CALLBACK(+[](WnckWindow* window, GroupWindow* me) {
			me->updateState();
		}),
		this);

	g_signal_connect(G_OBJECT(mWnckWindow), "class-changed",
		G_CALLBACK(+[](WnckWindow* window, GroupWindow* me) {
			std::string groupName = Wnck::getGroupName(me);
			Group* group = Dock::prepareGroup(AppInfos::search(groupName));
			if (group != me->mGroup)
			{
				me->leaveGroup();
				me->mGroup = group;
				me->getInGroup();
				Wnck::setActiveWindow();
			}
		}),
		this);

	updateState();
	mGroupMenuItem->updateIcon();
	mGroupMenuItem->updateLabel();
}

bool GroupWindow::getState(WnckWindowState flagMask) const
{
	return (mState & flagMask) != 0;
}

void GroupWindow::activate(guint32 timestamp)
{
	Wnck::activate(this, timestamp);
}

void GroupWindow::minimize()
{
	wnck_window_minimize(this->mWnckWindow);
}

GroupWindow::~GroupWindow()
{
	leaveGroup();
	delete mGroupMenuItem;
}

void GroupWindow::getInGroup()
{
	if (mGroupAssociated)
		return;
	mGroup->add(this);
	mGroupAssociated = true;
}

void GroupWindow::leaveGroup()
{
	if (!mGroupAssociated)
		return;
	mGroup->remove(this);
	mGroup->onWindowUnactivate();
	mGroupAssociated = false;
}

void GroupWindow::onActivate()
{
	gtk_widget_queue_draw(GTK_WIDGET(mGroupMenuItem->mItem));
	mGroupMenuItem->updateLabel();

	if (mGroupAssociated)
		mGroup->onWindowActivate(this);
}

void GroupWindow::onUnactivate() const
{
	gtk_widget_queue_draw(GTK_WIDGET(mGroupMenuItem->mItem));
	mGroupMenuItem->updateLabel();

	if (mGroupAssociated)
		mGroup->onWindowUnactivate();
}

void GroupWindow::updateState()
{
	bool onScreen = true;
	bool monitorChanged = false;
	bool onWorkspace = true;
	bool onTasklist = !(mState & WnckWindowState::WNCK_WINDOW_STATE_SKIP_TASKLIST);
	mState = wnck_window_get_state(this->mWnckWindow);

	if (Settings::onlyDisplayVisible)
	{
		WnckWorkspace* windowWorkspace = wnck_window_get_workspace(mWnckWindow);

		if (windowWorkspace != nullptr)
		{
			WnckWorkspace* activeWorkspace = wnck_screen_get_active_workspace(Wnck::mWnckScreen);

			if (windowWorkspace != activeWorkspace)
				onWorkspace = false;
		}
	}

	if (Settings::onlyDisplayScreen && gdk_display_get_n_monitors(Plugin::mDisplay) > 1)
	{
		gint x, y, w, h;

		wnck_window_get_geometry(mWnckWindow, &x, &y, &w, &h);

		GdkWindow* pluginWindow = gtk_widget_get_window(GTK_WIDGET(Plugin::mXfPlugin));
		GdkMonitor* currentMonitor = gdk_display_get_monitor_at_point(Plugin::mDisplay, x + (w / 2), y + (h / 2));

		if (gdk_display_get_monitor_at_window(Plugin::mDisplay, pluginWindow) != currentMonitor)
			onScreen = false;

		if (mMonitor != currentMonitor)
		{
			monitorChanged = true;
			mMonitor = currentMonitor;
		}
		else
			monitorChanged = false;
	}

	if (onWorkspace && onTasklist && onScreen)
	{
		getInGroup();
		if (monitorChanged)
			Wnck::setActiveWindow();
	}
	else
		leaveGroup();

	gtk_widget_show(GTK_WIDGET(mGroupMenuItem->mItem));
}

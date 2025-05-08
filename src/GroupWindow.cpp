/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "GroupWindow.hpp"

GroupWindow::GroupWindow(XfwWindow* xfwWindow)
{
	mXfwWindow = xfwWindow;
	mGroupMenuItem = new GroupMenuItem(this);
	mGroupAssociated = false;

	std::string groupName = Xfw::getGroupName(this);

	g_debug("NEW: %s", groupName.c_str());

	std::shared_ptr<AppInfo> appInfo = AppInfos::search(groupName);
	mGroup = Dock::prepareGroup(appInfo);

	//--------------------------------------------------

	g_signal_connect(G_OBJECT(mXfwWindow), "name-changed",
		G_CALLBACK(+[](XfwWindow* window, GroupWindow* me) {
			me->mGroupMenuItem->updateLabel();
		}),
		this);

	g_signal_connect(G_OBJECT(mXfwWindow), "icon-changed",
		G_CALLBACK(+[](XfwWindow* window, GroupWindow* me) {
			me->mGroupMenuItem->updateIcon();
		}),
		this);

	g_signal_connect(G_OBJECT(mXfwWindow), "state-changed",
		G_CALLBACK(+[](XfwWindow* window, XfwWindowState changed_mask,
						XfwWindowState new_state, GroupWindow* me) {
			me->updateState();
		}),
		this);

	g_signal_connect(G_OBJECT(mXfwWindow), "workspace-changed",
		G_CALLBACK(+[](XfwWindow* window, GroupWindow* me) {
			me->updateState();
		}),
		this);

	g_signal_connect(G_OBJECT(mXfwWindow), "notify::monitors",
		G_CALLBACK(+[](XfwWindow* window, GParamSpec* pspec, GroupWindow* me) {
			me->updateState();
			Xfw::setActiveWindow();
		}),
		this);

	g_signal_connect(G_OBJECT(mXfwWindow), "class-changed",
		G_CALLBACK(+[](XfwWindow* window, GroupWindow* me) {
			std::string _groupName = Xfw::getGroupName(me);
			Group* group = Dock::prepareGroup(AppInfos::search(_groupName));
			if (group != me->mGroup)
			{
				me->leaveGroup();
				me->mGroup = group;
				me->getInGroup();
				Xfw::setActiveWindow();
			}
		}),
		this);

	updateState();
	mGroupMenuItem->updateIcon();
	mGroupMenuItem->updateLabel();
}

bool GroupWindow::getState(XfwWindowState flagMask) const
{
	return (mState & flagMask) != 0;
}

void GroupWindow::activate(guint32 timestamp)
{
	Xfw::activate(this, timestamp);
}

void GroupWindow::minimize()
{
	xfw_window_set_minimized(this->mXfwWindow, TRUE, nullptr);
}

GroupWindow::~GroupWindow()
{
	leaveGroup();
	g_signal_handlers_disconnect_by_data(this->mXfwWindow, this);
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
	bool onWorkspace = true;
	bool onTasklist = !(mState & XfwWindowState::XFW_WINDOW_STATE_SKIP_TASKLIST);
	mState = xfw_window_get_state(this->mXfwWindow);

	if (Settings::onlyDisplayVisible)
	{
		XfwWorkspace* windowWorkspace = xfw_window_get_workspace(mXfwWindow);

		if (windowWorkspace != nullptr)
		{
			XfwWorkspace* activeWorkspace = xfw_workspace_group_get_active_workspace(Xfw::mXfwWorkspaceGroup);

			if (windowWorkspace != activeWorkspace)
				onWorkspace = false;
		}
	}

	if (Settings::onlyDisplayScreen && gdk_display_get_n_monitors(Plugin::mDisplay) > 1)
	{
		GList* monitors = xfw_window_get_monitors(mXfwWindow);
		GdkWindow* pluginWindow = gtk_widget_get_window(GTK_WIDGET(Plugin::mXfPlugin));

		if (!g_list_find_custom(
				monitors,
				gdk_display_get_monitor_at_window(Plugin::mDisplay, pluginWindow),
				[](gconstpointer a, gconstpointer b) { return xfw_monitor_get_gdk_monitor((XfwMonitor*)a) == b ? 0 : 1; }))
		{
			onScreen = false;
		}
	}

	if (onWorkspace && onTasklist && onScreen)
		getInGroup();
	else
		leaveGroup();

	gtk_widget_show(GTK_WIDGET(mGroupMenuItem->mItem));
}

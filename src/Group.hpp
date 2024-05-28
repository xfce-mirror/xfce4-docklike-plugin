/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef GROUP_HPP
#define GROUP_HPP

#include "AppInfos.hpp"
#include "GroupMenu.hpp"
#include "GroupWindow.hpp"
#include "Helpers.hpp"
#include "State.tpp"

#include <gtk/gtk.h>
#include <math.h>

#include <algorithm>
#include <iostream>

enum MiddleButtonBehavior
{
	BEHAVIOR_CLOSE_ALL,
	BEHAVIOR_LAUNCH_NEW,
	BEHAVIOR_DO_NOTHING
};

enum IndicatorOrientation
{
	ORIENTATION_AUTOMATIC,
	ORIENTATION_BOTTOM,
	ORIENTATION_RIGHT,
	ORIENTATION_TOP,
	ORIENTATION_LEFT,
};

enum IndicatorStyle
{
	STYLE_BARS,
	STYLE_DOTS,
	STYLE_RECTS,
	STYLE_CILIORA,
	STYLE_CIRCLES,
	STYLE_NONE
};

class GroupWindow;

class Group
{
public:
	Group(std::shared_ptr<AppInfo> appInfo, bool pinned);
	~Group();

	void add(GroupWindow* window);
	void remove(GroupWindow* window);
	void electNewTopWindow();
	void setTopWindow(GroupWindow* groupWindow);

	void activate(guint32 timestamp);
	void scrollWindows(guint32 timestamp, GdkScrollDirection direction);
	void closeAll();

	void resize();
	void updateStyle();

	void onDraw(cairo_t* cr);
	void onWindowActivate(GroupWindow* groupWindow);
	void onWindowUnactivate();
	void onButtonPress(GdkEventButton* event);
	void onButtonRelease(GdkEventButton* event);
	void onMouseEnter();
	void onMouseLeave();
	void setMouseLeaveTimeout();
	bool onDragMotion(GtkWidget* widget, GdkDragContext* context, int x, int y, guint time);
	void onDragLeave(const GdkDragContext* context, guint time);
	void onDragDataGet(const GdkDragContext* context, GtkSelectionData* selectionData, guint info, guint time);
	void onDragDataReceived(const GdkDragContext* context, int x, int y, const GtkSelectionData* selectionData, guint info, guint time);
	void onDragBegin(GdkDragContext* context);

	bool mPinned;
	bool mActive;

	uint mTolerablePointerDistance;
	uint mTopWindowIndex;
	Store::List<GroupWindow*> mWindows;
	LogicalState<uint> mWindowsCount;

	std::shared_ptr<AppInfo> mAppInfo;
	GroupMenu mGroupMenu;

	GtkWidget* mButton;
	GtkWidget* mLabel;
	GtkWidget* mImage;
	GdkPixbuf* mIconPixbuf;

	Help::Gtk::Timeout mLeaveTimeout;
	Help::Gtk::Timeout mMenuShowTimeout;
};

#endif // GROUP_HPP

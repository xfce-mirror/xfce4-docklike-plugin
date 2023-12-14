/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "GroupMenu.hpp"

#include "Group.hpp"
#include "GroupMenuItem.hpp"
#include "Plugin.hpp"

GroupMenu::GroupMenu(Group* dockButton)
{
	mGroup = dockButton;
	mVisible = false;
	mMouseHover = false;
	mWindow = gtk_window_new(GtkWindowType::GTK_WINDOW_POPUP);
	mBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	Help::Gtk::cssClassAdd(mBox, "menu");
	gtk_widget_add_events(mWindow, GDK_SCROLL_MASK);
	gtk_window_set_default_size(GTK_WINDOW(mWindow), 1, 1);
	gtk_container_add(GTK_CONTAINER(mWindow), mBox);
	gtk_widget_show(mBox);

	//--------------------------------------------------

	g_signal_connect(G_OBJECT(mWindow), "enter-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEvent* event, GroupMenu* me) {
			me->mMouseHover = true;
			me->mGroup->mSHover = true;

			me->mGroup->mWindows.forEach([](GroupWindow* w) -> void {
				gtk_widget_set_visible(GTK_WIDGET(w->mGroupMenuItem->mPreview), Settings::showPreviews);

				if (Settings::showPreviews)
					w->mGroupMenuItem->updatePreview();
			});

			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mWindow), "leave-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEvent* event, GroupMenu* me) {
			gint w;
			gint h;
			gtk_window_get_size(GTK_WINDOW(me->mWindow), &w, &h);
			gint mx = ((GdkEventCrossing*)event)->x;
			gint my = ((GdkEventCrossing*)event)->y;
			if (mx >= 0 && mx < w && my >= 0 && my < h)
				return true;

			me->mGroup->setMouseLeaveTimeout();
			me->mMouseHover = false;

			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mWindow), "scroll-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventScroll* event, GroupMenu* me) {
			((Group*)me->mGroup)->scrollWindows(event->time, event->direction);
			return true;
		}),
		this);
}

GroupMenu::~GroupMenu()
{
	gtk_widget_destroy(mWindow);
}

void GroupMenu::add(GroupMenuItem* menuItem)
{
	gtk_box_pack_end(GTK_BOX(mBox), GTK_WIDGET(menuItem->mItem), false, true, 0);
}

void GroupMenu::remove(GroupMenuItem* menuItem)
{
	gtk_container_remove(GTK_CONTAINER(mBox), GTK_WIDGET(menuItem->mItem));
	gtk_window_resize(GTK_WINDOW(mWindow), 1, 1);

	if (mGroup->mWindowsCount < (Settings::noWindowsListIfSingle ? 2 : 1))
		gtk_widget_hide(mWindow);
}

void GroupMenu::popup()
{
	if (mGroup->mWindowsCount >= (Settings::noWindowsListIfSingle ? 2 : 1))
	{
		gint wx, wy;
		mVisible = true;

		updateOrientation();

		// Update the previews before showing the window
		if (Settings::showPreviews)
			mGroup->mWindows.forEach([](GroupWindow* w) -> void {
				w->mGroupMenuItem->updatePreview();
			});

		xfce_panel_plugin_position_widget(Plugin::mXfPlugin, mWindow, mGroup->mButton, &wx, &wy);
		updatePosition(wx, wy);
		gtk_widget_show(mWindow);
	}
}

void GroupMenu::updateOrientation()
{
	XfcePanelPluginMode panelMode = xfce_panel_plugin_get_mode(Plugin::mXfPlugin);

	if (Settings::showPreviews && panelMode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
		gtk_orientable_set_orientation(GTK_ORIENTABLE(mBox), GTK_ORIENTATION_HORIZONTAL);
	else
		gtk_orientable_set_orientation(GTK_ORIENTABLE(mBox), GTK_ORIENTATION_VERTICAL);
}

void GroupMenu::updatePosition(gint wx, gint wy)
{
	GdkScreen *screen;
	GdkRectangle geometry;
	GdkDisplay *display;
	GdkMonitor *monitor;

	screen = gtk_widget_get_screen(mGroup->mButton);
	display = gdk_screen_get_display(screen);
	monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(mGroup->mButton));
	gdk_monitor_get_geometry(monitor, &geometry);

	gint window_width, window_height;
	gtk_window_get_size(GTK_WINDOW(mWindow), &window_width, &window_height);

	gint button_width = gtk_widget_get_allocated_width(mGroup->mButton);
	gint button_height = gtk_widget_get_allocated_height(mGroup->mButton);
	
	XfcePanelPluginMode panelMode = xfce_panel_plugin_get_mode(Plugin::mXfPlugin);

	if (panelMode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
	{
		if (wx != geometry.x + geometry.width - window_width) {
			wx -= (window_width/2) - (button_width/2);
			wx = wx < geometry.x ? geometry.x : wx;
		}
	} else {
		if (wy != geometry.y + geometry.height - window_height) {
			wy -= (window_height/2) - (button_height/2);
			wy = wy < geometry.y ? geometry.y : wy;
		}
	}

	gtk_window_move(GTK_WINDOW(mWindow), wx, wy);
}

void GroupMenu::hide()
{
	mVisible = false;
	gtk_widget_hide(mWindow);
}

uint GroupMenu::getPointerDistance()
{
	gint wx, wy, ww, wh, px, py;
	guint dx, dy;
	dx = dy = 0;

	gtk_window_get_position(GTK_WINDOW(mWindow), &wx, &wy);
	gtk_window_get_size(GTK_WINDOW(mWindow), &ww, &wh);
	gdk_device_get_position(Plugin::mPointer, NULL, &px, &py);

	if (px < wx)
		dx = wx - px;
	else if (px > wx + ww)
		dx = px - (wx + ww);

	if (py < wy)
		dy = wy - py;
	else if (py > wy + wh)
		dy = py - (wy + wh);

	return std::max(dx, dy);
}

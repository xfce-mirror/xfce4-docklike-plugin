/*
 * Docklike Taskbar - A modern, minimalist taskbar for XFCE
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * gnu.org/licenses/gpl-3.0
 */

#include "Group.hpp"
#include "Helpers.hpp"

static GtkTargetEntry entries[1] = {{(gchar*)"application/docklike_group", 0, 0}};
static GtkTargetList* targetList = gtk_target_list_new(entries, 1);

Group::Group(AppInfo* appInfo, bool pinned) : mGroupMenu(this)
{
	mButton = gtk_button_new();
	mIconPixbuf = NULL;
	mAppInfo = appInfo;
	mPinned = pinned;
	mTopWindowIndex = 0;
	mActive = mSFocus = mSOpened = mSMany = false;

	//--------------------------------------------------

	mWindowsCount.setup(
		0, [this]() -> uint {
			uint count = 0;
			
			mWindows.findIf([&count](GroupWindow* e) -> bool {
				if (!e->getState(WnckWindowState::WNCK_WINDOW_STATE_SKIP_TASKLIST))
				{
					++count;
					if (count == 2)
						return true;
				}
			return false;
		});

		return count; },
		[this](uint windowsCount) -> void {
			updateStyle();
		});

	mLeaveTimeout.setup(40, [this]() {
		uint distance = mGroupMenu.getPointerDistance();

		if (distance >= mTolerablePointerDistance)
		{
			onMouseLeave();
			return false;
		}

		mTolerablePointerDistance -= 10;
		return true;
	});

	mMenuShowTimeout.setup(90, [this]() {
		onMouseEnter();
		return false;
	});

	//--------------------------------------------------

	g_signal_connect(
		G_OBJECT(mButton), "button-press-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, Group* me) {
			if (event->button != 3 && event->state & GDK_CONTROL_MASK)
				gtk_drag_begin_with_coordinates(widget, targetList, GDK_ACTION_MOVE, event->button, (GdkEvent*)event, -1, -1);

			if (event->state & GDK_CONTROL_MASK)
			{
				me->mGroupMenu.hide();
				return false;
			}

			me->onButtonPress(event);
			return true;
		}),
		this);

	g_signal_connect(
		G_OBJECT(mButton), "button-release-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, Group* me) {
			if (event->button != 1 && event->button != 2)
				return false;
			me->onButtonRelease(event);
			return true;
		}),
		this);

	g_signal_connect(
		G_OBJECT(mButton), "scroll-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventScroll* event, Group* me) {
			me->scrollWindows(event->time, event->direction);
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "drag-begin",
		G_CALLBACK(+[](GtkWidget* widget, GdkDragContext* context, Group* me) {
			me->onDragBegin(context);
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "drag-motion",
		G_CALLBACK(+[](GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time, Group* me) {
			return me->onDragMotion(widget, context, x, y, time);
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "drag-leave",
		G_CALLBACK(+[](GtkWidget* widget, GdkDragContext* context, guint time, Group* me) {
			me->onDragLeave(context, time);
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "drag-data-get",
		G_CALLBACK(+[](GtkWidget* widget, GdkDragContext* context, GtkSelectionData* data, guint info, guint time, Group* me) {
			me->onDragDataGet(context, data, info, time);
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "drag-data-received",
		G_CALLBACK(+[](GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint info, guint time, Group* me) {
			me->onDragDataReceived(context, x, y, data, info, time);
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "enter-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventCrossing* event, Group* me) {
			gtk_widget_set_name(widget, "hover_group");
			me->mLeaveTimeout.stop();
			me->mMenuShowTimeout.start();

			if (Settings::showPreviews)
				me->mWindows.forEach([](GroupWindow* w) -> void {
					w->mGroupMenuItem->mPreviewTimeout.start();
				});

			return true;
		}),
		this);

	g_signal_connect(
		G_OBJECT(mButton), "leave-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventCrossing* event, Group* me) {
			gtk_widget_set_name(widget, "");

			me->mMenuShowTimeout.stop();

			if (me->mPinned && me->mWindowsCount == 0)
				me->onMouseLeave();
			else
				me->setMouseLeaveTimeout();

			if (Settings::showPreviews)
				me->mWindows.forEach([](GroupWindow* w) -> void {
					w->mGroupMenuItem->mPreviewTimeout.stop();
				});

			return true;
		}),
		this);

	//--------------------------------------------------

	gtk_drag_dest_set(mButton, GTK_DEST_DEFAULT_DROP, entries, 1, GDK_ACTION_MOVE);
	g_object_set_data(G_OBJECT(mButton), "group", this);
	gtk_button_set_relief(GTK_BUTTON(mButton), GTK_RELIEF_NONE);
	gtk_widget_add_events(mButton, GDK_SCROLL_MASK);
	gtk_button_set_always_show_image(GTK_BUTTON(mButton), true);
	Help::Gtk::cssClassAdd(mButton, "group");
	Help::Gtk::cssClassAdd(mButton, "flat");

	if (mPinned)
		gtk_widget_show(mButton);

	if (mAppInfo != NULL && !mAppInfo->icon.empty())
	{
		if (mAppInfo->icon[0] == '/' && g_file_test(mAppInfo->icon.c_str(), G_FILE_TEST_IS_REGULAR))
			mIconPixbuf = gdk_pixbuf_new_from_file(mAppInfo->icon.c_str(), NULL);
		else
			gtk_button_set_image(GTK_BUTTON(mButton),
				gtk_image_new_from_icon_name(mAppInfo->icon.c_str(), GTK_ICON_SIZE_BUTTON));
	}
	else
		gtk_button_set_image(GTK_BUTTON(mButton),
			gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_BUTTON));

	resize();
	updateStyle();
}

void Group::add(GroupWindow* window)
{
	mWindows.push(window);
	mWindowsCount.updateState();
	mGroupMenu.add(window->mGroupMenuItem);
	Help::Gtk::cssClassAdd(GTK_WIDGET(mButton), "open_group");

	if (mWindowsCount == 1 && !mPinned)
		gtk_box_reorder_child(GTK_BOX(Dock::mBox), GTK_WIDGET(mButton), -1);
}

void Group::remove(GroupWindow* window)
{
	mWindows.pop(window);
	mWindowsCount.updateState();
	mGroupMenu.remove(window->mGroupMenuItem);

	if (!mWindowsCount)
		Help::Gtk::cssClassRemove(GTK_WIDGET(mButton), "open_group");
}

void Group::activate(guint32 timestamp)
{
	if (mWindowsCount == 0)
		return;

	GroupWindow* groupWindow = mWindows.get(mTopWindowIndex);

	mWindows.forEach([&timestamp, &groupWindow](GroupWindow* w) -> void {
		if (w != groupWindow)
			w->activate(timestamp);
	});

	groupWindow->activate(timestamp);
}

void Group::scrollWindows(guint32 timestamp, GdkScrollDirection direction)
{
	if (mPinned && mWindowsCount == 0)
		return;

	if (!mActive)
		mWindows.get(mTopWindowIndex)->activate(timestamp);
	else
	{
		if (direction == GDK_SCROLL_UP)
			mTopWindowIndex = ++mTopWindowIndex % mWindows.size();
		else if (direction == GDK_SCROLL_DOWN)
		{
			int size = mWindows.size();
			mTopWindowIndex = (--mTopWindowIndex + size) % size;
		}
		mWindows.get(mTopWindowIndex)->activate(timestamp);
	}
}

void Group::closeAll()
{
	mWindows.forEach([](GroupWindow* w) -> void {
		if (!w->getState(WNCK_WINDOW_STATE_SKIP_TASKLIST))
			Wnck::close(w, 0);
	});
}

void Group::resize()
{
	GtkWidget* img;
	gtk_widget_set_size_request(mButton, (round((Dock::mPanelSize * 1.2) / 2) * 2), Dock::mPanelSize);

	if (mIconPixbuf != NULL)
	{
		GdkPixbuf* pixbuf = gdk_pixbuf_scale_simple(mIconPixbuf, Dock::mIconSize, Dock::mIconSize, GDK_INTERP_HYPER);
		GtkWidget* icon = gtk_image_new_from_pixbuf(pixbuf);
		gtk_button_set_image(GTK_BUTTON(mButton), icon);
		img = gtk_button_get_image(GTK_BUTTON(mButton));
	}
	else
	{
		img = gtk_button_get_image(GTK_BUTTON(mButton));
		gtk_image_set_pixel_size(GTK_IMAGE(img), Dock::mIconSize);
	}

	gtk_widget_set_valign(img, GTK_ALIGN_CENTER);
	gtk_widget_queue_draw(mButton);
}

void Group::onMouseEnter()
{
	mLeaveTimeout.stop();

	Dock::mGroups.forEach([this](std::pair<AppInfo*, Group*> g) -> void {
		if (&(g.second->mGroupMenu) != &(this->mGroupMenu))
			g.second->mGroupMenu.mGroup->onMouseLeave();
	});

	mGroupMenu.popup();
}

void Group::onMouseLeave()
{
	if (!mGroupMenu.mMouseHover)
		mGroupMenu.hide();
	if (mWindowsCount)
		Help::Gtk::cssClassAdd(mButton, "open_group");
	else
	 	Help::Gtk::cssClassRemove(mButton, "open_group");
}

void Group::setMouseLeaveTimeout()
{
	mTolerablePointerDistance = 200;
	mLeaveTimeout.start();
}

void Group::updateStyle()
{
	if (mPinned || mWindowsCount)
		gtk_widget_show(mButton);
	else
		gtk_widget_hide(mButton);

	if (mWindowsCount)
	{
		if (mWindowsCount == 1 && Settings::noWindowsListIfSingle)
			gtk_widget_set_tooltip_text(mButton, mAppInfo->name.c_str());
		else
			gtk_widget_set_tooltip_text(mButton, NULL);
	}
	else
		gtk_widget_set_tooltip_text(mButton, mAppInfo->name.c_str());
}

void Group::electNewTopWindow()
{
	if (mWindows.size() > 0)
	{
		GroupWindow* newTopWindow;

		if (mWindows.size() == 1)
			newTopWindow = mWindows.get(0);
		else
			newTopWindow = Wnck::mGroupWindows.findIf([this](std::pair<gulong, GroupWindow*> e) -> bool {
				if (e.second->mGroup == this)
					return true;
				return false;
			});

		setTopWindow(newTopWindow);
	}
}

void Group::onWindowActivate(GroupWindow* groupWindow)
{
	if (!groupWindow->getState(WNCK_WINDOW_STATE_SKIP_TASKLIST))
	{
		mActive = true;
		setTopWindow(groupWindow);
		gtk_widget_set_name(mButton, "active_group");
		Help::Gtk::cssClassRemove(mButton, "open_group");
	}
}

void Group::onWindowUnactivate()
{
	mActive = false;
	gtk_widget_set_name(mButton, "");
	if (mWindowsCount)
		Help::Gtk::cssClassAdd(mButton, "open_group");
}

void Group::setTopWindow(GroupWindow* groupWindow)
{
	mTopWindowIndex = mWindows.getIndex(groupWindow);
}

void Group::onButtonPress(GdkEventButton* event)
{
	if (event->button == 3)
	{
		GroupWindow* win = Wnck::mGroupWindows.findIf([this](std::pair<gulong, GroupWindow*> e) -> bool {
			return (e.second->mGroupAssociated && e.second->mGroup == this);
		});

		if (win == NULL && !mPinned)
			return;

		GtkWidget* menu = Wnck::buildActionMenu(win, this);

		xfce_panel_plugin_register_menu(Plugin::mXfPlugin, GTK_MENU(menu));
		gtk_menu_attach_to_widget(GTK_MENU(menu), GTK_WIDGET(mButton), NULL);
		gtk_menu_popup_at_widget(GTK_MENU(menu), GTK_WIDGET(mButton), GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, (GdkEvent*)event);

		mGroupMenu.hide();
	}
}

void Group::onButtonRelease(GdkEventButton* event)
{
	if (event->button == 2)
		closeAll();
	else if (event->state & GDK_SHIFT_MASK || (mPinned && mWindowsCount == 0))
		mAppInfo->launch();
	else if (mActive)
		mWindows.get(mTopWindowIndex)->minimize();
	else if (!mActive)
		activate(event->time);
}

bool Group::onDragMotion(GtkWidget* widget, GdkDragContext* context, int x, int y, guint time)
{
	GdkModifierType mask;
	GdkDevice* device = gdk_drag_context_get_device(context);
	gdk_window_get_device_position(gtk_widget_get_window(widget), device, NULL, NULL, &mask);

	if (mask & GDK_CONTROL_MASK)
		gtk_drag_cancel(context);

	GList* tmpList = gdk_drag_context_list_targets(context);

	if (tmpList != NULL)
	{
		std::string target = gdk_atom_name(GDK_POINTER_TO_ATOM(tmpList->data));

		if (target != "application/docklike_group")
		{
			if (mWindowsCount > 0)
			{
				GroupWindow* groupWindow = mWindows.get(mTopWindowIndex);
				groupWindow->activate(time);

				if (!mGroupMenu.mVisible)
					onMouseEnter();
			}

			gdk_drag_status(context, GDK_ACTION_DEFAULT, time);
			return true;
		}
	}

	gtk_widget_set_name(GTK_WIDGET(mButton), "drop_target");
	gdk_drag_status(context, GDK_ACTION_MOVE, time);
	return true;
}

void Group::onDragLeave(const GdkDragContext* context, guint time)
{
	gtk_widget_set_name(GTK_WIDGET(mButton), "");
}

void Group::onDragDataGet(const GdkDragContext* context, GtkSelectionData* selectionData, guint info, guint time)
{
	gtk_selection_data_set(selectionData, gdk_atom_intern("button", false), 32, (const guchar*)this, sizeof(gpointer) * 32);
}

void Group::onDragDataReceived(const GdkDragContext* context, int x, int y, const GtkSelectionData* selectionData, guint info, guint time)
{
	Dock::moveButton((Group*)gtk_selection_data_get_data(selectionData), this);
}

void Group::onDragBegin(GdkDragContext* context)
{
	gtk_drag_set_icon_name(context, mAppInfo->icon.c_str(), 0, 0);
}
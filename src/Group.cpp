/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Group.hpp"

static GtkTargetEntry entries[1] = {{(gchar*)"application/docklike_group", 0, 0}};
static GtkTargetList* targetList = gtk_target_list_new(entries, 1);

Group::Group(AppInfo* appInfo, bool pinned) : mGroupMenu(this)
{
	mIconPixbuf = NULL;
	mAppInfo = appInfo;
	mPinned = pinned;
	mTopWindowIndex = 0;
	mActive = mSFocus = mSOpened = mSMany = mSHover = mSSuper = false;

	//--------------------------------------------------

	mWindowsCount.setup(
		0,
		[this]() -> uint {
			uint count = 0;
			
			mWindows.findIf([&count](GroupWindow* e) -> bool {
				if (!e->getState(WNCK_WINDOW_STATE_SKIP_TASKLIST))
					++count;
			return false;
		
		}); return count; },
		[this](uint windowsCount) -> void { updateStyle(); });

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

	mButton = gtk_button_new();
	mImage = gtk_image_new();
	mLabel = gtk_label_new("");
	GtkWidget* overlay = gtk_overlay_new();

	// The button contains a GtkOverlay, so that the label can be placed on top of the image.
	gtk_label_set_use_markup(GTK_LABEL(mLabel), true);
	gtk_container_add(GTK_CONTAINER(overlay), mImage);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), mLabel);
	gtk_widget_set_halign(GTK_WIDGET(mLabel), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(mLabel), GTK_ALIGN_START);
	gtk_overlay_set_overlay_pass_through(GTK_OVERLAY(overlay), mLabel, true);
	gtk_container_add(GTK_CONTAINER(mButton), overlay);

	Help::Gtk::cssClassAdd(mButton, "flat");
	Help::Gtk::cssClassAdd(mButton, "group");
	g_object_set_data(G_OBJECT(mButton), "group", this);
	gtk_button_set_relief(GTK_BUTTON(mButton), GTK_RELIEF_NONE);
	gtk_drag_dest_set(mButton, GTK_DEST_DEFAULT_DROP, entries, 1, GDK_ACTION_MOVE);
	gtk_widget_add_events(mButton, GDK_SCROLL_MASK);

	//--------------------------------------------------

	g_signal_connect(G_OBJECT(mButton), "button-press-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, Group* me) {
			if (event->button != GDK_BUTTON_SECONDARY && event->state & GDK_CONTROL_MASK)
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

	g_signal_connect(G_OBJECT(mButton), "button-release-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, Group* me) {
			if (event->button != GDK_BUTTON_PRIMARY && event->button != GDK_BUTTON_MIDDLE)
				return false;
			me->onButtonRelease(event);
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "scroll-event",
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
			Help::Gtk::cssClassAdd(me->mButton, "hover_group");
			me->mSHover = true;
			me->mLeaveTimeout.stop();
			me->mMenuShowTimeout.start();

			if (Settings::showPreviews)
				me->mWindows.forEach([](GroupWindow* w) -> void {
					w->mGroupMenuItem->mPreviewTimeout.start();
				});

			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "leave-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventCrossing* event, Group* me) {
			Help::Gtk::cssClassRemove(me->mButton, "hover_group");
			me->mSHover = false;
			me->mMenuShowTimeout.stop();

			if (me->mPinned && !me->mWindowsCount)
				me->onMouseLeave();
			else
				me->setMouseLeaveTimeout();

			if (Settings::showPreviews)
				me->mWindows.forEach([](GroupWindow* w) -> void {
					w->mGroupMenuItem->mPreviewTimeout.stop();
				});

			return false;
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "draw",
		G_CALLBACK(+[](GtkWidget* widget, cairo_t* cr, Group* me) {
			me->onDraw(cr);
			return false;
		}),
		this);

	//--------------------------------------------------

	if (mPinned)
		gtk_widget_show_all(mButton);

	if (mAppInfo != NULL && !mAppInfo->icon.empty())
	{
		if (mAppInfo->icon[0] == '/' && g_file_test(mAppInfo->icon.c_str(), G_FILE_TEST_IS_REGULAR))
		{
			mIconPixbuf = gdk_pixbuf_new_from_file(mAppInfo->icon.c_str(), NULL);
			gtk_image_set_from_pixbuf(GTK_IMAGE(mImage), mIconPixbuf);
		}
		else
			gtk_image_set_from_icon_name(GTK_IMAGE(mImage), mAppInfo->icon.c_str(), GTK_ICON_SIZE_BUTTON);
	}
	else
		gtk_image_set_from_icon_name(GTK_IMAGE(mImage), "application-x-executable", GTK_ICON_SIZE_BUTTON);

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

	gtk_widget_queue_draw(mButton);
}

void Group::remove(GroupWindow* window)
{
	mWindows.pop(window);
	mWindowsCount.updateState();
	mGroupMenu.remove(window->mGroupMenuItem);
	mSFocus = false;

	if (!mWindowsCount)
		Help::Gtk::cssClassRemove(GTK_WIDGET(mButton), "open_group");

	gtk_widget_queue_draw(mButton);
}

void Group::activate(guint32 timestamp)
{
	if (!mWindowsCount)
		return;

	GroupWindow* groupWindow = mWindows.get(mTopWindowIndex);

	mWindows.forEach([&timestamp, &groupWindow](GroupWindow* w) -> void {
		if (w != groupWindow)
			w->activate(timestamp);
	});

	groupWindow->activate(timestamp);
	onWindowActivate(groupWindow);
}

void Group::scrollWindows(guint32 timestamp, GdkScrollDirection direction)
{
	if (mPinned && !mWindowsCount)
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
	// TODO: set `min-width` / `min-height` CSS property on button?
	// https://github.com/davekeogh/xfce4-docklike-plugin/issues/39
	
	if (mIconPixbuf != NULL)
		gtk_image_set_from_pixbuf(GTK_IMAGE(mImage),
			gdk_pixbuf_scale_simple(mIconPixbuf, Dock::mIconSize, Dock::mIconSize, GDK_INTERP_HYPER));
	else
		gtk_image_set_pixel_size(GTK_IMAGE(mImage), Dock::mIconSize);

	gtk_widget_set_valign(mImage, GTK_ALIGN_CENTER);
	gtk_widget_queue_draw(mButton);
}

void Group::onDraw(cairo_t* cr)
{
	int w = gtk_widget_get_allocated_width(GTK_WIDGET(mButton));
	int h = gtk_widget_get_allocated_height(GTK_WIDGET(mButton));
	
	double rgba[4];
	if (mSFocus)
	{
		rgba[0] = (*Settings::indicatorColor).red;
		rgba[1] = (*Settings::indicatorColor).green;
		rgba[2] = (*Settings::indicatorColor).blue;
		rgba[3] = (*Settings::indicatorColor).alpha;
	}
	else
	{
		rgba[0] = (*Settings::inactiveColor).red;
		rgba[1] = (*Settings::inactiveColor).green;
		rgba[2] = (*Settings::inactiveColor).blue;
		rgba[3] = (*Settings::inactiveColor).alpha;
	}

	const float BAR_WEIGHT = 0.935;
	const double DOT_RADIUS = h * 0.065;
	
	int orientation = Settings::indicatorOrientation;
	// Orientation based on panel mode and position
	// Mimics Windows 10 style, indicator stays on outside
	// TODO: make this a hidden setting now
	if (orientation == ORIENTATION_AUTOMATIC)
	{
		XfcePanelPluginMode panelMode = xfce_panel_plugin_get_mode(Plugin::mXfPlugin);
		XfceScreenPosition screenPosition = xfce_panel_plugin_get_screen_position(Plugin::mXfPlugin);

		if (panelMode == XFCE_PANEL_PLUGIN_MODE_VERTICAL || panelMode == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
		{
			if (xfce_screen_position_is_left(screenPosition))
				orientation = ORIENTATION_LEFT;
			else if (xfce_screen_position_is_right(screenPosition))
				orientation = ORIENTATION_RIGHT;
		}
		else
		{
			if (xfce_screen_position_is_top(screenPosition))
				orientation = ORIENTATION_TOP;
			else if (xfce_screen_position_is_bottom(screenPosition))
				orientation = ORIENTATION_BOTTOM;
		}
	}

	switch (Settings::indicatorStyle)
	{
	case STYLE_NONE:
		break;

	case STYLE_BARS:
	{
		if (mSOpened)
		{
			cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);

			if (orientation == ORIENTATION_BOTTOM)
				cairo_rectangle(cr, 0, round(h * BAR_WEIGHT), w, h - round(h * BAR_WEIGHT));
			else if (orientation == ORIENTATION_RIGHT)
				cairo_rectangle(cr, round(w * BAR_WEIGHT), 0, w - round(w * BAR_WEIGHT), h);
			else if (orientation == ORIENTATION_TOP)
				cairo_rectangle(cr, 0, 0, w, round(h * (1 - BAR_WEIGHT)));
			else if (orientation == ORIENTATION_LEFT)
				cairo_rectangle(cr, 0, 0, round(w * (1 - BAR_WEIGHT)), h);

			cairo_fill(cr);
		}

		if (mSMany && (mSOpened || mSHover))
		{
			int pat0;
			cairo_pattern_t* pat;

			if (orientation == ORIENTATION_BOTTOM || orientation == ORIENTATION_TOP)
			{
				pat0 = (int)w * 0.88;
				pat = cairo_pattern_create_linear(pat0, 0, w, 0);
			}
			else
			{
				pat0 = (int)h * 0.90;
				pat = cairo_pattern_create_linear(0, pat0, 0, h);
			}

			cairo_pattern_add_color_stop_rgba(pat, 0.0, 0, 0, 0, 0.45);
			cairo_pattern_add_color_stop_rgba(pat, 0.1, 0, 0, 0, 0.35);
			cairo_pattern_add_color_stop_rgba(pat, 0.3, 0, 0, 0, 0.15);

			if (orientation == ORIENTATION_BOTTOM)
				cairo_rectangle(cr, pat0, round(h * BAR_WEIGHT), w - pat0, round(h * (1 - BAR_WEIGHT)));
			else if (orientation == ORIENTATION_RIGHT)
				cairo_rectangle(cr, round(w * BAR_WEIGHT), pat0, round(w * (1 - BAR_WEIGHT)), h - pat0);
			else if (orientation == ORIENTATION_TOP)
				cairo_rectangle(cr, pat0, 0, w - pat0, round(h * (1 - BAR_WEIGHT)));
			else if (orientation == ORIENTATION_LEFT)
				cairo_rectangle(cr, 0, pat0, round(w * (1 - BAR_WEIGHT)), h - pat0);

			cairo_set_source(cr, pat);
			cairo_fill(cr);
			cairo_pattern_destroy(pat);
		}
		break;
	}

	case STYLE_DOTS:
	{
		if (mSOpened)
		{
			if (mSMany)
			{
				double x0, y0, x1, y1;

				if (orientation == ORIENTATION_BOTTOM)
				{
					x0 = (w / 2.) - DOT_RADIUS * 1.3;
					x1 = (w / 2.) + DOT_RADIUS * 1.3;
					y0 = y1 = h * 0.99;
				}
				else if (orientation == ORIENTATION_RIGHT)
				{
					y0 = (h / 2.) - DOT_RADIUS * 1.3;
					y1 = (h / 2.) + DOT_RADIUS * 1.3;
					x0 = x1 = w * 0.99;
				}
				else if (orientation == ORIENTATION_TOP)
				{
					x0 = (w / 2.) - DOT_RADIUS * 1.3;
					x1 = (w / 2.) + DOT_RADIUS * 1.3;
					y0 = y1 = h * 0.01;
				}
				else if (orientation == ORIENTATION_LEFT)
				{
					y0 = (h / 2.) - DOT_RADIUS * 1.3;
					y1 = (h / 2.) + DOT_RADIUS * 1.3;
					x0 = x1 = w * 0.01;
				}

				cairo_pattern_t* pat = cairo_pattern_create_radial(x0, y0, 0, x0, y0, DOT_RADIUS);
				cairo_pattern_add_color_stop_rgba(pat, 0.4, rgba[0], rgba[1], rgba[2], rgba[3]);
				cairo_pattern_add_color_stop_rgba(pat, 1, rgba[0], rgba[1], rgba[2], rgba[3]);
				cairo_set_source(cr, pat);

				cairo_arc(cr, x0, y0, DOT_RADIUS, 0.0, 2.0 * M_PI);
				cairo_fill(cr);

				cairo_pattern_destroy(pat);

				pat = cairo_pattern_create_radial(x1, y1, 0, x1, y1, DOT_RADIUS);
				cairo_pattern_add_color_stop_rgba(pat, 0.4, rgba[0], rgba[1], rgba[2], rgba[3]);
				cairo_pattern_add_color_stop_rgba(pat, 1, rgba[0], rgba[1], rgba[2], rgba[3]);
				cairo_set_source(cr, pat);

				cairo_arc(cr, x1, y1, DOT_RADIUS, 0.0, 2.0 * M_PI);
				cairo_fill(cr);

				cairo_pattern_destroy(pat);
			}
			else
			{
				double x, y;

				if (orientation == ORIENTATION_BOTTOM)
				{
					x = (w / 2.);
					y = h * 0.99;
				}
				else if (orientation == ORIENTATION_RIGHT)
				{
					x = w * 0.99;
					y = (h / 2.);
				}
				else if (orientation == ORIENTATION_TOP)
				{
					x = (w / 2.);
					y = h * 0.01;
				}
				else if (orientation == ORIENTATION_LEFT)
				{
					x = w * 0.01;
					y = (h / 2.);
				}

				cairo_pattern_t* pat = cairo_pattern_create_radial(x, y, 0, x, y, DOT_RADIUS);
				cairo_pattern_add_color_stop_rgba(pat, 0.4, rgba[0], rgba[1], rgba[2], rgba[3]);
				cairo_pattern_add_color_stop_rgba(pat, 1, rgba[0], rgba[1], rgba[2], rgba[3]);
				cairo_set_source(cr, pat);

				cairo_arc(cr, x, y, DOT_RADIUS, 0.0, 2.0 * M_PI);
				cairo_fill(cr);

				cairo_pattern_destroy(pat);
			}
		}
		break;
	}

	case STYLE_RECTS:
	{
		if (mSOpened)
		{
			int vw;

			if (orientation == ORIENTATION_BOTTOM || orientation == ORIENTATION_TOP)
				vw = w;
			else
				vw = h;

			if (mSMany)
			{
				int space = floor(vw / 4.5);
				int sep = vw / 11.;
				sep = std::max(sep - (sep % 2) + (vw % 2), 2);

				cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);

				if (orientation == ORIENTATION_BOTTOM)
				{
					cairo_rectangle(cr, w / 2. - sep / 2. - space, round(h * BAR_WEIGHT), space, round(h * (1 - BAR_WEIGHT)));
					cairo_rectangle(cr, w / 2. + sep / 2., round(h * BAR_WEIGHT), space, round(h * (1 - BAR_WEIGHT)));
				}
				else if (orientation == ORIENTATION_RIGHT)
				{
					cairo_rectangle(cr, round(w * BAR_WEIGHT), h / 2. - sep / 2. - space, round(w * (1 - BAR_WEIGHT)), space);
					cairo_rectangle(cr, round(w * BAR_WEIGHT), h / 2. + sep / 2., round(w * (1 - BAR_WEIGHT)), space);
				}
				else if (orientation == ORIENTATION_TOP)
				{
					cairo_rectangle(cr, w / 2. - sep / 2. - space, 0, space, round(h * (1 - BAR_WEIGHT)));
					cairo_rectangle(cr, w / 2. + sep / 2., 0, space, round(h * (1 - BAR_WEIGHT)));
				}
				else if (orientation == ORIENTATION_LEFT)
				{
					cairo_rectangle(cr, 0, h / 2. - sep / 2. - space, round(w * (1 - BAR_WEIGHT)), space);
					cairo_rectangle(cr, 0, h / 2. + sep / 2., round(w * (1 - BAR_WEIGHT)), space);
				}

				cairo_fill(cr);
			}
			else
			{
				int space = floor(vw / 4.5);
				space = space + (space % 2) + (vw % 2);
				int start = (vw - space) / 2;

				cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);

				if (orientation == ORIENTATION_BOTTOM)
					cairo_rectangle(cr, start, round(h * BAR_WEIGHT), space, round(h * (1 - BAR_WEIGHT)));
				else if (orientation == ORIENTATION_RIGHT)
					cairo_rectangle(cr, round(w * BAR_WEIGHT), start, round(w * (1 - BAR_WEIGHT)), space);
				else if (orientation == ORIENTATION_TOP)
					cairo_rectangle(cr, start, 0, space, round(h * (1 - BAR_WEIGHT)));
				else if (orientation == ORIENTATION_LEFT)
					cairo_rectangle(cr, 0, start, round(w * (1 - BAR_WEIGHT)), space);

				cairo_fill(cr);
			}
		}
		break;
	}
	}
}

void Group::onMouseEnter()
{
	Dock::mGroups.forEach([this](std::pair<AppInfo*, Group*> g) -> void {
		if (&(g.second->mGroupMenu) != &(this->mGroupMenu))
			g.second->mGroupMenu.mGroup->onMouseLeave();
	});

	mGroupMenu.popup();
	mSHover = true;
}

void Group::onMouseLeave()
{
	if (!mGroupMenu.mMouseHover)
		mGroupMenu.hide();
}

void Group::setMouseLeaveTimeout()
{
	mTolerablePointerDistance = 200;
	mLeaveTimeout.start();
}

void Group::updateStyle()
{
	int wCount = mWindowsCount;

	if (mPinned || wCount)
		gtk_widget_show_all(mButton);
	else
		gtk_widget_hide(mButton);

	if (wCount)
	{
		if (wCount == 1 && Settings::noWindowsListIfSingle)
			gtk_widget_set_tooltip_text(mButton, mAppInfo->name.c_str());
		else
			gtk_widget_set_tooltip_text(mButton, NULL);

		mSOpened = true;

		if (wCount > 1)
			mSMany = true;
		else
			mSMany = false;

		if (wCount > 2 && Settings::showWindowCount)
			gtk_label_set_markup(GTK_LABEL(mLabel), g_strdup_printf("<b>%d</b>", wCount));
		else
			gtk_label_set_markup(GTK_LABEL(mLabel), "");
	}
	else
	{
		gtk_widget_set_tooltip_text(mButton, mAppInfo->name.c_str());
		mSOpened = false;
		mSFocus = false;
	}
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
		mSFocus = true;
		setTopWindow(groupWindow);
		Help::Gtk::cssClassAdd(GTK_WIDGET(mButton), "active_group");
	}
}

void Group::onWindowUnactivate()
{
	mActive = false;
	mSFocus = false;
	Help::Gtk::cssClassRemove(GTK_WIDGET(mButton), "active_group");
}

void Group::setTopWindow(GroupWindow* groupWindow)
{
	mTopWindowIndex = mWindows.getIndex(groupWindow);
}

void Group::onButtonPress(GdkEventButton* event)
{
	if (event->button == GDK_BUTTON_SECONDARY)
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
	if (event->button == GDK_BUTTON_MIDDLE)
		closeAll();
	else if (event->state & GDK_SHIFT_MASK || (mPinned && !mWindowsCount))
		mAppInfo->launch();
	else if (mActive)
		mWindows.get(mTopWindowIndex)->minimize();
	else
		activate(event->time);
}

bool Group::onDragMotion(GtkWidget* widget, GdkDragContext* context, int x, int y, guint time)
{
	GdkModifierType mask;
	GdkDevice* device = gdk_drag_context_get_device(context);
	gdk_window_get_device_position(gtk_widget_get_window(widget), device, NULL, NULL, &mask);

	if (mask & GDK_CONTROL_MASK)
		gtk_drag_cancel(context);

	GList* tmp_list = gdk_drag_context_list_targets(context);

	if (tmp_list != NULL)
	{
		std::string target = gdk_atom_name(GDK_POINTER_TO_ATOM(tmp_list->data));

		if (target != "application/docklike_group")
		{
			if (mWindowsCount)
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

	gtk_drag_highlight(mButton);
	gdk_drag_status(context, GDK_ACTION_MOVE, time);
	return true;
}

void Group::onDragLeave(const GdkDragContext* context, guint time)
{
	gtk_drag_unhighlight(mButton);
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

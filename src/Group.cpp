/*
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Group.hpp"

static GtkTargetEntry entries[1] = {{(gchar*)"application/docklike_group", 0, 0}};
static GtkTargetList* targetList = gtk_target_list_new(entries, 1);

Group::Group(std::shared_ptr<AppInfo> appInfo, bool pinned) : mGroupMenu(this)
{
	mIconPixbuf = nullptr;
	mContextMenu = nullptr;
	mAppInfo = appInfo;
	mPinned = pinned;
	mTopWindowIndex = 0;
	mActive = false;
	mWindowMenuShown = false;

	//--------------------------------------------------

	mWindowsCount.setup(
		0,
		[this]() -> uint {
			uint count = 0;
			
			mWindows.findIf([&count](GroupWindow* e) -> bool {
				if (!e->getState(XFW_WINDOW_STATE_SKIP_TASKLIST))
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

	mButton = GTK_WIDGET(g_object_ref(gtk_button_new()));
	mImage = gtk_image_new();
	mLabel = gtk_label_new("");
	GtkWidget* overlay = gtk_overlay_new();

	// The button contains a GtkOverlay, so that the label can be placed on top of the image.
	gtk_label_set_use_markup(GTK_LABEL(mLabel), true);
	gtk_container_add(GTK_CONTAINER(overlay), mImage);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), mLabel);
	gtk_widget_set_halign(mLabel, GTK_ALIGN_START);
	gtk_widget_set_valign(mLabel, GTK_ALIGN_START);
	gtk_overlay_set_overlay_pass_through(GTK_OVERLAY(overlay), mLabel, true);
	gtk_container_add(GTK_CONTAINER(mButton), overlay);

	Help::Gtk::cssClassAdd(mButton, "flat");
	Help::Gtk::cssClassAdd(mButton, "group");
	Help::Gtk::cssClassAdd(mLabel, "window_count");

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
			me->mLeaveTimeout.stop();
			me->mMenuShowTimeout.start();

			if (Settings::showPreviews)
				me->mWindows.forEach([](GroupWindow* w) -> void {
					w->mGroupMenuItem->mPreviewTimeout.start();
				});

			return false;
		}),
		this);

	g_signal_connect(G_OBJECT(mButton), "leave-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventCrossing* event, Group* me) {
			Help::Gtk::cssClassRemove(me->mButton, "hover_group");
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

	g_signal_connect_after(G_OBJECT(mButton), "draw",
		G_CALLBACK(+[](GtkWidget* widget, cairo_t* cr, Group* me) {
			me->onDraw(cr);
			return false;
		}),
		this);

	//--------------------------------------------------

	if (mPinned)
		gtk_widget_show_all(mButton);

	if (mAppInfo != nullptr && !mAppInfo->mIcon.empty())
	{
		if (mAppInfo->mIcon[0] == '/' && g_file_test(mAppInfo->mIcon.c_str(), G_FILE_TEST_IS_REGULAR))
			mIconPixbuf = gdk_pixbuf_new_from_file(mAppInfo->mIcon.c_str(), nullptr);
		else
			gtk_image_set_from_icon_name(GTK_IMAGE(mImage), mAppInfo->mIcon.c_str(), GTK_ICON_SIZE_BUTTON);
	}
	else
		gtk_image_set_from_icon_name(GTK_IMAGE(mImage), "application-x-executable", GTK_ICON_SIZE_BUTTON);

	resize();
	updateStyle();
}

Group::~Group()
{
	mLeaveTimeout.stop();
	mMenuShowTimeout.stop();

	// can be unparented before the group is destroyed on exit
	if (gtk_widget_get_parent(mButton) != nullptr)
		gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(mButton)), mButton);
	g_object_unref(mButton);

	if (mIconPixbuf != nullptr)
		g_object_unref(mIconPixbuf);
}

void Group::add(GroupWindow* window)
{
	mWindows.push(window);
	mWindowsCount.updateState();
	mGroupMenu.add(window->mGroupMenuItem);
	Help::Gtk::cssClassAdd(mButton, "open_group");

	if (mWindowsCount == 1 && !mPinned)
		gtk_box_reorder_child(GTK_BOX(Dock::mBox), mButton, -1);

	if (!mActive && xfw_window_is_active(window->mXfwWindow))
		onWindowActivate(window);

	gtk_widget_queue_draw(mButton);
}

void Group::remove(GroupWindow* window)
{
	mWindows.pop(window);
	mWindowsCount.updateState();
	mGroupMenu.remove(window->mGroupMenuItem);

	if (mTopWindowIndex >= mWindowsCount)
		mTopWindowIndex = 0;

	if (!mWindowsCount)
		Help::Gtk::cssClassRemove(mButton, "open_group");

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
			mTopWindowIndex = (mTopWindowIndex + 1) % mWindows.size();
		else if (direction == GDK_SCROLL_DOWN)
		{
			int size = mWindows.size();
			mTopWindowIndex = (mTopWindowIndex - 1 + size) % size;
		}
		mWindows.get(mTopWindowIndex)->activate(timestamp);
	}
}

void Group::closeAll()
{
	mWindows.forEach([](GroupWindow* w) -> void {
		if (!w->getState(XFW_WINDOW_STATE_SKIP_TASKLIST))
			Xfw::close(w, 0);
	});
}

void Group::resize()
{
	// TODO: set `min-width` / `min-height` CSS property on button?
	// https://github.com/davekeogh/xfce4-docklike-plugin/issues/39

	// may not yet be initialized
	if (Dock::mIconSize == 0)
		return;

	if (mIconPixbuf != nullptr)
	{
		gint scale_factor = gtk_widget_get_scale_factor(mButton);
		gint size = Dock::mIconSize * scale_factor;
		GdkPixbuf* scaled = gdk_pixbuf_scale_simple(mIconPixbuf, size, size, GDK_INTERP_BILINEAR);
		cairo_surface_t* surface = gdk_cairo_surface_create_from_pixbuf(scaled, scale_factor, nullptr);
		gtk_image_set_from_surface(GTK_IMAGE(mImage), surface);
		cairo_surface_destroy(surface);
		g_object_unref(scaled);
	}
	else
		gtk_image_set_pixel_size(GTK_IMAGE(mImage), Dock::mIconSize);

	gtk_widget_set_valign(mImage, GTK_ALIGN_CENTER);
	gtk_widget_queue_draw(mButton);
}

void Group::onDraw(cairo_t* cr)
{
	int w = gtk_widget_get_allocated_width(mButton);
	int h = gtk_widget_get_allocated_height(mButton);

	double rgba[4];

	if (Settings::indicatorColorFromTheme)
	{
		GtkWidget* menu = GTK_WIDGET(g_object_ref_sink(gtk_menu_new()));
		GtkStyleContext* sc = gtk_widget_get_style_context(menu);

		GValue gv = G_VALUE_INIT;
		gtk_style_context_get_property(sc, "color", GTK_STATE_FLAG_NORMAL, &gv);
		GdkRGBA* indicatorColor = (GdkRGBA*)g_value_get_boxed(&gv);

		g_object_unref(menu);

		rgba[0] = (*indicatorColor).red;
		rgba[1] = (*indicatorColor).green;
		rgba[2] = (*indicatorColor).blue;
		rgba[3] = (*indicatorColor).alpha;
		g_value_unset(&gv);
	}
	else
	{
		std::shared_ptr<GdkRGBA> color;
		if (mActive)
			color = Settings::indicatorColor;
		else
			color = Settings::inactiveColor;

		rgba[0] = color->red;
		rgba[1] = color->green;
		rgba[2] = color->blue;
		rgba[3] = color->alpha;
	}

	const float BAR_WEIGHT = 0.935;
	const double DOT_RADIUS = h * 0.065;
	const double CIRCLE_WEIGHT = 0.0375;

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

	int indicator_style = Settings::inactiveIndicatorStyle;
	if (mActive)
		indicator_style = Settings::indicatorStyle;

	switch (indicator_style)
	{
	case STYLE_NONE:
		break;

	case STYLE_BARS:
	{
		if (mWindowsCount > 0)
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

		if (mWindowsCount > 1)
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

	case STYLE_CILIORA:
	{
		if (mWindowsCount > 0)
		{
			int offset;
			cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);

			offset = 0;

			if (mWindowsCount > 1)
			{
				if (orientation == ORIENTATION_BOTTOM || orientation == ORIENTATION_TOP)
				{
					offset = 2 * (round(h * (1 - BAR_WEIGHT)));
				}
				else
				{
					offset = (2 * (round(w * (1 - BAR_WEIGHT))));
				}
			}

			if (orientation == ORIENTATION_BOTTOM)
				cairo_rectangle(cr, 0, round(h * BAR_WEIGHT), w - offset, round(h * (1 - BAR_WEIGHT)));
			else if (orientation == ORIENTATION_RIGHT)
				cairo_rectangle(cr, round(w * BAR_WEIGHT), 0, round(w * (1 - BAR_WEIGHT)), h - offset);
			else if (orientation == ORIENTATION_TOP)
				cairo_rectangle(cr, 0, 0, w - offset, round(h * (1 - BAR_WEIGHT)));
			else if (orientation == ORIENTATION_LEFT)
				cairo_rectangle(cr, 0, 0, round(w * (1 - BAR_WEIGHT)), h - offset);

			cairo_fill(cr);
		}

		if (mWindowsCount > 1)
		{
			int size;
			cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);

			if (orientation == ORIENTATION_BOTTOM || orientation == ORIENTATION_TOP)
			{
				size = round(h * (1 - BAR_WEIGHT));
			}
			else
			{
				size = round(w * (1 - BAR_WEIGHT));
			}

			if (orientation == ORIENTATION_BOTTOM)
				cairo_rectangle(cr, w - size, round(h * BAR_WEIGHT), size, size);
			else if (orientation == ORIENTATION_RIGHT)
				cairo_rectangle(cr, round(w * BAR_WEIGHT), h - size, size, size);
			else if (orientation == ORIENTATION_TOP)
				cairo_rectangle(cr, w - size, 0, size, size);
			else if (orientation == ORIENTATION_LEFT)
				cairo_rectangle(cr, 0, h - size, size, size);

			cairo_fill(cr);
		}
		break;
	}

	case STYLE_CIRCLES:
	{
		if (mWindowsCount > 0)
		{
			if (mWindowsCount > 1)
			{
				double x0 = 0, y0 = 0, x1 = 0, y1 = 0, radius = 0;

				if (orientation == ORIENTATION_BOTTOM)
				{
					radius = h * CIRCLE_WEIGHT;
					x0 = (w / 2.) - radius * 1.5;
					x1 = (w / 2.) + radius * 1.5;
					y0 = y1 = h - radius;
				}
				else if (orientation == ORIENTATION_RIGHT)
				{
					radius = w * CIRCLE_WEIGHT;
					y0 = (h / 2.) - radius * 1.5;
					y1 = (h / 2.) + radius * 1.5;
					x0 = x1 = w - radius;
				}
				else if (orientation == ORIENTATION_TOP)
				{
					radius = h * CIRCLE_WEIGHT;
					x0 = (w / 2.) - radius * 1.5;
					x1 = (w / 2.) + radius * 1.5;
					y0 = y1 = radius;
				}
				else if (orientation == ORIENTATION_LEFT)
				{
					radius = w * CIRCLE_WEIGHT;
					y0 = (h / 2.) - radius * 1.5;
					y1 = (h / 2.) + radius * 1.5;
					x0 = x1 = radius;
				}

				cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);

				cairo_arc(cr, x0, y0, radius, 0.0, 2.0 * M_PI);
				cairo_fill(cr);

				cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);

				cairo_arc(cr, x1, y1, radius, 0.0, 2.0 * M_PI);
				cairo_fill(cr);
			}
			else
			{
				double x = 0, y = 0, radius = 0;

				if (orientation == ORIENTATION_BOTTOM)
				{
					radius = h * CIRCLE_WEIGHT;
					x = (w / 2.);
					y = h - radius;
				}
				else if (orientation == ORIENTATION_RIGHT)
				{
					radius = w * CIRCLE_WEIGHT;
					x = w - radius;
					y = (h / 2.);
				}
				else if (orientation == ORIENTATION_TOP)
				{
					radius = h * CIRCLE_WEIGHT;
					x = (w / 2.);
					y = radius;
				}
				else if (orientation == ORIENTATION_LEFT)
				{
					radius = w * CIRCLE_WEIGHT;
					x = radius;
					y = (h / 2.);
				}

				cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);

				cairo_arc(cr, x, y, radius, 0.0, 2.0 * M_PI);
				cairo_fill(cr);
			}
		}
		break;
	}

	case STYLE_DOTS:
	{
		if (mWindowsCount > 0)
		{
			if (mWindowsCount > 1)
			{
				double x0 = 0, y0 = 0, x1 = 0, y1 = 0;

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
				double x = 0, y = 0;

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
		if (mWindowsCount > 0)
		{
			int vw;

			if (orientation == ORIENTATION_BOTTOM || orientation == ORIENTATION_TOP)
				vw = w;
			else
				vw = h;

			if (mWindowsCount > 1)
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
	Dock::mGroups.forEach([this](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> g) -> void {
		if (&(g.second->mGroupMenu) != &(this->mGroupMenu))
			g.second->mGroupMenu.mGroup->onMouseLeave();
	});

	if (mContextMenu == nullptr)
		mGroupMenu.popup();
}

void Group::onMouseLeave()
{
	if (!mGroupMenu.mMouseHover && !mWindowMenuShown)
		mGroupMenu.hide();
}

void Group::setMouseLeaveTimeout()
{
	mTolerablePointerDistance = 200;
	mLeaveTimeout.start();
}

void Group::updateStyle()
{
	if (mPinned || mWindowsCount > 0)
		gtk_widget_show_all(mButton);
	else
		gtk_widget_hide(mButton);

	if (mWindowsCount > 0)
	{
		if (mWindowsCount == 1 && Settings::noWindowsListIfSingle)
		{
			const gchar* tooltip = mAppInfo->mName.c_str();
			GroupWindow* gw = mWindows.get(0);
			if (gw && gw->mXfwWindow)
			{
				const gchar* n = xfw_window_get_name(gw->mXfwWindow);
				if (n && *n) tooltip = n;
			}
			gtk_widget_set_tooltip_text(mButton, tooltip);
		}
		else
			gtk_widget_set_tooltip_text(mButton, nullptr);

		if (mWindowsCount > 2 && Settings::showWindowCount)
		{
			gchar* markup = g_strdup_printf("<b>%d</b>", (int)mWindowsCount);
			gtk_label_set_markup(GTK_LABEL(mLabel), markup);
			g_free(markup);
		}
		else
			gtk_label_set_markup(GTK_LABEL(mLabel), "");
	}
	else
	{
		gtk_widget_set_tooltip_text(mButton, mAppInfo->mName.c_str());
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
			newTopWindow = (Xfw::mGroupWindows.findIf([this](std::pair<XfwWindow*, std::shared_ptr<GroupWindow>> e) -> bool {
				if (e.second->mGroup == this)
					return true;
				return false;
			})).get();

		setTopWindow(newTopWindow);
	}
}

void Group::onWindowActivate(GroupWindow* groupWindow)
{
	mActive = true;
	setTopWindow(groupWindow);
	Help::Gtk::cssClassAdd(mButton, "active_group");
}

void Group::onWindowUnactivate()
{
	mActive = false;
	Help::Gtk::cssClassRemove(mButton, "active_group");
}

void Group::setTopWindow(GroupWindow* groupWindow)
{
	mTopWindowIndex = mWindows.getIndex(groupWindow);
}

GtkWidget* Group::buildContextMenu()
{
	GtkWidget* menu = gtk_menu_new();

	if (mAppInfo->mGAppInfo != nullptr)
	{
		GtkWidget* item = gtk_check_menu_item_new_with_label(mPinned ? _("Pinned to Dock") : _("Pin to Dock"));
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), mPinned);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "toggled",
			G_CALLBACK(+[](GtkCheckMenuItem* _item, Group* group) {
				group->mPinned = !group->mPinned;
				if (!group->mPinned)
					group->updateStyle();
				Dock::savePinned();
			}),
			this);

		item = gtk_menu_item_new_with_label(_("Edit Launcher..."));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(+[](GtkMenuItem* _item, AppInfo* appInfo) {
				appInfo->edit();
			}),
			mAppInfo.get());

		const gchar* const* actions = mAppInfo->getActions();
		if (actions[0] != nullptr)
		{
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			for (int i = 0; actions[i] != nullptr; i++)
			{
				gchar* action_name = g_desktop_app_info_get_action_name(mAppInfo->mGAppInfo.get(), actions[i]);
				item = gtk_menu_item_new_with_label(action_name);
				g_free(action_name);
				g_object_set_data((GObject*)item, "action", (gpointer)actions[i]);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate",
					G_CALLBACK(+[](GtkMenuItem* _item, AppInfo* appInfo) {
						appInfo->launchAction((const gchar*)g_object_get_data((GObject*)_item, "action"));
					}),
					mAppInfo.get());
			}
		}

		item = gtk_menu_item_new_with_label(_("Close All"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
			G_CALLBACK(+[](GtkMenuItem* _item, Group* group) {
				group->closeAll();
			}),
			this);
	}
	else
	{
		if (mPinned)
		{
			// when a pinned app loses its icon (typically the app is uninstalled) the user may want to remove it from the dock
			GtkWidget* item = gtk_menu_item_new_with_label(_("Remove"));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(+[](GtkMenuItem* _item, Group* group) {
					group->mPinned = false;
					Dock::savePinned();
					Dock::drawGroups();
				}),
				this);
		}
		else
		{
			// we weren't able to find a launcher for this app, let the user set it manually
			GtkWidget* item = gtk_menu_item_new_with_label(_("Select Launcher..."));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(+[](GtkMenuItem* _item, const gchar* classId) {
					if (AppInfos::selectLauncher(classId))
						Dock::drawGroups();
				}),
				(gpointer)mAppInfo->mName.c_str());

			item = gtk_menu_item_new_with_label(_("Create Launcher..."));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(+[](GtkMenuItem* _item, const gchar* classId) {
					AppInfos::createLauncher(classId);
				}),
				(gpointer)mAppInfo->mName.c_str());
		}
	}

	gtk_widget_show_all(menu);

	return menu;
}

void Group::onButtonPress(GdkEventButton* event)
{
	if (event->button == GDK_BUTTON_SECONDARY
		&& mButton != nullptr
		&& (mWindowsCount > 0 || mPinned))
	{
		mContextMenu = GTK_WIDGET(g_object_ref_sink(buildContextMenu()));
		xfce_panel_plugin_register_menu(Plugin::mXfPlugin, GTK_MENU(mContextMenu));
		g_signal_connect_swapped(mContextMenu, "deactivate", G_CALLBACK(g_clear_object), &mContextMenu);
		gtk_menu_popup_at_widget(GTK_MENU(mContextMenu), mButton, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, (GdkEvent*)event);
		mGroupMenu.hide();
	}
}

void Group::onButtonRelease(GdkEventButton* event)
{
	if (event->button == GDK_BUTTON_MIDDLE)
	{
		switch (Settings::middleButtonBehavior)
		{
		case BEHAVIOR_CLOSE_ALL:
		{
			closeAll();
			break;
		}
		case BEHAVIOR_LAUNCH_NEW:
		{
			mAppInfo->launch();
			break;
		}
		case BEHAVIOR_DO_NOTHING:
		{
			break;
		}
		}
	}
	else if (event->state & GDK_SHIFT_MASK || (mPinned && !mWindowsCount))
		mAppInfo->launch();
	else if (mActive)
	{
		mWindows.forEach([](GroupWindow* w) -> void {
			if (!w->getState(XFW_WINDOW_STATE_MINIMIZED))
				w->minimize();
		});
	}
	else
		activate(event->time);
}

bool Group::onDragMotion(GtkWidget* widget, GdkDragContext* context, int x, int y, guint time)
{
	GdkModifierType mask;
	GdkDevice* device = gdk_drag_context_get_device(context);
	gdk_window_get_device_position(gtk_widget_get_window(widget), device, nullptr, nullptr, &mask);

	if (mask & GDK_CONTROL_MASK)
		gtk_drag_cancel(context);

	GList* tmp_list = gdk_drag_context_list_targets(context);

	if (tmp_list != nullptr)
	{
		gchar* name = gdk_atom_name(GDK_POINTER_TO_ATOM(tmp_list->data));
		std::string target = name;
		g_free(name);

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
	Dock::moveButton((Group*)(gpointer)gtk_selection_data_get_data(selectionData), this);
}

void Group::onDragBegin(GdkDragContext* context)
{
	if (mIconPixbuf != nullptr)
	{
		gint scale_factor = gtk_widget_get_scale_factor(mButton);
		gint size;
		if (!gtk_icon_size_lookup(GTK_ICON_SIZE_DND, &size, nullptr))
			size = 32;
		size *= scale_factor;
		GdkPixbuf* scaled = gdk_pixbuf_scale_simple(mIconPixbuf, size, size, GDK_INTERP_BILINEAR);
		cairo_surface_t* surface = gdk_cairo_surface_create_from_pixbuf(scaled, scale_factor, nullptr);
		gtk_drag_set_icon_surface(context, surface);
		cairo_surface_destroy(surface);
		g_object_unref(scaled);
	}
	else
	{
		const gchar* icon_name;
		gtk_image_get_icon_name(GTK_IMAGE(mImage), &icon_name, nullptr);
		gtk_drag_set_icon_name(context, icon_name, 0, 0);
	}
}

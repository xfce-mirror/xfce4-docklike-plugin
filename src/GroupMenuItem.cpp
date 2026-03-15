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

#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#include <libxfce4windowing/xfw-x11.h>
#endif

#include "GroupMenuItem.hpp"

static GtkTargetEntry entries[1] = {{(gchar*)"any", 0, 0}};

GroupMenuItem::GroupMenuItem(GroupWindow* groupWindow)
{
	mGroupWindow = groupWindow;

	// This needs work to survive porting to GTK4.
	// GtkEventBox is removed, all events are supported by all widgets.

	mItem = GTK_EVENT_BOX(gtk_event_box_new());
	gtk_drag_dest_set(GTK_WIDGET(mItem), GTK_DEST_DEFAULT_DROP, entries, 1, GDK_ACTION_MOVE);
	Help::Gtk::cssClassAdd(GTK_WIDGET(mItem), "menu_item");
	gtk_widget_show(GTK_WIDGET(mItem));
	g_object_ref(mItem);

	mGrid = GTK_GRID(gtk_grid_new());
	gtk_grid_set_column_spacing(mGrid, 6);
	gtk_widget_show(GTK_WIDGET(mGrid));
	gtk_container_add(GTK_CONTAINER(mItem), GTK_WIDGET(mGrid));

	mIcon = GTK_IMAGE(gtk_image_new());
	Help::Gtk::cssClassAdd(GTK_WIDGET(mIcon), "icon");
	gtk_widget_set_halign(GTK_WIDGET(mIcon), GTK_ALIGN_START);
	gtk_widget_set_hexpand(GTK_WIDGET(mIcon), false);
	gtk_widget_show(GTK_WIDGET(mIcon));
	gtk_grid_attach(mGrid, GTK_WIDGET(mIcon), 0, 0, 1, 1);

	mLabel = GTK_LABEL(gtk_label_new(""));
	Help::Gtk::cssClassAdd(GTK_WIDGET(mLabel), "title");
	gtk_label_set_xalign(mLabel, 0);
	gtk_label_set_ellipsize(mLabel, PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars(mLabel, 26);
	gtk_widget_set_hexpand(GTK_WIDGET(mLabel), true);
	gtk_widget_show(GTK_WIDGET(mLabel));
	gtk_grid_attach(mGrid, GTK_WIDGET(mLabel), 1, 0, 1, 1);

	mCloseButton = GTK_BUTTON(gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(mCloseButton, GTK_RELIEF_NONE);
	gtk_widget_set_halign(GTK_WIDGET(mCloseButton), GTK_ALIGN_END);
	gtk_widget_set_hexpand(GTK_WIDGET(mCloseButton), false);
	gtk_widget_show(GTK_WIDGET(mCloseButton));
	gtk_grid_attach(mGrid, GTK_WIDGET(mCloseButton), 2, 0, 1, 1);

	mPreview = GTK_IMAGE(gtk_image_new());
	Help::Gtk::cssClassAdd(GTK_WIDGET(mPreview), "preview");
	gtk_grid_attach(mGrid, GTK_WIDGET(mPreview), 0, 1, 3, 1);
	gtk_widget_set_visible(GTK_WIDGET(mPreview), Settings::showPreviews);

	if (Xfw::getActiveWindow() == mGroupWindow->mXfwWindow)
		Help::Gtk::cssClassAdd(GTK_WIDGET(mItem), "active_menu_item");

	int sleepMS = 1000;
	if (Settings::previewSleep)
		sleepMS = Settings::previewSleep;

	mPreviewTimeout.setup(sleepMS, [this]() {
		updatePreview();
		return true;
	});

	//--------------------------------------------------

	g_signal_connect(G_OBJECT(mItem), "button-press-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, GroupMenuItem* me) {
			if (event->button == GDK_BUTTON_PRIMARY)
				me->mGroupWindow->activate((event)->time);
			else if (event->button == GDK_BUTTON_MIDDLE && Settings::middleButtonBehavior == BEHAVIOR_CLOSE_ALL)
				Xfw::close(me->mGroupWindow, event->time);
			else if (event->button == GDK_BUTTON_SECONDARY)
			{
				GtkWidget* menu = GTK_WIDGET(g_object_ref_sink(xfw_window_action_menu_new(me->mGroupWindow->mXfwWindow)));
				xfce_panel_plugin_register_menu(Plugin::mXfPlugin, GTK_MENU(menu));
				g_signal_connect(G_OBJECT(menu), "deactivate",
					G_CALLBACK(+[](GtkMenuShell* _menu, GroupMenuItem* _me) {
						g_object_unref(_menu);
						_me->mGroupWindow->mGroup->mWindowMenuShown = false;
						_me->mGroupWindow->mGroup->setMouseLeaveTimeout();
					}),
					me);
				gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
				me->mGroupWindow->mGroup->mWindowMenuShown = true;
			}
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mItem), "enter-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventCrossing* event, GroupMenuItem* me) {
			if (event->state & GDK_BUTTON1_MASK)
				me->mGroupWindow->activate(event->time);
			Help::Gtk::cssClassAdd(widget, "hover_menu_item");
			gtk_widget_queue_draw(widget);
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mItem), "leave-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEvent* event, GroupMenuItem* me) {
			Help::Gtk::cssClassRemove(widget, "hover_menu_item");
			gtk_widget_queue_draw(widget);
			gtk_widget_queue_draw(me->mGroupWindow->mGroup->mButton);
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mCloseButton), "clicked",
		G_CALLBACK(+[](GtkButton* button, GroupMenuItem* me) {
			Xfw::close(me->mGroupWindow, 0);
		}),
		this);
}

GroupMenuItem::~GroupMenuItem()
{
	mPreviewTimeout.stop();
	g_object_unref(mItem);
}

void GroupMenuItem::updateLabel()
{
	const char* winName = xfw_window_get_name(mGroupWindow->mXfwWindow);

	if (Xfw::getActiveWindow() == mGroupWindow->mXfwWindow)
	{
		gchar* escaped = g_markup_escape_text(winName, -1);
		gchar* markup = g_strdup_printf("<b>%s</b>", escaped);
		gtk_label_set_markup(mLabel, markup);
		g_free(markup);
		g_free(escaped);
	}
	else if (mGroupWindow->getState(XFW_WINDOW_STATE_MINIMIZED))
	{
		gchar* escaped = g_markup_escape_text(winName, -1);
		gchar* markup = g_strdup_printf("<i>%s</i>", escaped);
		gtk_label_set_markup(mLabel, markup);
		g_free(markup);
		g_free(escaped);
	}
	else
		gtk_label_set_text(mLabel, winName);
}

void GroupMenuItem::updateIcon()
{
	gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(mIcon));
	// Use same theme icon as the dock button (from .desktop file)
	std::shared_ptr<AppInfo> appInfo = mGroupWindow->mGroup->mAppInfo;
	if (appInfo != nullptr && !appInfo->mIcon.empty())
	{
		if (appInfo->mIcon[0] == '/' && g_file_test(appInfo->mIcon.c_str(), G_FILE_TEST_IS_REGULAR))
		{
			GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(appInfo->mIcon.c_str(), 16, 16, nullptr);
			if (pixbuf != nullptr)
			{
				cairo_surface_t* surface = gdk_cairo_surface_create_from_pixbuf(pixbuf, scale_factor, nullptr);
				gtk_image_set_from_surface(mIcon, surface);
				cairo_surface_destroy(surface);
				g_object_unref(pixbuf);
				return;
			}
		}
		else
		{
			gtk_image_set_from_icon_name(GTK_IMAGE(mIcon), appInfo->mIcon.c_str(), GTK_ICON_SIZE_LARGE_TOOLBAR);
			gtk_image_set_pixel_size(GTK_IMAGE(mIcon), 16);
			return;
		}
	}
	// Fallback to window icon if no .desktop icon found
	GdkPixbuf* iconPixbuf = xfw_window_get_icon(mGroupWindow->mXfwWindow, 16, scale_factor);
	if (iconPixbuf != nullptr)
	{
		cairo_surface_t* surface = gdk_cairo_surface_create_from_pixbuf(iconPixbuf, scale_factor, nullptr);
		gtk_image_set_from_surface(mIcon, surface);
		cairo_surface_destroy(surface);
	}
}

void GroupMenuItem::updatePreview()
{
	if (mGroupWindow->getState(XFW_WINDOW_STATE_MINIMIZED))
		return; // minimized windows never need a new thumbnail

#ifdef ENABLE_X11
	// This needs work to survive porting to GTK4 and/or Wayland.
	// GDK doesn't expose an API to get a foreign window on X11 anymore (so X11 code).
	// Wayland may never have a public protocol for obtaining a preview of a foreign window,
	// which goes against its security principles (so private protocol).
	if (GDK_IS_X11_DISPLAY(Plugin::mDisplay))
	{
		GdkWindow* window;
		GdkPixbuf* pixbuf;
		GdkPixbuf* thumbnail;

		window = gdk_x11_window_foreign_new_for_display(Plugin::mDisplay,
			xfw_window_x11_get_xid(mGroupWindow->mXfwWindow));

		if (window != nullptr)
		{
			// we're probably not doing anything wrong at our level, but things can go wrong in cairo when
			// multiple windows are closed and thumbnails are shown (#71), so let's catch X11 errors for it
			GdkDisplay* display = gdk_display_get_default();
			gdk_x11_display_error_trap_push(display);
			pixbuf = gdk_pixbuf_get_from_window(window, 0, 0, gdk_window_get_width(window), gdk_window_get_height(window));
			gdk_x11_display_error_trap_pop_ignored(display);

			if (pixbuf != nullptr)
			{
				gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(mPreview));

				gint previewWidth = Settings::previewWidth * scale_factor;
				gint previewHeight = Settings::previewHeight * scale_factor;

				gint pixbufWidth = gdk_pixbuf_get_width(pixbuf);
				gint pixbufHeight = gdk_pixbuf_get_height(pixbuf);

				/* calculate the new dimensions */
				gdouble wRatio = (gdouble)pixbufWidth / (gdouble)previewWidth;
				gdouble hRatio = (gdouble)pixbufHeight / (gdouble)previewHeight;

				if (hRatio > wRatio)
				{
					pixbufWidth = MAX(1, pixbufWidth / hRatio);
					pixbufHeight = MIN(pixbufHeight, previewHeight);
				}
				else
				{
					pixbufWidth = MIN(pixbufWidth, previewWidth);
					pixbufHeight = MAX(1, pixbufHeight / wRatio);
				}

				thumbnail = gdk_pixbuf_scale_simple(pixbuf, pixbufWidth, pixbufHeight, GDK_INTERP_BILINEAR);

				GdkPixbuf* sized = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, previewWidth, previewHeight);
				gint thumbWidth = gdk_pixbuf_get_width(thumbnail);
				gint thumbHeight = gdk_pixbuf_get_height(thumbnail);
				gint xOffset = (previewWidth - thumbWidth) / 2;
				gint yOffset = (previewHeight - thumbHeight) / 2;
				gdk_pixbuf_composite(thumbnail, sized, xOffset, yOffset, thumbWidth, thumbHeight, xOffset, yOffset, 1, 1, GDK_INTERP_BILINEAR, 255);

				cairo_surface_t* surface = gdk_cairo_surface_create_from_pixbuf(sized, scale_factor, nullptr);

				gtk_image_set_from_surface(mPreview, surface);

				cairo_surface_destroy(surface);
				g_object_unref(sized);
				g_object_unref(thumbnail);
				g_object_unref(pixbuf);
			}
			g_object_unref(window);
		}
	}
#endif
}

/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

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
	gtk_widget_set_margin_start(GTK_WIDGET(mGrid), 6);
	gtk_widget_set_margin_end(GTK_WIDGET(mGrid), 6);
	gtk_widget_set_margin_top(GTK_WIDGET(mGrid), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(mGrid), 2);
	gtk_widget_show(GTK_WIDGET(mGrid));
	gtk_container_add(GTK_CONTAINER(mItem), GTK_WIDGET(mGrid));

	mIcon = GTK_IMAGE(gtk_image_new());
	gtk_widget_show(GTK_WIDGET(mIcon));
	gtk_grid_attach(mGrid, GTK_WIDGET(mIcon), 0, 0, 1, 1);

	mLabel = GTK_LABEL(gtk_label_new(""));
	gtk_label_set_xalign(mLabel, 0);
	gtk_label_set_ellipsize(mLabel, PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars(mLabel, 26);
	gtk_widget_show(GTK_WIDGET(mLabel));
	gtk_grid_attach(mGrid, GTK_WIDGET(mLabel), 1, 0, 1, 1);

	mCloseButton = GTK_BUTTON(gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(mCloseButton, GTK_RELIEF_NONE);
	gtk_widget_show(GTK_WIDGET(mCloseButton));
	gtk_grid_attach(mGrid, GTK_WIDGET(mCloseButton), 2, 0, 1, 1);

	mPreview = GTK_IMAGE(gtk_image_new());
	gtk_widget_set_margin_top(GTK_WIDGET(mPreview), 6);
	gtk_widget_set_margin_bottom(GTK_WIDGET(mPreview), 6);
	gtk_grid_attach(mGrid, GTK_WIDGET(mPreview), 0, 1, 3, 1);
	gtk_widget_set_visible(GTK_WIDGET(mPreview), Settings::showPreviews);

	if (Wnck::getActiveWindowXID() == wnck_window_get_xid(mGroupWindow->mWnckWindow))
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
			me->mGroupWindow->mGroup->mSHover = false;
			Help::Gtk::cssClassRemove(widget, "hover_menu_item");
			gtk_widget_queue_draw(widget);
			gtk_widget_queue_draw(me->mGroupWindow->mGroup->mButton);
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mCloseButton), "clicked",
		G_CALLBACK(+[](GtkButton* button, GroupMenuItem* me) {
			Wnck::close(me->mGroupWindow, 0);
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
	const char* winName = wnck_window_get_name(mGroupWindow->mWnckWindow);

	if (Wnck::getActiveWindowXID() == wnck_window_get_xid(mGroupWindow->mWnckWindow))
	{
		gchar* escaped = g_markup_escape_text(winName, -1);
		gchar* markup = g_strdup_printf("<b>%s</b>", escaped);
		gtk_label_set_markup(mLabel, markup);
		g_free(markup);
		g_free(escaped);
	}
	else if (mGroupWindow->getState(WNCK_WINDOW_STATE_MINIMIZED))
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
	GdkPixbuf* iconPixbuf = wnck_window_get_mini_icon(mGroupWindow->mWnckWindow);

	if (iconPixbuf != NULL)
		gtk_image_set_from_pixbuf(GTK_IMAGE(mIcon), iconPixbuf);
}

void GroupMenuItem::updatePreview()
{
	gtk_widget_set_visible(GTK_WIDGET(mPreview), Settings::showPreviews);

	if (!Settings::showPreviews)
		return;

	if (mGroupWindow->getState(WNCK_WINDOW_STATE_MINIMIZED))
		return; // minimized windows never need a new thumbnail

	// This needs work to survive porting to GTK4 and/or Wayland.
	// use gdk_pixbuf_get_from_surface in GTK4
	// use gdk_device_get_window_at_position in Wayland

	if (GDK_IS_X11_DISPLAY(Plugin::mDisplay))
	{
		GdkWindow* window;
		GdkPixbuf* pixbuf;
		GdkPixbuf* thumbnail;

		double scale = 0.125;
		if (Settings::previewScale)
			scale = Settings::previewScale;

		window = gdk_x11_window_foreign_new_for_display(Plugin::mDisplay,
			wnck_window_get_xid(mGroupWindow->mWnckWindow));

		if (window != NULL)
		{
			pixbuf = gdk_pixbuf_get_from_window(window, 0, 0, gdk_window_get_width(window),
				gdk_window_get_height(window));

			if (pixbuf != NULL)
			{
				thumbnail = gdk_pixbuf_scale_simple(pixbuf,
					gdk_pixbuf_get_width(pixbuf) * scale, gdk_pixbuf_get_height(pixbuf) * scale, GDK_INTERP_BILINEAR);

				gtk_image_set_from_pixbuf(mPreview, thumbnail);

				g_object_unref(thumbnail);
				g_object_unref(pixbuf);
			}
			g_object_unref(window);
		}
	}
}

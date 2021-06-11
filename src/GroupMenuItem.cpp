/*
 * Docklike Taskbar - A modern, minimalist taskbar for XFCE
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * gnu.org/licenses/gpl-3.0
 */

#include "GroupMenuItem.hpp"

static GtkTargetEntry entries[1] = {{(gchar*)"any", 0, 0}};

GroupMenuItem::GroupMenuItem(GroupWindow* groupWindow)
{
	mGroupWindow = groupWindow;

	mItem = (GtkEventBox*)gtk_event_box_new();
	Help::Gtk::cssClassAdd(GTK_WIDGET(mItem), "menu_item");
	gtk_widget_show(GTK_WIDGET(mItem));
	g_object_ref(mItem);

	mGrid = (GtkGrid*)gtk_grid_new();
	gtk_widget_show(GTK_WIDGET(mGrid));
	gtk_container_add(GTK_CONTAINER(mItem), GTK_WIDGET(mGrid));

	mIcon = (GtkImage*)gtk_image_new();
	gtk_widget_show(GTK_WIDGET(mIcon));
	gtk_grid_attach(mGrid, GTK_WIDGET(mIcon), 0, 0, 1, 1);

	mLabel = (GtkLabel*)gtk_label_new("");
	gtk_label_set_xalign(mLabel, 0);
	gtk_label_set_ellipsize(mLabel, PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars(mLabel, 26);
	gtk_widget_show(GTK_WIDGET(mLabel));
	gtk_grid_attach(mGrid, GTK_WIDGET(mLabel), 1, 0, 1, 1);

	mCloseButton = (GtkButton*)gtk_button_new_from_icon_name("gtk-close", GTK_ICON_SIZE_MENU);
	gtk_widget_show(GTK_WIDGET(mCloseButton));
	gtk_grid_attach(mGrid, GTK_WIDGET(mCloseButton), 2, 0, 1, 1);

	mPreview = (GtkImage*)gtk_image_new();
	gtk_widget_set_margin_top(GTK_WIDGET(mPreview), 6);
	gtk_widget_set_margin_bottom(GTK_WIDGET(mPreview), 6);
	gtk_grid_attach(mGrid, GTK_WIDGET(mPreview), 1, 1, 1, 1);
	gtk_widget_set_visible(GTK_WIDGET(mPreview), Settings::showPreviews);

	if (Settings::showPreviews)
		updatePreview();

	// Update the previews every second while the group or menu is hovered
	mPreviewTimeout.setup(1000, [this]() {
		gtk_widget_set_visible(GTK_WIDGET(mPreview), Settings::showPreviews);
		updatePreview();
		return true;
	});

	mDragSwitchTimeout.setup(250, [this]() {
		mGroupWindow->activate(0);
		return false;
	});

	//--------------------------------------------------

	g_signal_connect(G_OBJECT(mItem), "button-press-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, GroupMenuItem* me) {
			if (event->button == 1)
				me->mGroupWindow->activate((event)->time);
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mItem), "enter-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventCrossing* event, GroupMenuItem* me) {
			if (event->state & GDK_BUTTON1_MASK)
				me->mGroupWindow->activate(event->time);
			Help::Gtk::cssClassAdd(GTK_WIDGET(me->mItem), "hover");
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mItem), "leave-notify-event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEvent* event, GroupMenuItem* me) {
			Help::Gtk::cssClassRemove(GTK_WIDGET(me->mItem), "hover");
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mItem), "drag-leave",
		G_CALLBACK(+[](GtkWidget* widget, GdkDragContext* context, guint time, GroupMenuItem* me) {
			me->mGroupWindow->mGroup->setMouseLeaveTimeout();
			me->mDragSwitchTimeout.stop();
		}),
		this);

	g_signal_connect(G_OBJECT(mItem), "drag-motion",
		G_CALLBACK(+[](GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time, GroupMenuItem* me) {
			if (!me->mDragSwitchTimeout.mTimeoutId)
				me->mDragSwitchTimeout.start();

			me->mGroupWindow->mGroup->mLeaveTimeout.stop();
			gdk_drag_status(context, GDK_ACTION_DEFAULT, time);
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(mCloseButton), "clicked",
		G_CALLBACK(+[](GtkButton* button, GroupMenuItem* me) {
			Wnck::close(me->mGroupWindow, 0);
		}),
		this);

	gtk_drag_dest_set(GTK_WIDGET(mItem), GTK_DEST_DEFAULT_DROP, entries, 1, GDK_ACTION_MOVE);
}

GroupMenuItem::~GroupMenuItem()
{
	mPreviewTimeout.stop();
	gtk_widget_destroy(GTK_WIDGET(mPreview));
	gtk_widget_destroy(GTK_WIDGET(mItem));
}

void GroupMenuItem::updateLabel()
{
	gtk_label_set_text(mLabel, Wnck::getName(mGroupWindow).c_str());
}

void GroupMenuItem::updateIcon()
{
	GdkPixbuf* iconPixbuf = Wnck::getMiniIcon(mGroupWindow);

	if (iconPixbuf != NULL)
		gtk_image_set_from_pixbuf(GTK_IMAGE(mIcon), iconPixbuf);
}

void GroupMenuItem::updatePreview()
{
	// TODO: This needs work to survive porting to GTK4 and/or Wayland.
	// use gdk_pixbuf_get_from_surface in GTK4
	// use gdk_device_get_window_at_position in Wayland

	if (GDK_IS_X11_DISPLAY(Plugin::display))
	{
		GdkWindow* win = gdk_x11_window_foreign_new_for_display(Plugin::display, wnck_window_get_xid(mGroupWindow->mWnckWindow));

		if (win != NULL && (mGroupWindow->mState & WNCK_WINDOW_STATE_MINIMIZED) != WNCK_WINDOW_STATE_MINIMIZED)
		{
			GdkPixbuf* pb = gdk_pixbuf_get_from_window(win, 0, 0, gdk_window_get_width(win), gdk_window_get_height(win));

			if (pb != NULL)
			{
				gtk_image_set_from_pixbuf(mPreview,
					gdk_pixbuf_scale_simple(pb, gdk_pixbuf_get_width(pb) / 8,
						gdk_pixbuf_get_height(pb) / 8, GDK_INTERP_BILINEAR));
			}

			g_object_unref(pb);
		}

		g_object_unref(win);
	}
}

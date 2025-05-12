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

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif
#include "Plugin.hpp"
#include "Helpers.hpp"
#ifdef ENABLE_X11
#include "Hotkeys.hpp"
#endif

namespace Plugin
{
	XfcePanelPlugin* mXfPlugin;
	GdkDevice* mPointer;
	GdkDisplay* mDisplay;

	static void init(XfcePanelPlugin* xfPlugin)
	{
		xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, nullptr);

		mXfPlugin = xfPlugin;
		mDisplay = gdk_display_get_default();
		mPointer = gdk_seat_get_pointer(gdk_display_get_default_seat(mDisplay));

		Settings::init();
		AppInfos::init();
		Xfw::init();
		Dock::init();
		Theme::init();
#ifdef ENABLE_X11
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
			Hotkeys::init();
#endif

		gtk_container_add(GTK_CONTAINER(mXfPlugin), Dock::mBox);
		xfce_panel_plugin_menu_show_configure(mXfPlugin);
		xfce_panel_plugin_menu_show_about(mXfPlugin);

		//--------------------------------------------------

		g_signal_connect(G_OBJECT(GTK_WIDGET(mXfPlugin)), "size-changed",
			G_CALLBACK(+[](XfcePanelPlugin* plugin, gint size) {
				Dock::onPanelResize(size);
				return true;
			}),
			nullptr);

		g_signal_connect(G_OBJECT(GTK_WIDGET(mXfPlugin)), "orientation-changed",
			G_CALLBACK(+[](XfcePanelPlugin* plugin, GtkOrientation orientation) {
				Dock::onPanelOrientationChange(orientation);
			}),
			nullptr);

		g_signal_connect(G_OBJECT(mXfPlugin), "configure-plugin",
			G_CALLBACK(+[](XfcePanelPlugin* plugin) {
				SettingsDialog::popup();
			}),
			nullptr);

		g_signal_connect(G_OBJECT(mXfPlugin), "about",
			G_CALLBACK(+[](XfcePanelPlugin* plugin) {
				Plugin::aboutDialog();
			}),
			nullptr);

		g_signal_connect(G_OBJECT(mXfPlugin), "remote-event",
			G_CALLBACK(+[](XfcePanelPlugin* plugin, gchar* name, GValue* value) {
				remoteEvent(name, value);
			}),
			nullptr);

		g_signal_connect(G_OBJECT(mXfPlugin), "free-data",
			G_CALLBACK(+[](XfcePanelPlugin* plugin) {
				Xfw::finalize();
				Dock::mGroups.clear();
				AppInfos::finalize();
				Settings::finalize();
			}),
			nullptr);
	}

	void aboutDialog()
	{
		const gchar* AUTHORS[] = {
			"Nicolas Szabo <nszabo@vivaldi.net>",
			"David Keogh <davidtkeogh@gmail.com>",
			nullptr};

		gtk_show_about_dialog(nullptr,
			"program-name", PACKAGE_NAME,
			"logo-icon-name", "preferences-system",
			"version", VERSION_FULL,
			"copyright", "Copyright \302\251 2003-" COPYRIGHT_YEAR " The Xfce development team",
			"license-type", GTK_LICENSE_GPL_3_0,
			"authors", AUTHORS,
			"translator-credits", _("translator-credits"),
			nullptr);
	}

	void remoteEvent(gchar* name, GValue* value)
	{
		if (g_strcmp0(name, "settings") == 0)
			SettingsDialog::popup();
		else if (g_strcmp0(name, "about") == 0)
			aboutDialog();
	}

} // namespace Plugin

//----------------------------------------------------------------------------------------------------------------------

extern "C" void construct(XfcePanelPlugin* xfPlugin) { Plugin::init(xfPlugin); }

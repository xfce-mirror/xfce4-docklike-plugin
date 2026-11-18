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

#include "Plugin.hpp"
#include "SettingsDialog.hpp"

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include <gtk/gtk.h>

Plugin::Plugin(XfcePanelPlugin* xfPlugin) : mXfPlugin(xfPlugin)
{
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, nullptr);

	mSettings = new Settings(this);
	mAppInfos = new AppInfos(this);
	mXfw = new Xfw(this);
	mDock = new Dock(this);
	mLauncherEntry = new LauncherEntry(this);
	mTheme = new Theme(this);
	mHotkeys = new Hotkeys(this);

	gtk_container_add(GTK_CONTAINER(xfPlugin), mDock->mBox);
	xfce_panel_plugin_menu_show_configure(xfPlugin);
	xfce_panel_plugin_menu_show_about(xfPlugin);

	g_signal_connect(G_OBJECT(GTK_WIDGET(xfPlugin)), "size-changed",
		G_CALLBACK(+[](XfcePanelPlugin* _xfPlugin, gint size, Plugin* plugin) -> gboolean {
			plugin->mDock->onPanelResize(size);
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(GTK_WIDGET(xfPlugin)), "orientation-changed",
		G_CALLBACK(+[](XfcePanelPlugin* _xfPlugin, GtkOrientation orientation, Plugin* plugin) {
			plugin->mDock->onPanelOrientationChange(orientation);
		}),
		this);

	g_signal_connect(G_OBJECT(xfPlugin), "configure-plugin",
		G_CALLBACK(+[](XfcePanelPlugin* _xfPlugin, Plugin* plugin) {
			SettingsDialog::popup(plugin);
		}),
		this);

	// cppcheck doesn't want this in G_CALLBACK() as long as LIBXFCE4PANEL_CHECK_VERSION is used
	auto aboutDialog = +[](XfcePanelPlugin* _xfPlugin) {
		const gchar* AUTHORS[] = {
			"Nicolas Szabo <nszabo@vivaldi.net>",
			"David Keogh <davidtkeogh@gmail.com>",
			nullptr};

#if LIBXFCE4PANEL_CHECK_VERSION(4, 21, 4)
		xfce_panel_gtk_show_about_dialog(
#else
		gtk_show_about_dialog(nullptr,
#endif
			"program-name", PACKAGE_NAME,
			"logo-icon-name", "preferences-system",
			"version", VERSION_FULL,
			"copyright", "Copyright \302\251 2019-" COPYRIGHT_YEAR " The Xfce development team",
			"license-type", GTK_LICENSE_GPL_3_0,
			"website", PACKAGE_URL,
			"authors", AUTHORS,
			"translator-credits", _("translator-credits"),
			nullptr);
	};
	g_signal_connect(G_OBJECT(xfPlugin), "about",
		G_CALLBACK(aboutDialog),
		nullptr);

	g_signal_connect(G_OBJECT(xfPlugin), "remote-event",
		G_CALLBACK(+[](XfcePanelPlugin* _xfPlugin, gchar* name, GValue* value, Plugin* plugin) -> gboolean {
			if (g_strcmp0(name, "settings") == 0)
				SettingsDialog::popup(plugin);
			else if (g_strcmp0(name, "about") == 0)
				g_signal_emit_by_name(_xfPlugin, "about");
			else if (g_strcmp0(name, "switch-to-last-window") == 0 && plugin->mSettings->keyAloneActive)
				plugin->mXfw->switchToLastWindow();
			else if (g_strcmp0(name, "activate-group") == 0)
			{
				if (G_VALUE_HOLDS_INT64(value) && plugin->mSettings->keyComboActive)
					plugin->mDock->activateGroup(g_value_get_int64(value));
				else if (G_VALUE_HOLDS_STRING(value))
					plugin->mDock->activateGroup(g_value_get_string(value));
			}
			// stop signal emission to other plugins
			return true;
		}),
		this);

	g_signal_connect(G_OBJECT(xfPlugin), "free-data",
		G_CALLBACK(+[](XfcePanelPlugin* _xfPlugin, Plugin* plugin) {
			delete plugin;
		}),
		this);

	g_signal_connect(G_OBJECT(xfPlugin), "removed",
		G_CALLBACK(+[](XfcePanelPlugin* _xfPlugin, Plugin* plugin) {
			plugin->mHotkeys->resetShortcuts();
		}),
		this);
}

Plugin::~Plugin()
{
	delete mLauncherEntry;
	delete mXfw;
	delete mDock;
	delete mAppInfos;
	delete mTheme;
	delete mHotkeys;
	delete mSettings;
}

extern "C" void construct(XfcePanelPlugin* xfPlugin) { new Plugin(xfPlugin); }

/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Plugin.hpp"
#include "Helpers.hpp"

namespace Plugin
{
	XfcePanelPlugin* mXfPlugin;
	GdkDevice* mPointer;
	GdkDisplay* mDisplay;

	void init(XfcePanelPlugin* xfPlugin)
	{
		xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, NULL);

		mXfPlugin = xfPlugin;
		mDisplay = gdk_display_get_default();
		mPointer = gdk_seat_get_pointer(gdk_display_get_default_seat(mDisplay));

		Settings::init();
		AppInfos::init();
		Dock::init();
		Wnck::init();
		Theme::init();
		Hotkeys::init();

		gtk_container_add(GTK_CONTAINER(mXfPlugin), GTK_WIDGET(Dock::mBox));
		xfce_panel_plugin_menu_show_configure(mXfPlugin);
		xfce_panel_plugin_menu_show_about(mXfPlugin);

		//--------------------------------------------------

		g_signal_connect(G_OBJECT(GTK_WIDGET(mXfPlugin)), "size-changed",
			G_CALLBACK(+[](XfcePanelPlugin* plugin, gint size) {
				Dock::onPanelResize(size);
				return true;
			}),
			NULL);

		g_signal_connect(G_OBJECT(GTK_WIDGET(mXfPlugin)), "orientation-changed",
			G_CALLBACK(+[](XfcePanelPlugin* plugin, GtkOrientation orientation) {
				Dock::onPanelOrientationChange(orientation);
			}),
			NULL);

		g_signal_connect(G_OBJECT(mXfPlugin), "configure-plugin",
			G_CALLBACK(+[](XfcePanelPlugin* plugin) {
				SettingsDialog::popup();
			}),
			NULL);

		g_signal_connect(G_OBJECT(mXfPlugin), "about",
			G_CALLBACK(+[](XfcePanelPlugin* plugin) {
				Plugin::aboutDialog();
			}),
			NULL);

		g_signal_connect(G_OBJECT(mXfPlugin), "remote-event",
			G_CALLBACK(+[](XfcePanelPlugin* plugin, gchar* name, GValue* value) {
				remoteEvent(name, value);
			}),
			NULL);
	}

	void aboutDialog()
	{
		const gchar* AUTHORS[] = {
			"Nicolas Szabo <nszabo@vivaldi.net> © 2019-2020",
			"David Keogh <davidtkeogh@gmail.com> © 2020-2021"
			"\n",
			NULL};

		// TODO: Load these from a TRANSLATORS text file, so people can add themselves.
		const gchar* TRANSLATORS =
			"Yamada Hayao <hayao@fascode.net> : ja\n"
			"Dmitry K <dkabishchev@ya.ru> : ru\n"
			"Fábio Meneghetti <fabiom@riseup.net> : pt\n"
			"Mirko Brombin <send@mirko.pm> : it\n"
			"Adem Kürşat Uzun <ademkursatuzun@gmail.com> : tr\n"
			"Santiago Soler <santiago.r.soler@gmail.com> : es\n"
			"fredii: de\n"
			"Lucas Hadjilucas <puzzle@outlook.com> : el\n"
			"Jan Kazemier : nl\n"
			"Matthaiks : pl\n"
			"Faisal Rachmadin <frachmadin@gmail.com> : id\n";

		gtk_show_about_dialog(NULL,
			"program-name", "Docklike Taskbar",
			"logo-icon-name", "preferences-system-windows",
			"version", PACKAGE_VERSION,
			"copyright", "Copyright © 2019-2021",
			"license-type", GTK_LICENSE_GPL_3_0,
			"website", PLUGIN_WEBSITE,
			"website-label", PLUGIN_WEBSITE,
			"authors", AUTHORS,
			"translator-credits", TRANSLATORS,
			NULL);
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

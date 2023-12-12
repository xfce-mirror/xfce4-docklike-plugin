/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Settings.hpp"

namespace Settings
{
	std::string mPath;
	GKeyFile* mFile;

	State<bool> forceIconSize;
	State<int> iconSize;

	State<bool> noWindowsListIfSingle;
	State<bool> onlyDisplayVisible;
	State<bool> onlyDisplayScreen;
	State<bool> showPreviews;
	State<int> middleButtonBehavior;

	State<int> indicatorOrientation;
	State<int> indicatorStyle;
	State<int> inactiveIndicatorStyle;
	State<bool> indicatorColorFromTheme;
	State<GdkRGBA*> indicatorColor;
	State<GdkRGBA*> inactiveColor;

	State<bool> keyComboActive;
	State<bool> keyAloneActive;

	State<std::list<std::string>> pinnedAppList;

	State<bool> showWindowCount;
	State<int> dockSize;
	State<double> previewScale;
	State<int> previewSleep;

	void init()
	{
		gchar* location = xfce_panel_plugin_save_location(Plugin::mXfPlugin, true);
		mPath = location;
		g_free(location);
		mFile = g_key_file_new();

		if (g_file_test(mPath.c_str(), G_FILE_TEST_IS_REGULAR))
			g_key_file_load_from_file(mFile, mPath.c_str(), G_KEY_FILE_NONE, NULL);

		else // Look for a default config file in XDG_CONFIG_DIRS/xfce4/panel/docklike.rc
		{
			gchar* distConfig = xfce_resource_lookup(XFCE_RESOURCE_CONFIG, "xfce4/panel/docklike.rc");

			if (distConfig != NULL && g_file_test(distConfig, G_FILE_TEST_IS_REGULAR))
				g_key_file_load_from_file(mFile, distConfig, G_KEY_FILE_NONE, NULL);

			g_free(distConfig);
		}

		showPreviews.setup(g_key_file_get_boolean(mFile, "user", "showPreviews", NULL),
			[](bool _showPreviews) -> void {
				g_key_file_set_boolean(mFile, "user", "showPreviews", _showPreviews);
				saveFile();
			});
		
		showWindowCount.setup(g_key_file_get_boolean(mFile, "user", "showWindowCount", NULL),
			[](bool _showWindowCount) -> void {
				g_key_file_set_boolean(mFile, "user", "showWindowCount", _showWindowCount);
				saveFile();

				Dock::drawGroups();
			});

		middleButtonBehavior.setup(g_key_file_get_integer(mFile, "user", "middleButtonBehavior", NULL),
			[](int _middleButtonBehavior) -> void {
				g_key_file_set_integer(mFile, "user", "middleButtonBehavior", _middleButtonBehavior);
				saveFile();
			});

		indicatorOrientation.setup(g_key_file_get_integer(mFile, "user", "indicatorOrientation", NULL),
			[](int _indicatorOrientation) -> void {
				g_key_file_set_integer(mFile, "user", "indicatorOrientation", _indicatorOrientation);
				saveFile();

				Dock::drawGroups();
			});

		forceIconSize.setup(g_key_file_get_boolean(mFile, "user", "forceIconSize", NULL),
			[](bool _forceIconSize) -> void {
				g_key_file_set_boolean(mFile, "user", "forceIconSize", _forceIconSize);
				saveFile();

				Dock::onPanelResize();
			});

		iconSize.setup(g_key_file_get_integer(mFile, "user", "iconSize", NULL),
			[](int _iconSize) -> void {
				g_key_file_set_integer(mFile, "user", "iconSize", _iconSize);
				saveFile();

				Dock::onPanelResize();
			});

		indicatorStyle.setup(g_key_file_get_integer(mFile, "user", "indicatorStyle", NULL),
			[](int _indicatorStyle) -> void {
				g_key_file_set_integer(mFile, "user", "indicatorStyle", _indicatorStyle);
				saveFile();

				Dock::drawGroups();
			});

		inactiveIndicatorStyle.setup(g_key_file_get_integer(mFile, "user", "inactiveIndicatorStyle", NULL),
			[](int _inactiveIndicatorStyle) -> void {
				g_key_file_set_integer(mFile, "user", "inactiveIndicatorStyle", _inactiveIndicatorStyle);
				saveFile();

				Dock::drawGroups();
			});

		indicatorColorFromTheme.setup(g_key_file_get_boolean(mFile, "user", "indicatorColorFromTheme", NULL),
			[](bool _indicatorColorFromTheme) -> void {
				g_key_file_set_boolean(mFile, "user", "indicatorColorFromTheme", _indicatorColorFromTheme);
				saveFile();

				Theme::load();
				Dock::drawGroups();
			});

		gchar* colorString = g_key_file_get_string(mFile, "user", "indicatorColor", NULL);
		GdkRGBA* color = g_new(GdkRGBA, 1);

		if (colorString == NULL || !gdk_rgba_parse(color, colorString))
			gdk_rgba_parse(color, "rgb(76,166,230)");
		g_free(colorString);

		indicatorColor.setup(color,
			[](GdkRGBA* _indicatorColor) -> void {
				g_key_file_set_string(mFile, "user", "indicatorColor", gdk_rgba_to_string(_indicatorColor));
				saveFile();

				Theme::load();
				Dock::drawGroups();
			});

		colorString = g_key_file_get_string(mFile, "user", "inactiveColor", NULL);
		color = g_new(GdkRGBA, 1);

		if (colorString == NULL || !gdk_rgba_parse(color, colorString))
			gdk_rgba_parse(color, "rgb(76,166,230)");
		g_free(colorString);

		inactiveColor.setup(color,
			[](GdkRGBA* _inactiveColor) -> void {
				g_key_file_set_string(mFile, "user", "inactiveColor", gdk_rgba_to_string(_inactiveColor));
				saveFile();

				Theme::load();
				Dock::drawGroups();
			});

		noWindowsListIfSingle.setup(g_key_file_get_boolean(mFile, "user", "noWindowsListIfSingle", NULL),
			[](bool _noWindowsListIfSingle) -> void {
				g_key_file_set_boolean(mFile, "user", "noWindowsListIfSingle", _noWindowsListIfSingle);
				saveFile();
			});

		onlyDisplayVisible.setup(g_key_file_get_boolean(mFile, "user", "onlyDisplayVisible", NULL),
			[](bool _onlyDisplayVisible) -> void {
				g_key_file_set_boolean(mFile, "user", "onlyDisplayVisible", _onlyDisplayVisible);
				saveFile();
			});

		onlyDisplayScreen.setup(g_key_file_get_boolean(mFile, "user", "onlyDisplayScreen", NULL),
			[](bool _onlyDisplayScreen) -> void {
				g_key_file_set_boolean(mFile, "user", "onlyDisplayScreen", _onlyDisplayScreen);
				saveFile();
			});

		keyComboActive.setup(g_key_file_get_boolean(mFile, "user", "keyComboActive", NULL),
			[](bool _keyComboActive) -> void {
				g_key_file_set_boolean(mFile, "user", "keyComboActive", _keyComboActive);
				saveFile();

				Hotkeys::updateSettings();
			});

		keyAloneActive.setup(g_key_file_get_boolean(mFile, "user", "keyAloneActive", NULL),
			[](bool _keyAloneActive) -> void {
				g_key_file_set_boolean(mFile, "user", "keyAloneActive", _keyAloneActive);
				saveFile();

				Hotkeys::updateSettings();
			});

		gchar** pinnedListBuffer = g_key_file_get_string_list(mFile, "user", "pinned", NULL, NULL);

		pinnedAppList.setup(Help::Gtk::bufferToStdStringList(pinnedListBuffer),
			[](std::list<std::string> list) -> void {
				std::vector<char*> buf = Help::Gtk::stdToBufferStringList(list);
				g_key_file_set_string_list(mFile, "user", "pinned", buf.data(), buf.size());
				saveFile();
			});

		if (pinnedListBuffer != NULL)
		{
			// try to be backward compatible: retrieve ids from paths
			for (gchar** p = pinnedListBuffer; *p != NULL; p++)
				if (**p == '/' && g_str_has_suffix(*p, ".desktop"))
				{
					gchar* basename = g_path_get_basename(*p);
					std::string id = basename;
					g_free(basename);
					g_free(*p);
					*p = g_strdup(id.substr(0, id.size() - 8).c_str());
				}
			pinnedAppList.set(Help::Gtk::bufferToStdStringList(pinnedListBuffer));
			g_strfreev(pinnedListBuffer);
		}

		// HIDDEN SETTINGS:
		dockSize.setup(g_key_file_get_integer(mFile, "user", "dockSize", NULL),
			[](int _dockSize) -> void {
				g_key_file_set_integer(mFile, "user", "dockSize", _dockSize);
				saveFile();
			});
		
		previewScale.setup(g_key_file_get_double(mFile, "user", "previewScale", NULL),
			[](int _previewScale) -> void {
				g_key_file_set_double(mFile, "user", "previewScale", _previewScale);
				saveFile();
			});
		
		previewSleep.setup(g_key_file_get_integer(mFile, "user", "previewSleep", NULL),
			[](int _previewSleep) -> void {
				g_key_file_set_integer(mFile, "user", "previewSleep", _previewSleep);
				saveFile();
			});
	}

	void saveFile()
	{
		g_key_file_save_to_file(mFile, mPath.c_str(), NULL);
	}
} // namespace Settings

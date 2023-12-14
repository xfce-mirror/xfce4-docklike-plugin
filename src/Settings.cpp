/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Settings.hpp"

namespace Settings
{
	Store::AutoPtr<gchar> mPath;
	Store::AutoPtr<GKeyFile> mFile;

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
	State<std::shared_ptr<GdkRGBA>> indicatorColor;
	State<std::shared_ptr<GdkRGBA>> inactiveColor;

	State<bool> keyComboActive;
	State<bool> keyAloneActive;

	State<std::list<std::string>> pinnedAppList;

	State<bool> showWindowCount;
	State<int> dockSize;
	State<double> previewScale;
	State<int> previewSleep;

	void init()
	{
		gchar* path = xfce_panel_plugin_save_location(Plugin::mXfPlugin, true);
		GKeyFile* file = g_key_file_new();
		mPath = Store::AutoPtr<gchar>(path, g_free);
		mFile = Store::AutoPtr<GKeyFile>(file, (GDestroyNotify)g_key_file_unref);

		if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
			g_key_file_load_from_file(file, path, G_KEY_FILE_NONE, NULL);

		else // Look for a default config file in XDG_CONFIG_DIRS/xfce4/panel/docklike.rc
		{
			gchar* distConfig = xfce_resource_lookup(XFCE_RESOURCE_CONFIG, "xfce4/panel/docklike.rc");

			if (distConfig != NULL && g_file_test(distConfig, G_FILE_TEST_IS_REGULAR))
				g_key_file_load_from_file(file, distConfig, G_KEY_FILE_NONE, NULL);

			g_free(distConfig);
		}

		showPreviews.setup(g_key_file_get_boolean(file, "user", "showPreviews", NULL),
			[](bool _showPreviews) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "showPreviews", _showPreviews);
				saveFile();
			});
		
		showWindowCount.setup(g_key_file_get_boolean(file, "user", "showWindowCount", NULL),
			[](bool _showWindowCount) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "showWindowCount", _showWindowCount);
				saveFile();

				Dock::drawGroups();
			});

		middleButtonBehavior.setup(g_key_file_get_integer(file, "user", "middleButtonBehavior", NULL),
			[](int _middleButtonBehavior) -> void {
				g_key_file_set_integer(mFile.get(), "user", "middleButtonBehavior", _middleButtonBehavior);
				saveFile();
			});

		indicatorOrientation.setup(g_key_file_get_integer(file, "user", "indicatorOrientation", NULL),
			[](int _indicatorOrientation) -> void {
				g_key_file_set_integer(mFile.get(), "user", "indicatorOrientation", _indicatorOrientation);
				saveFile();

				Dock::drawGroups();
			});

		forceIconSize.setup(g_key_file_get_boolean(file, "user", "forceIconSize", NULL),
			[](bool _forceIconSize) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "forceIconSize", _forceIconSize);
				saveFile();

				Dock::onPanelResize();
			});

		iconSize.setup(g_key_file_get_integer(file, "user", "iconSize", NULL),
			[](int _iconSize) -> void {
				g_key_file_set_integer(mFile.get(), "user", "iconSize", _iconSize);
				saveFile();

				Dock::onPanelResize();
			});

		indicatorStyle.setup(g_key_file_get_integer(file, "user", "indicatorStyle", NULL),
			[](int _indicatorStyle) -> void {
				g_key_file_set_integer(mFile.get(), "user", "indicatorStyle", _indicatorStyle);
				saveFile();

				Dock::drawGroups();
			});

		inactiveIndicatorStyle.setup(g_key_file_get_integer(file, "user", "inactiveIndicatorStyle", NULL),
			[](int _inactiveIndicatorStyle) -> void {
				g_key_file_set_integer(mFile.get(), "user", "inactiveIndicatorStyle", _inactiveIndicatorStyle);
				saveFile();

				Dock::drawGroups();
			});

		indicatorColorFromTheme.setup(g_key_file_get_boolean(file, "user", "indicatorColorFromTheme", NULL),
			[](bool _indicatorColorFromTheme) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "indicatorColorFromTheme", _indicatorColorFromTheme);
				saveFile();

				Theme::load();
				Dock::drawGroups();
			});

		gchar* colorString = g_key_file_get_string(file, "user", "indicatorColor", NULL);
		std::shared_ptr<GdkRGBA> color(g_new(GdkRGBA, 1), g_free);

		if (colorString == NULL || !gdk_rgba_parse(color.get(), colorString))
			gdk_rgba_parse(color.get(), "rgb(76,166,230)");
		g_free(colorString);

		indicatorColor.setup(color,
			[](std::shared_ptr<GdkRGBA> _indicatorColor) -> void {
				gchar* str = gdk_rgba_to_string(_indicatorColor.get());
				g_key_file_set_string(mFile.get(), "user", "indicatorColor", str);
				g_free(str);
				saveFile();

				Theme::load();
				Dock::drawGroups();
			});

		colorString = g_key_file_get_string(file, "user", "inactiveColor", NULL);
		color = std::shared_ptr<GdkRGBA>(g_new(GdkRGBA, 1), g_free);

		if (colorString == NULL || !gdk_rgba_parse(color.get(), colorString))
			gdk_rgba_parse(color.get(), "rgb(76,166,230)");
		g_free(colorString);

		inactiveColor.setup(color,
			[](std::shared_ptr<GdkRGBA> _inactiveColor) -> void {
				gchar* str = gdk_rgba_to_string(_inactiveColor.get());
				g_key_file_set_string(mFile.get(), "user", "inactiveColor", str);
				g_free(str);
				saveFile();

				Theme::load();
				Dock::drawGroups();
			});

		noWindowsListIfSingle.setup(g_key_file_get_boolean(file, "user", "noWindowsListIfSingle", NULL),
			[](bool _noWindowsListIfSingle) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "noWindowsListIfSingle", _noWindowsListIfSingle);
				saveFile();
			});

		onlyDisplayVisible.setup(g_key_file_get_boolean(file, "user", "onlyDisplayVisible", NULL),
			[](bool _onlyDisplayVisible) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "onlyDisplayVisible", _onlyDisplayVisible);
				saveFile();
			});

		onlyDisplayScreen.setup(g_key_file_get_boolean(file, "user", "onlyDisplayScreen", NULL),
			[](bool _onlyDisplayScreen) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "onlyDisplayScreen", _onlyDisplayScreen);
				saveFile();
			});

		keyComboActive.setup(g_key_file_get_boolean(file, "user", "keyComboActive", NULL),
			[](bool _keyComboActive) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "keyComboActive", _keyComboActive);
				saveFile();

				Hotkeys::updateSettings();
			});

		keyAloneActive.setup(g_key_file_get_boolean(file, "user", "keyAloneActive", NULL),
			[](bool _keyAloneActive) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "keyAloneActive", _keyAloneActive);
				saveFile();

				Hotkeys::updateSettings();
			});

		gchar** pinnedListBuffer = g_key_file_get_string_list(file, "user", "pinned", NULL, NULL);

		pinnedAppList.setup(Help::Gtk::bufferToStdStringList(pinnedListBuffer),
			[](std::list<std::string> list) -> void {
				std::vector<char*> buf = Help::Gtk::stdToBufferStringList(list);
				g_key_file_set_string_list(mFile.get(), "user", "pinned", buf.data(), buf.size());
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
		dockSize.setup(g_key_file_get_integer(file, "user", "dockSize", NULL),
			[](int _dockSize) -> void {
				g_key_file_set_integer(mFile.get(), "user", "dockSize", _dockSize);
				saveFile();
			});
		
		previewScale.setup(g_key_file_get_double(file, "user", "previewScale", NULL),
			[](int _previewScale) -> void {
				g_key_file_set_double(mFile.get(), "user", "previewScale", _previewScale);
				saveFile();
			});
		
		previewSleep.setup(g_key_file_get_integer(file, "user", "previewSleep", NULL),
			[](int _previewSleep) -> void {
				g_key_file_set_integer(mFile.get(), "user", "previewSleep", _previewSleep);
				saveFile();
			});
	}

	void saveFile()
	{
		g_key_file_save_to_file(mFile.get(), mPath.get(), NULL);
	}
} // namespace Settings

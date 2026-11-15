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

#include "Settings.hpp"
#include "Dock.hpp"
#include "Hotkeys.hpp"
#include "LauncherEntry.hpp"
#include "Theme.hpp"

Settings::Settings(Plugin* plugin) : mPlugin(plugin)
{
	gchar* path = xfce_panel_plugin_save_location(plugin->mXfPlugin, true);
	GKeyFile* file = g_key_file_new();
	mPath = Store::AutoPtr<gchar>(path, g_free);
	mFile = Store::AutoPtr<GKeyFile>(file, (GDestroyNotify)g_key_file_unref);
	GError* error = nullptr;
	int intValue;

	if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
		g_key_file_load_from_file(file, path, G_KEY_FILE_NONE, nullptr);
	else // Look for a default config file in XDG_CONFIG_DIRS/xfce4/panel/docklike.rc
	{
		gchar* distConfig = xfce_resource_lookup(XFCE_RESOURCE_CONFIG, "xfce4/panel/docklike.rc");

		if (distConfig != nullptr && g_file_test(distConfig, G_FILE_TEST_IS_REGULAR))
			g_key_file_load_from_file(file, distConfig, G_KEY_FILE_NONE, nullptr);

		g_free(distConfig);
	}

	showPreviews.setup(g_key_file_get_boolean(file, "user", "showPreviews", nullptr),
		[this](bool _showPreviews) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "showPreviews", _showPreviews);
			saveFile();

			Dock::mGroups.forEach([](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> g) -> void {
				g.second->mGroupMenu.showPreviewsChanged();
			});
		});

	intValue = g_key_file_get_integer(file, "user", "previewWidth", nullptr);
	previewWidth.setup(intValue > 0 ? intValue : defPreviewWidth,
		[this](int _previewWidth) -> void {
			if (_previewWidth <= 0)
			{
				previewWidth.set(defPreviewWidth);
				return;
			}

			g_key_file_set_integer(mFile.get(), "user", "previewWidth", _previewWidth);
			saveFile();

			Dock::mGroups.forEach([](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> g) -> void {
				g.second->mGroupMenu.showPreviewsChanged();
			});
		});

	intValue = g_key_file_get_integer(file, "user", "previewHeight", nullptr);
	previewHeight.setup(intValue > 0 ? intValue : defPreviewHeight,
		[this](int _previewHeight) -> void {
			if (_previewHeight <= 0)
			{
				previewHeight.set(defPreviewHeight);
				return;
			}

			g_key_file_set_integer(mFile.get(), "user", "previewHeight", _previewHeight);
			saveFile();

			Dock::mGroups.forEach([](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> g) -> void {
				g.second->mGroupMenu.showPreviewsChanged();
			});
		});

	showWindowCount.setup(g_key_file_get_boolean(file, "user", "showWindowCount", nullptr),
		[this](bool _showWindowCount) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "showWindowCount", _showWindowCount);
			saveFile();

			Dock::drawGroups();
		});

	disableLauncherCounts.setup(g_key_file_get_boolean(file, "user", "disableLauncherCounts", nullptr),
		[this](bool _disableLauncherCounts) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "disableLauncherCounts", _disableLauncherCounts);
			saveFile();

			LauncherEntry::refreshGroups();
		});

	intValue = g_key_file_get_integer(file, "user", "middleButtonBehavior", &error);
	if (error != nullptr)
	{
		intValue = BEHAVIOR_DO_NOTHING;
		g_clear_error(&error);
	}
	middleButtonBehavior.setup(intValue,
		[this](int _middleButtonBehavior) -> void {
			g_key_file_set_integer(mFile.get(), "user", "middleButtonBehavior", _middleButtonBehavior);
			saveFile();
		});

	indicatorOrientation.setup(g_key_file_get_integer(file, "user", "indicatorOrientation", nullptr),
		[this](int _indicatorOrientation) -> void {
			g_key_file_set_integer(mFile.get(), "user", "indicatorOrientation", _indicatorOrientation);
			saveFile();

			Dock::drawGroups();
		});

	forceIconSize.setup(g_key_file_get_boolean(file, "user", "forceIconSize", nullptr),
		[this](bool _forceIconSize) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "forceIconSize", _forceIconSize);
			saveFile();

			Dock::onPanelResize();
		});

	iconSize.setup(CLAMP(g_key_file_get_integer(file, "user", "iconSize", nullptr), minIconSize, maxIconSize),
		[this](int _iconSize) -> void {
			int clamped = CLAMP(_iconSize, minIconSize, maxIconSize);
			if (clamped != _iconSize)
			{
				iconSize.set(clamped);
				return;
			}
			g_key_file_set_integer(mFile.get(), "user", "iconSize", _iconSize);
			saveFile();

			Dock::onPanelResize();
		});

	indicatorStyle.setup(g_key_file_get_integer(file, "user", "indicatorStyle", nullptr),
		[this](int _indicatorStyle) -> void {
			g_key_file_set_integer(mFile.get(), "user", "indicatorStyle", _indicatorStyle);
			saveFile();

			Dock::drawGroups();
		});

	inactiveIndicatorStyle.setup(g_key_file_get_integer(file, "user", "inactiveIndicatorStyle", nullptr),
		[this](int _inactiveIndicatorStyle) -> void {
			g_key_file_set_integer(mFile.get(), "user", "inactiveIndicatorStyle", _inactiveIndicatorStyle);
			saveFile();

			Dock::drawGroups();
		});

	indicatorColorFromTheme.setup(g_key_file_get_boolean(file, "user", "indicatorColorFromTheme", nullptr),
		[this](bool _indicatorColorFromTheme) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "indicatorColorFromTheme", _indicatorColorFromTheme);
			saveFile();

			Theme::load();
			Dock::drawGroups();
		});

	gchar* colorString = g_key_file_get_string(file, "user", "indicatorColor", nullptr);
	std::shared_ptr<GdkRGBA> color(g_new(GdkRGBA, 1), g_free);

	if (colorString == nullptr || !gdk_rgba_parse(color.get(), colorString))
		gdk_rgba_parse(color.get(), "rgb(53,132,228)");
	g_free(colorString);

	indicatorColor.setup(color,
		[this](std::shared_ptr<GdkRGBA> _indicatorColor) -> void {
			gchar* str = gdk_rgba_to_string(_indicatorColor.get());
			g_key_file_set_string(mFile.get(), "user", "indicatorColor", str);
			g_free(str);
			saveFile();

			Theme::load();
			Dock::drawGroups();
		});

	colorString = g_key_file_get_string(file, "user", "inactiveColor", nullptr);
	color = std::shared_ptr<GdkRGBA>(g_new(GdkRGBA, 1), g_free);

	if (colorString == nullptr || !gdk_rgba_parse(color.get(), colorString))
		gdk_rgba_parse(color.get(), "rgb(51,209,122)");
	g_free(colorString);

	inactiveColor.setup(color,
		[this](std::shared_ptr<GdkRGBA> _inactiveColor) -> void {
			gchar* str = gdk_rgba_to_string(_inactiveColor.get());
			g_key_file_set_string(mFile.get(), "user", "inactiveColor", str);
			g_free(str);
			saveFile();

			Theme::load();
			Dock::drawGroups();
		});

	noWindowsListIfSingle.setup(g_key_file_get_boolean(file, "user", "noWindowsListIfSingle", nullptr),
		[this](bool _noWindowsListIfSingle) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "noWindowsListIfSingle", _noWindowsListIfSingle);
			saveFile();
		});

	onlyDisplayVisible.setup(g_key_file_get_boolean(file, "user", "onlyDisplayVisible", nullptr),
		[this](bool _onlyDisplayVisible) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "onlyDisplayVisible", _onlyDisplayVisible);
			saveFile();
		});

	onlyDisplayScreen.setup(g_key_file_get_boolean(file, "user", "onlyDisplayScreen", nullptr),
		[this](bool _onlyDisplayScreen) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "onlyDisplayScreen", _onlyDisplayScreen);
			saveFile();
		});

	keyComboActive.setup(g_key_file_get_boolean(file, "user", "keyComboActive", nullptr),
		[this](bool _keyComboActive) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "keyComboActive", _keyComboActive);
			saveFile();
			Hotkeys::updateSettings();
		});

	keyComboMinimize.setup(g_key_file_get_boolean(file, "user", "keyComboMinimize", nullptr),
		[this](bool _keyComboMinimize) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "keyComboMinimize", _keyComboMinimize);
			saveFile();
		});

	keyAloneActive.setup(g_key_file_get_boolean(file, "user", "keyAloneActive", nullptr),
		[this](bool _keyAloneActive) -> void {
			g_key_file_set_boolean(mFile.get(), "user", "keyAloneActive", _keyAloneActive);
			saveFile();
			Hotkeys::updateSettings();
		});

	gchar** pinnedListBuffer = g_key_file_get_string_list(file, "user", "pinned", nullptr, nullptr);

	pinnedAppList.setup(Help::Gtk::bufferToStdStringList(pinnedListBuffer),
		[this](std::list<std::string> list) -> void {
			std::vector<const char*> buf = Help::Gtk::stdToBufferStringList(list);
			g_key_file_set_string_list(mFile.get(), "user", "pinned", buf.data(), buf.size());
			saveFile();
		});

	if (pinnedListBuffer != nullptr)
	{
		// try to be backward compatible: retrieve ids from paths
		for (gchar** p = pinnedListBuffer; *p != nullptr; p++)
			if (**p == '/' && g_str_has_suffix(*p, ".desktop"))
			{
				std::string id = Help::String::pathBasename(*p, true);
				g_free(*p);
				*p = g_strdup(id.c_str());
			}
		pinnedAppList.set(Help::Gtk::bufferToStdStringList(pinnedListBuffer));
		g_strfreev(pinnedListBuffer);
	}

	gchar** userSetIds = g_key_file_get_string_list(file, "user", "userSetIds", nullptr, nullptr);
	gchar** userSetPaths = g_key_file_get_string_list(file, "user", "userSetPaths", nullptr, nullptr);
	std::list<std::string> ids = Help::Gtk::bufferToStdStringList(userSetIds);
	std::list<std::string> paths = Help::Gtk::bufferToStdStringList(userSetPaths);
	ids.resize(paths.size());
	paths.resize(ids.size());
	std::pair<std::list<std::string>, std::list<std::string>> _userSetApps{ids, paths};
	g_strfreev(userSetIds);
	g_strfreev(userSetPaths);

	userSetApps.setup(_userSetApps,
		[this](std::pair<std::list<std::string>, std::list<std::string>> pair) -> void {
			std::vector<const char*> buf = Help::Gtk::stdToBufferStringList(pair.first);
			g_key_file_set_string_list(mFile.get(), "user", "userSetIds", buf.data(), buf.size());
			buf = Help::Gtk::stdToBufferStringList(pair.second);
			g_key_file_set_string_list(mFile.get(), "user", "userSetPaths", buf.data(), buf.size());
			saveFile();
		});

	// HIDDEN SETTINGS:
	dockSize.setup(g_key_file_get_integer(file, "user", "dockSize", nullptr),
		[this](int _dockSize) -> void {
			g_key_file_set_integer(mFile.get(), "user", "dockSize", _dockSize);
			saveFile();
		});

	previewSleep.setup(g_key_file_get_integer(file, "user", "previewSleep", nullptr),
		[this](int _previewSleep) -> void {
			g_key_file_set_integer(mFile.get(), "user", "previewSleep", _previewSleep);
			saveFile();
		});
}

void Settings::saveFile()
{
	g_key_file_save_to_file(mFile.get(), mPath.get(), nullptr);
}

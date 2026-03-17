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
#include "AppInfos.hpp"
#ifdef ENABLE_X11
#include "Hotkeys.hpp"
#endif

#include <unordered_set>

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
	State<std::pair<std::list<std::string>, std::list<std::string>>> userSetApps;

	State<bool> showWindowCount;
	State<int> dockSize;
	State<int> previewWidth;
	State<int> previewHeight;
	State<int> previewSleep;

	void init()
	{
		gchar* path = xfce_panel_plugin_save_location(Plugin::mXfPlugin, true);
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
			[](bool _showPreviews) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "showPreviews", _showPreviews);
				saveFile();

				Dock::mGroups.forEach([](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> g) -> void {
					g.second->mGroupMenu.showPreviewsChanged();
				});
			});

		intValue = g_key_file_get_integer(file, "user", "previewWidth", nullptr);
		previewWidth.setup(intValue > 0 ? intValue : defPreviewWidth,
			[](int _previewWidth) -> void {
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
			[](int _previewHeight) -> void {
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
			[](bool _showWindowCount) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "showWindowCount", _showWindowCount);
				saveFile();

				Dock::drawGroups();
			});

		intValue = g_key_file_get_integer(file, "user", "middleButtonBehavior", &error);
		if (error != nullptr)
		{
			intValue = BEHAVIOR_DO_NOTHING;
			g_clear_error(&error);
		}
		middleButtonBehavior.setup(intValue,
			[](int _middleButtonBehavior) -> void {
				g_key_file_set_integer(mFile.get(), "user", "middleButtonBehavior", _middleButtonBehavior);
				saveFile();
			});

		indicatorOrientation.setup(g_key_file_get_integer(file, "user", "indicatorOrientation", nullptr),
			[](int _indicatorOrientation) -> void {
				g_key_file_set_integer(mFile.get(), "user", "indicatorOrientation", _indicatorOrientation);
				saveFile();

				Dock::drawGroups();
			});

		forceIconSize.setup(g_key_file_get_boolean(file, "user", "forceIconSize", nullptr),
			[](bool _forceIconSize) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "forceIconSize", _forceIconSize);
				saveFile();

				Dock::onPanelResize();
			});

		iconSize.setup(CLAMP(g_key_file_get_integer(file, "user", "iconSize", nullptr), Settings::minIconSize, Settings::maxIconSize),
			[](int _iconSize) -> void {
				int clamped = CLAMP(_iconSize, Settings::minIconSize, Settings::maxIconSize);
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
			[](int _indicatorStyle) -> void {
				g_key_file_set_integer(mFile.get(), "user", "indicatorStyle", _indicatorStyle);
				saveFile();

				Dock::drawGroups();
			});

		inactiveIndicatorStyle.setup(g_key_file_get_integer(file, "user", "inactiveIndicatorStyle", nullptr),
			[](int _inactiveIndicatorStyle) -> void {
				g_key_file_set_integer(mFile.get(), "user", "inactiveIndicatorStyle", _inactiveIndicatorStyle);
				saveFile();

				Dock::drawGroups();
			});

		indicatorColorFromTheme.setup(g_key_file_get_boolean(file, "user", "indicatorColorFromTheme", nullptr),
			[](bool _indicatorColorFromTheme) -> void {
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
			[](std::shared_ptr<GdkRGBA> _indicatorColor) -> void {
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
			[](std::shared_ptr<GdkRGBA> _inactiveColor) -> void {
				gchar* str = gdk_rgba_to_string(_inactiveColor.get());
				g_key_file_set_string(mFile.get(), "user", "inactiveColor", str);
				g_free(str);
				saveFile();

				Theme::load();
				Dock::drawGroups();
			});

		noWindowsListIfSingle.setup(g_key_file_get_boolean(file, "user", "noWindowsListIfSingle", nullptr),
			[](bool _noWindowsListIfSingle) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "noWindowsListIfSingle", _noWindowsListIfSingle);
				saveFile();
			});

		onlyDisplayVisible.setup(g_key_file_get_boolean(file, "user", "onlyDisplayVisible", nullptr),
			[](bool _onlyDisplayVisible) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "onlyDisplayVisible", _onlyDisplayVisible);
				saveFile();
			});

		onlyDisplayScreen.setup(g_key_file_get_boolean(file, "user", "onlyDisplayScreen", nullptr),
			[](bool _onlyDisplayScreen) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "onlyDisplayScreen", _onlyDisplayScreen);
				saveFile();
			});

		keyComboActive.setup(g_key_file_get_boolean(file, "user", "keyComboActive", nullptr),
			[](bool _keyComboActive) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "keyComboActive", _keyComboActive);
				saveFile();

#ifdef ENABLE_X11
				if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
					Hotkeys::updateSettings();
#endif
			});

		keyAloneActive.setup(g_key_file_get_boolean(file, "user", "keyAloneActive", nullptr),
			[](bool _keyAloneActive) -> void {
				g_key_file_set_boolean(mFile.get(), "user", "keyAloneActive", _keyAloneActive);
				saveFile();

#ifdef ENABLE_X11
				if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
					Hotkeys::updateSettings();
#endif
			});

		gchar** pinnedListBuffer = g_key_file_get_string_list(file, "user", "pinned", nullptr, nullptr);

		pinnedAppList.setup(Help::Gtk::bufferToStdStringList(pinnedListBuffer),
			[](std::list<std::string> list) -> void {
				std::vector<char*> buf = Help::Gtk::stdToBufferStringList(list);
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
			[](std::pair<std::list<std::string>, std::list<std::string>> pair) -> void {
				std::vector<char*> buf = Help::Gtk::stdToBufferStringList(pair.first);
				g_key_file_set_string_list(mFile.get(), "user", "userSetIds", buf.data(), buf.size());
				buf = Help::Gtk::stdToBufferStringList(pair.second);
				g_key_file_set_string_list(mFile.get(), "user", "userSetPaths", buf.data(), buf.size());
				saveFile();
			});

		// HIDDEN SETTINGS:
		dockSize.setup(g_key_file_get_integer(file, "user", "dockSize", nullptr),
			[](int _dockSize) -> void {
				g_key_file_set_integer(mFile.get(), "user", "dockSize", _dockSize);
				saveFile();
			});

		previewSleep.setup(g_key_file_get_integer(file, "user", "previewSleep", nullptr),
			[](int _previewSleep) -> void {
				g_key_file_set_integer(mFile.get(), "user", "previewSleep", _previewSleep);
				saveFile();
			});
	}

	void finalize()
	{
		mPath.reset();
		mFile.reset();
		indicatorColor.get().reset();
		inactiveColor.get().reset();
		pinnedAppList.get().clear();
	}

	void saveFile()
	{
		g_key_file_save_to_file(mFile.get(), mPath.get(), nullptr);
	}

	// ---------------------------------------------------------------------------
	// PinnedAppEntry persistence helpers
	// ---------------------------------------------------------------------------

	std::vector<PinnedAppEntry> loadPinnedAppEntries()
	{
		std::vector<PinnedAppEntry> entries;

		// Read the pinned id list directly from the config — do NOT write back
		std::list<std::string> ids = pinnedAppList.get();

		// Pre-load the disabled-default set once (keyed by app id)
		std::unordered_set<std::string> disabledSet;
		gsize disabledLen = 0;
		gchar** disabledList = g_key_file_get_string_list(mFile.get(), "user", "pinnedDisabledDefault", &disabledLen, nullptr);
		if (disabledList != nullptr)
		{
			for (gsize d = 0; d < disabledLen; ++d)
				disabledSet.insert(disabledList[d]);
			g_strfreev(disabledList);
		}

		int idx = 0;
		for (const std::string& id : ids)
		{
			// Skip any empty/blank ids that may have been left by old builds
			if (id.empty())
			{
				++idx;
				continue;
			}

			PinnedAppEntry e;
			e.id = id;

			// Resolve the full .desktop path (read-only, no side effects).
			// AppInfos stores everything under lowercase keys (same as drawGroups),
			// so search with the lowercased id to get a hit.
			std::shared_ptr<AppInfo> appInfo = AppInfos::search(Help::String::toLowercase(id));
			e.path = (appInfo && !appInfo->mPath.empty()) ? appInfo->mPath : id;

			e.defaultKeyDisabled = (disabledSet.count(id) > 0);

			// Custom keys: prefer the launcher-identity-based key "pinnedKeys_id_<id>"
			// (new scheme), fall back to the legacy position-based "pinnedKeys_<idx>"
			// for backward compatibility with existing configs.
			// Note: only apps with a resolved .desktop file (non-empty id) use the
			// id-based key.  Apps whose id couldn't be resolved (empty id, raw
			// command launchers, etc.) always use the positional fallback so they
			// don't all collide on the same "pinnedKeys_id_" key.
			std::string posKeyName = "pinnedKeys_" + std::to_string(idx);

			gsize keysLen = 0;
			gchar** keysList = nullptr;
			if (!id.empty())
			{
				std::string idKeyName = "pinnedKeys_id_" + id;
				keysList = g_key_file_get_string_list(mFile.get(), "user", idKeyName.c_str(), &keysLen, nullptr);
			}
			if (keysList == nullptr)
			{
				// Fall back to the legacy positional key (covers empty-id apps and
				// configs written before this feature was introduced)
				keysList = g_key_file_get_string_list(mFile.get(), "user", posKeyName.c_str(), &keysLen, nullptr);
			}
			if (keysList != nullptr)
			{
				for (gsize k = 0; k < keysLen; ++k)
					e.customKeys.push_back(keysList[k]);
				g_strfreev(keysList);
			}

			entries.push_back(e);
			++idx;
		}

		return entries;
	}

	void savePinnedAppEntries(const std::vector<PinnedAppEntry>& entries)
	{
		// IMPORTANT: Do NOT touch pinnedAppList / "pinned=" here.
		// The pinned order is owned by Dock::savePinned() and the existing
		// drag-drop / reorder machinery.  We only persist our extra metadata:
		//   • pinnedDisabledDefault      — ids whose positional Super+N key is off
		//   • pinnedKeys_id_<appId>      — per-launcher custom key bindings (new scheme,
		//                                  only used when the app has a known .desktop id)
		//   • pinnedKeys_<N>             — positional fallback for apps with no resolved id
		//
		// The old positional keys for apps that now have an id-based key are removed
		// on save so they don't linger.

		// 1. Write the disabled-default id list (already id-based — unchanged)
		std::vector<const char*> disabledBuf;
		for (const auto& e : entries)
			if (e.defaultKeyDisabled)
				disabledBuf.push_back(e.id.c_str());
		g_key_file_set_string_list(mFile.get(), "user", "pinnedDisabledDefault",
			disabledBuf.data(), disabledBuf.size());

		// 2. Remove legacy positional custom key entries for apps that now have an
		//    id-based key (they've been migrated).  Keep positional keys for any
		//    entries whose id is still empty (unresolved / command-based launchers).
		std::unordered_set<int> keepPositional;
		for (int i = 0; i < (int)entries.size(); ++i)
			if (entries[i].id.empty())
				keepPositional.insert(i);

		for (int i = 0; i < 128; ++i)
		{
			if (keepPositional.find(i) == keepPositional.end())
			{
				std::string posKeyName = "pinnedKeys_" + std::to_string(i);
				g_key_file_remove_key(mFile.get(), "user", posKeyName.c_str(), nullptr);
			}
		}

		// 3. Collect the set of app ids currently in the entries so we can remove
		//    stale id-based entries for apps that are no longer pinned.
		std::unordered_set<std::string> currentIds;
		for (const auto& e : entries)
			if (!e.id.empty())
				currentIds.insert(e.id);

		// Remove any id-based key entries whose app is no longer present.
		// We must enumerate the keys in the group to find them.
		gsize nkeys = 0;
		gchar** keys = g_key_file_get_keys(mFile.get(), "user", &nkeys, nullptr);
		if (keys != nullptr)
		{
			for (gsize k = 0; k < nkeys; ++k)
			{
				std::string key(keys[k]);
				const std::string prefix = "pinnedKeys_id_";
				if (key.substr(0, prefix.size()) == prefix)
				{
					std::string storedId = key.substr(prefix.size());
					if (currentIds.find(storedId) == currentIds.end())
						g_key_file_remove_key(mFile.get(), "user", keys[k], nullptr);
				}
			}
			g_strfreev(keys);
		}

		// 4. Write custom keys:
		//    • Known apps (non-empty id) → "pinnedKeys_id_<appId>"
		//    • Unresolved apps (empty id) → "pinnedKeys_<idx>" (positional fallback)
		for (int i = 0; i < (int)entries.size(); ++i)
		{
			const auto& e = entries[i];
			std::string keyName = e.id.empty()
				? "pinnedKeys_" + std::to_string(i)
				: "pinnedKeys_id_" + e.id;

			if (!e.customKeys.empty())
			{
				std::vector<const char*> buf;
				for (const auto& k : e.customKeys)
					buf.push_back(k.c_str());
				g_key_file_set_string_list(mFile.get(), "user", keyName.c_str(),
					buf.data(), buf.size());
			}
			else
			{
				// Ensure no stale key exists for this app
				g_key_file_remove_key(mFile.get(), "user", keyName.c_str(), nullptr);
			}
		}

		saveFile();

#ifdef ENABLE_X11
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
			Hotkeys::updateCustomKeys();
#endif
	}
} // namespace Settings

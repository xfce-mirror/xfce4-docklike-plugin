/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include <unordered_set>

#include "AppInfos.hpp"
#include "Settings.hpp"

void AppInfo::launch()
{
	GDesktopAppInfo* info = g_desktop_app_info_new_from_filename(this->path.c_str());

	if (info != NULL)
	{
		GdkAppLaunchContext* context = gdk_display_get_app_launch_context(Plugin::mDisplay);

		g_app_info_launch(G_APP_INFO(info), NULL, G_APP_LAUNCH_CONTEXT(context), NULL);

		g_object_unref(context);
		g_object_unref(info);
	}
}

void AppInfo::launch_action(const gchar* action)
{
	GDesktopAppInfo* info = g_desktop_app_info_new_from_filename(this->path.c_str());

	if (info != NULL)
	{
		GdkAppLaunchContext* context = gdk_display_get_app_launch_context(Plugin::mDisplay);

		g_desktop_app_info_launch_action(info, action, G_APP_LAUNCH_CONTEXT(context));

		g_object_unref(context);
		g_object_unref(info);
	}
}

void AppInfo::edit()
{
	gchar* command = g_strconcat("exo-desktop-item-edit ", g_shell_quote(this->path.c_str()), NULL);

	if (g_spawn_command_line_sync(command, NULL, NULL, NULL, NULL))
	{
		// If a new desktop file was created, it will be in $XDG_DATA_HOME
		// If the previous file was pinned, it needs to be replaced with the new one.
		gchar *basename = g_strdup_printf("%s.desktop", this->icon.c_str());
		gchar* newPath = g_build_filename(g_get_user_data_dir(), "applications", basename, NULL);

		if (this->path.compare(newPath) != 0 && g_file_test(newPath, G_FILE_TEST_IS_REGULAR))
		{
			std::list<std::string> pinnedApps = Settings::pinnedAppList;
			for (auto it = pinnedApps.begin(); it != pinnedApps.end();)
			{
				if (*it == this->path)
				{
					it = pinnedApps.erase(it);
					pinnedApps.insert(it, newPath);
					break;
				}
				++it;
			}

			Settings::pinnedAppList.set(pinnedApps);
		}

		g_free(newPath);
		g_free(basename);
	}

	g_free(command);
}

namespace AppInfos
{
	std::list<std::string> mXdgDataDirs;
	Store::Map<const std::string, AppInfo*> mAppInfoWMClasses;
	Store::Map<const std::string, AppInfo*> mAppInfoIds;
	Store::Map<const std::string, AppInfo*> mAppInfoNames;
	GAppInfoMonitor* mMonitor;

	static void findXDGDirectories()
	{
		std::unordered_set<std::string> dir_set;
		std::list<std::string> dir_list, topdir_list;

		dir_list.push_back(g_get_user_data_dir());
		for (const gchar *const *p = g_get_system_data_dirs(); *p != NULL; p++)
			dir_list.push_back(*p);

		for (std::string& dir : dir_list)
		{
			if (dir.back() == '/')
				dir += "applications/";
			else
				dir += "/applications/";

			// ensure uniqueness and existing dirs
			if (dir_set.insert(dir).second && g_file_test(dir.c_str(), G_FILE_TEST_IS_DIR))
				topdir_list.push_back(dir);
		}

		for (std::string& dir : topdir_list)
		{
			// Recursively add subdirectories of mXdgDataDirs to mXdgDataDirs.
			// Wine (and maybe some others) create their own directory tree.
			// See man ftw(3) for more information.
			ftw(
				dir.c_str(),
				[](const char* fpath, const struct stat* sb, int typeflag) -> int
				{
					if (typeflag == FTW_D)
						mXdgDataDirs.push_back(std::string(fpath) + '/');
					return 0;
				},
				16);
		}
	}

	static void loadDesktopEntry(const std::string& xdgDir, std::string filename)
	{
		#define DOT_DESKTOP ".desktop"
		constexpr size_t DOT_DESKTOP_SIZE = 8;

		if (!g_str_has_suffix(filename.c_str(), DOT_DESKTOP))
			return;

		std::string id = filename.substr(0, filename.size() - DOT_DESKTOP_SIZE);
		std::string lower_id = Help::String::toLowercase(id);
		if (mAppInfoIds.get(lower_id) != NULL)
			return;

		std::string path = xdgDir + filename;
		GDesktopAppInfo* gAppInfo = g_desktop_app_info_new_from_filename(path.c_str());
		if (gAppInfo == NULL)
			return;

		char* name_ = g_desktop_app_info_get_locale_string(gAppInfo, "Name");
		std::string name = (name_ != NULL) ? name_ : id;

		char* icon_ = g_desktop_app_info_get_string(gAppInfo, "Icon");
		std::string icon = (icon_ != NULL) ? icon_ : "";

		// g_desktop_app_info_list_actions always returns non-NULL
		AppInfo* info = new AppInfo({path, icon, name, g_desktop_app_info_list_actions(gAppInfo)});

		mAppInfoIds.set(lower_id, info);
		mAppInfoIds.set(path, info); // for saved pinned groups

		if (!name.empty())
		{
			name = Help::String::toLowercase(Help::String::trim(name));

			if (name.find(' ') == std::string::npos)
				if (name != id)
					mAppInfoNames.set(name, info);
		}

		std::string exec;
		char* exec_ = g_desktop_app_info_get_string(gAppInfo, "Exec");
		if (exec_ != NULL && exec_[0] != '\0')
		{
			std::string execLine = Help::String::toLowercase(Help::String::pathBasename(Help::String::trim(exec_)));
			exec = Help::String::getWord(execLine, 0);

			if (exec != "env" && exec != "exo-open")
				if (exec != id && exec != name)
					mAppInfoNames.set(Help::String::toLowercase(exec), info);
		}

		std::string wmclass;
		char* wmclass_ = g_desktop_app_info_get_string(gAppInfo, "StartupWMClass");
		if (wmclass_ != NULL && wmclass_[0] != '\0')
		{
			wmclass = Help::String::toLowercase(Help::String::trim(wmclass_));
			mAppInfoWMClasses.set(wmclass, info);
		}
	}

	static void loadXDGDirectories()
	{
		for (const std::string& xdgDir : mXdgDataDirs)
		{
			DIR* directory = opendir(xdgDir.c_str());
			if (directory == NULL)
				continue;

			struct dirent* entry;
			while ((entry = readdir(directory)) != NULL)
				loadDesktopEntry(xdgDir, entry->d_name);

			closedir(directory);
			PANEL_DEBUG("APPDIR: %s", xdgDir.c_str());
		}
	}

	void init()
	{
		mMonitor = g_app_info_monitor_get();

		g_signal_connect(G_OBJECT(mMonitor), "changed",
			G_CALLBACK(+[](GAppInfoMonitor* monitor)
				{
					mAppInfoIds.clear();
					mAppInfoNames.clear();
					mAppInfoWMClasses.clear();
					loadXDGDirectories();
					Dock::drawGroups();
				}),
			NULL);

		findXDGDirectories();
		loadXDGDirectories();
	}

	// TODO: Load these from a file so that the user can add their own aliases
	std::map<std::string, std::string> mGroupNameRename = {
		{"soffice", "libreoffice-startcenter"},
		{"libreoffice", "libreoffice-startcenter"},
		{"radium_linux.bin", "radium"},
		{"viberpc", "viber"},
		{"multimc5", "multimc"},
	};

	static void groupNameTransform(std::string& groupName)
	{
		std::map<std::string, std::string>::iterator itRenamed;
		if ((itRenamed = mGroupNameRename.find(groupName)) != mGroupNameRename.end())
			groupName = itRenamed->second;
	}

	AppInfo* search(std::string id)
	{
		groupNameTransform(id);

		AppInfo* ai = mAppInfoWMClasses.get(id);
		if (ai != NULL)
			return ai;

		ai = mAppInfoIds.get(id);
		if (ai != NULL)
			return ai;

		// Try to use just the first word of the window class; so that
		// virtualbox manager, virtualbox machine get grouped together etc.
		auto pos = id.find(' ');
		if (pos != std::string::npos)
		{
			id = id.substr(0, pos);
			ai = mAppInfoIds.get(id);

			if (ai != NULL)
				return ai;
		}

		gchar*** gioPath = g_desktop_app_info_search(id.c_str());

		if (gioPath != NULL && gioPath[0] != NULL && gioPath[0][0] != NULL && gioPath[0][0][0] != '\0')
		{
			std::string gioId = gioPath[0][0];
			gioId = Help::String::toLowercase(gioId.substr(0, gioId.size() - 8));
			ai = mAppInfoIds.get(gioId);

			for (int i = 0; gioPath[i] != NULL; ++i)
				g_strfreev(gioPath[i]);

			g_free(gioPath);

			if (ai != NULL)
				return ai;
		}

		PANEL_DEBUG("NO MATCH: %s", id.c_str());

		return new AppInfo({"", "", id});
	}
} // namespace AppInfos

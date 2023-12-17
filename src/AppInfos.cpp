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

	if (info != nullptr)
	{
		GdkAppLaunchContext* context = gdk_display_get_app_launch_context(Plugin::mDisplay);

		g_app_info_launch(G_APP_INFO(info), nullptr, G_APP_LAUNCH_CONTEXT(context), nullptr);

		g_object_unref(context);
		g_object_unref(info);
	}
}

void AppInfo::launch_action(const gchar* action)
{
	GDesktopAppInfo* info = g_desktop_app_info_new_from_filename(this->path.c_str());

	if (info != nullptr)
	{
		GdkAppLaunchContext* context = gdk_display_get_app_launch_context(Plugin::mDisplay);

		g_desktop_app_info_launch_action(info, action, G_APP_LAUNCH_CONTEXT(context));

		g_object_unref(context);
		g_object_unref(info);
	}
}

void AppInfo::edit()
{
	GError* error = nullptr;
	gchar* quoted = g_shell_quote(this->path.c_str());
	gchar* command = g_strconcat("exo-desktop-item-edit ", quoted, nullptr);

	if (!g_spawn_command_line_async(command, &error))
	{
		g_warning("Failed to open edit dialog: %s", error->message);
		g_error_free(error);
	}

	g_free(command);
	g_free(quoted);
}

namespace AppInfos
{
	std::list<std::string> mXdgDataDirs;
	Store::Map<const std::string, std::shared_ptr<AppInfo>> mAppInfoWMClasses;
	Store::Map<const std::string, std::shared_ptr<AppInfo>> mAppInfoIds;
	Store::Map<const std::string, std::shared_ptr<AppInfo>> mAppInfoNames;
	Store::AutoPtr<GAppInfoMonitor> mMonitor;

	static void findXDGDirectories()
	{
		std::unordered_set<std::string> dir_set;
		std::list<std::string> dir_list, topdir_list;

		dir_list.push_back(g_get_user_data_dir());
		for (const gchar *const *p = g_get_system_data_dirs(); *p != nullptr; p++)
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
		if (mAppInfoIds.get(lower_id) != nullptr)
			return;

		std::string path = xdgDir + filename;
		GDesktopAppInfo* gAppInfo = g_desktop_app_info_new_from_filename(path.c_str());
		if (gAppInfo == nullptr)
			return;

		char* name_ = g_desktop_app_info_get_locale_string(gAppInfo, "Name");
		std::string name = (name_ != nullptr) ? name_ : id;
		g_free(name_);

		char* icon_ = g_desktop_app_info_get_string(gAppInfo, "Icon");
		std::string icon = (icon_ != nullptr) ? icon_ : "";
		g_free(icon_);

		std::shared_ptr<AppInfo> info = std::make_shared<AppInfo>(id, path, icon, name, gAppInfo);
		mAppInfoIds.set(lower_id, info);

		if (!name.empty())
		{
			name = Help::String::toLowercase(Help::String::trim(name));

			if (name.find(' ') == std::string::npos)
				if (name != id)
					mAppInfoNames.set(name, info);
		}

		std::string exec;
		char* exec_ = g_desktop_app_info_get_string(gAppInfo, "Exec");
		if (exec_ != nullptr && exec_[0] != '\0')
		{
			std::string execLine = Help::String::toLowercase(Help::String::pathBasename(Help::String::trim(exec_)));
			exec = Help::String::getWord(execLine, 0);

			if (exec != "env" && exec != "exo-open")
				if (exec != id && exec != name)
					mAppInfoNames.set(Help::String::toLowercase(exec), info);
		}
		g_free(exec_);

		std::string wmclass;
		char* wmclass_ = g_desktop_app_info_get_string(gAppInfo, "StartupWMClass");
		if (wmclass_ != nullptr && wmclass_[0] != '\0')
		{
			wmclass = Help::String::toLowercase(Help::String::trim(wmclass_));
			mAppInfoWMClasses.set(wmclass, info);
		}
		g_free(wmclass_);
	}

	static void loadXDGDirectories()
	{
		for (const std::string& xdgDir : mXdgDataDirs)
		{
			DIR* directory = opendir(xdgDir.c_str());
			if (directory == nullptr)
				continue;

			struct dirent* entry;
			while ((entry = readdir(directory)) != nullptr)
				loadDesktopEntry(xdgDir, entry->d_name);

			closedir(directory);
			g_debug("APPDIR: %s", xdgDir.c_str());
		}
	}

	void init()
	{
		mMonitor = Store::AutoPtr<GAppInfoMonitor>(g_app_info_monitor_get(), g_object_unref);

		g_signal_connect(G_OBJECT(mMonitor.get()), "changed",
			G_CALLBACK(+[](GAppInfoMonitor* monitor)
				{
					mAppInfoIds.clear();
					mAppInfoNames.clear();
					mAppInfoWMClasses.clear();
					loadXDGDirectories();
					Dock::drawGroups();
				}),
			nullptr);

		findXDGDirectories();
		loadXDGDirectories();
	}

	void finalize()
	{
		mXdgDataDirs.clear();
		mAppInfoWMClasses.clear();
		mAppInfoIds.clear();
		mAppInfoNames.clear();
		mMonitor.reset();
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

	std::shared_ptr<AppInfo> search(std::string id)
	{
		groupNameTransform(id);

		std::shared_ptr<AppInfo> ai = mAppInfoWMClasses.get(id);
		if (ai != nullptr)
			return ai;

		ai = mAppInfoIds.get(id);
		if (ai != nullptr)
			return ai;

		// Try to use just the first word of the window class; so that
		// virtualbox manager, virtualbox machine get grouped together etc.
		auto pos = id.find(' ');
		if (pos != std::string::npos)
		{
			id = id.substr(0, pos);
			ai = mAppInfoIds.get(id);

			if (ai != nullptr)
				return ai;
		}

		gchar*** gioPath = g_desktop_app_info_search(id.c_str());

		if (gioPath[0] != nullptr && gioPath[0][0] != nullptr && gioPath[0][0][0] != '\0')
		{
			std::string gioId = gioPath[0][0];
			gioId = Help::String::toLowercase(gioId.substr(0, gioId.size() - 8));
			ai = mAppInfoIds.get(gioId);
		}

		for (int i = 0; gioPath[i] != nullptr; ++i)
			g_strfreev(gioPath[i]);
		g_free(gioPath);

		if (ai != nullptr)
			return ai;

		g_debug("NO MATCH: %s", id.c_str());

		return std::make_shared<AppInfo>("", "", "", id);
	}
} // namespace AppInfos

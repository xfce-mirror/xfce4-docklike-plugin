/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "AppInfos.hpp"
#include "Settings.hpp"

void AppInfo::launch()
{
	GDesktopAppInfo* info = g_desktop_app_info_new_from_filename(this->path.c_str());
	
	if (info != NULL)
		g_app_info_launch((GAppInfo*)info, NULL, NULL, NULL);
}

void AppInfo::launch_action(const gchar* action)
{
	GDesktopAppInfo* info = g_desktop_app_info_new_from_filename(this->path.c_str());
	
	if (info != NULL)
		g_desktop_app_info_launch_action(info, action, NULL);
}

void AppInfo::edit()
{
	gchar* command = g_strconcat("exo-desktop-item-edit ", g_shell_quote(this->path.c_str()), NULL);

	if (g_spawn_command_line_sync(command, NULL, NULL, NULL, NULL))
	{
		// If a new desktop file was created, it will be in ~/.local/share/applications
		// If the previous file was pinned it needs to be replaced with the new one.
		gchar* newPath = g_build_filename(getenv("HOME"), "/.local/share/applications/",
			g_strdup_printf("%s.desktop", this->icon.c_str()), NULL);
		
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
	}
}

namespace AppInfos
{
	std::list<std::string> mXdgDataDirs;
	Store::Map<const std::string, AppInfo*> mAppInfoWMClasses;
	Store::Map<const std::string, AppInfo*> mAppInfoIds;
	Store::Map<const std::string, AppInfo*> mAppInfoNames;
	pthread_mutex_t AppInfosLock;
	bool modified;

	void findXDGDirectories()
	{
		char* var = getenv("XDG_DATA_DIRS");

		if (var != NULL && var[0] != '\0')
			Help::String::split(var, mXdgDataDirs, ':');

		mXdgDataDirs.push_back("/usr/local/share");
		mXdgDataDirs.push_back("/usr/share");
		mXdgDataDirs.push_back(std::string(getenv("HOME")) + "/.local/share");

		for (std::string& dir : mXdgDataDirs)
			if (dir.back() == '/')
				dir += "applications/";
			else
				dir += "/applications/";

		std::list<std::string> tempDirs = mXdgDataDirs;
		for (std::string& dir : tempDirs)
		{
			if (!g_file_test(dir.c_str(), G_FILE_TEST_IS_DIR))
			{
				mXdgDataDirs.remove(dir);
				continue;
			}

			// Recursively add subdirectories of mXdgDataDirs to mXdgDataDirs.
			// Wine (and maybe some others) create their own directory tree.
			// See man ftw(3) for more information.
			ftw(
				dir.c_str(),
				[](const char* fpath, const struct stat* sb, int typeflag) -> int {
					if (typeflag == FTW_D)
						mXdgDataDirs.push_back(g_strdup_printf("%s/", fpath));
					return 0; },
				1);
		}

		mXdgDataDirs.sort();
		mXdgDataDirs.unique();
	}

	void loadDesktopEntry(const std::string& xdgDir, std::string filename)
	{
		std::string id = filename.substr(0, filename.size() - 8);
		std::string path = xdgDir + id + ".desktop";

		GDesktopAppInfo* gAppInfo = g_desktop_app_info_new_from_filename(path.c_str());

		if (gAppInfo == NULL)
			return;

		pthread_mutex_lock(&AppInfosLock);

		std::string icon;
		char* icon_ = g_desktop_app_info_get_string(gAppInfo, "Icon");
		if (icon_ == NULL)
		{
			pthread_mutex_unlock(&AppInfosLock);
			return;
		}
		icon = Help::String::trim(icon_);
		
		std::string name;
		char* name_ = g_desktop_app_info_get_string(gAppInfo, "Name");

		if (name_ != NULL)
			name = name_;

		const gchar* const* actions = g_desktop_app_info_list_actions(gAppInfo);
		AppInfo* info = new AppInfo({path, icon, name, actions});

		id = Help::String::toLowercase(id);
		mAppInfoIds.set(id, info);
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

			if (wmclass != id && wmclass != name && wmclass != exec)
				mAppInfoWMClasses.set(wmclass, info);
		}

		pthread_mutex_unlock(&AppInfosLock);
	}

	void removeDesktopEntry(const std::string& xdgDir, std::string filename)
	{
		std::string id = filename.substr(0, filename.size() - 8);

		pthread_mutex_lock(&AppInfosLock);

		mAppInfoIds.remove(id);
		mAppInfoNames.remove(id);
		mAppInfoWMClasses.remove(id);

		pthread_mutex_unlock(&AppInfosLock);
	}

	void* threadedXDGDirectoryWatcher(void* dirPath)
	{
		int i = 0;
		int len = 0;
		char buf[1024];
		struct inotify_event* event;
		int fd = inotify_init();

		inotify_add_watch(fd, ((std::string*)dirPath)->c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MODIFY);

		while (true)
		{
			i = 0;
			len = read(fd, buf, 1024);

			while (i < len)
			{
				event = (struct inotify_event*)&buf[i];
				i += sizeof(struct inotify_event) + event->len;
			}

			std::string filename = event->name;
			if (filename.substr(filename.size() - 8, std::string::npos) == ".desktop")
			{
				if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM)
				{
					removeDesktopEntry(*(std::string*)dirPath, filename);

					if (PANEL_DEBUG)
						g_print("REMOVED: %s%s\n", ((std::string*)dirPath)->c_str(), filename.c_str());

					std::list<std::string> pinnedApps = Settings::pinnedAppList;
					pinnedApps.remove(*(std::string*)dirPath);
					Settings::pinnedAppList.set(pinnedApps);
				}
				else
				{
					loadDesktopEntry(*(std::string*)dirPath, filename);

					if (PANEL_DEBUG)
						g_print("UPDATED: %s%s\n", ((std::string*)dirPath)->c_str(), filename.c_str());
				}
			}
			modified = true;
		}
	}

	void watchXDGDirectory(std::string xdgDir)
	{
		pthread_t thread_store;
		std::string* arg = new std::string(xdgDir);

		pthread_create(&thread_store, NULL, threadedXDGDirectoryWatcher, arg);
	}

	void loadXDGDirectories()
	{
		for (std::string xdgDir : mXdgDataDirs)
		{
			DIR* directory = opendir(xdgDir.c_str());
			if (directory == NULL)
				continue;

			struct dirent* entry;
			while ((entry = readdir(directory)) != NULL)
				loadDesktopEntry(xdgDir, entry->d_name);
			watchXDGDirectory(xdgDir);

			if (PANEL_DEBUG)
				g_print("APPDIR: %s\n", xdgDir.c_str());
		}
	}

	void init()
	{
		modified = false;
		pthread_mutex_init(&AppInfosLock, NULL);
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

	void groupNameTransform(std::string& groupName)
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
		//virtualbox manager, virtualbox machine get grouped together etc.
		uint pos = id.find(' ');
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

		if (PANEL_DEBUG)
			g_print("NO MATCH: %s\n", id.c_str());

		return new AppInfo({"", "", id});
	}

} // namespace AppInfos

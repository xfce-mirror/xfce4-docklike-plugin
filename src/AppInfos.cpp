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

#include "AppInfos.hpp"
#include "Settings.hpp"

#include <libxfce4ui/libxfce4ui.h>

#include <unordered_set>

void AppInfo::launch()
{
	if (mGAppInfo)
	{
		GError* error = nullptr;
		GdkAppLaunchContext* context = gdk_display_get_app_launch_context(Plugin::mDisplay);

		if (!g_app_info_launch(G_APP_INFO(mGAppInfo.get()), nullptr, G_APP_LAUNCH_CONTEXT(context), &error))
		{
			g_warning("Failed to launch app '%s': %s", mName.c_str(), error->message);
			g_error_free(error);
		}
		g_object_unref(context);
	}
}

void AppInfo::launchAction(const gchar* action)
{
	if (mGAppInfo)
	{
		GdkAppLaunchContext* context = gdk_display_get_app_launch_context(Plugin::mDisplay);
		g_desktop_app_info_launch_action(mGAppInfo.get(), action, G_APP_LAUNCH_CONTEXT(context));
		g_object_unref(context);
	}
}

void AppInfo::edit()
{
	GError* error = nullptr;
	gchar* quoted = g_shell_quote(mPath.c_str());
#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
	gchar* command = g_strconcat("xfce-desktop-item-edit ", quoted, nullptr);
#else
	gchar* command = g_strconcat("exo-desktop-item-edit ", quoted, nullptr);
#endif

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
	Store::Map<const std::string, std::shared_ptr<AppInfo>> mAppInfoUserSet;
	Store::AutoPtr<GAppInfoMonitor> mMonitor;

	static void findXDGDirectories()
	{
		std::unordered_set<std::string> dir_set;
		std::list<std::string> dir_list, topdir_list;

		dir_list.push_back(g_get_user_data_dir());
		for (const gchar* const* p = g_get_system_data_dirs(); *p != nullptr; p++)
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
				[](const char* fpath, const struct stat* sb, int typeflag) -> int {
					if (typeflag == FTW_D)
						mXdgDataDirs.push_back(std::string(fpath) + '/');
					return 0;
				},
				16);
		}
	}

	std::unordered_set<std::string> mExcludedBinaries = {
		// clang-format off
		"env", "exo-open", "xfce-open", "xdg-open",
		"bash", "dash", "ksh", "sh", "tcsh", "zsh",
		// clang-format on
	};

	static void loadDesktopEntry(const std::string& xdgDir, std::string filename)
	{
		if (!g_str_has_suffix(filename.c_str(), ".desktop"))
			return;

		std::string id = Help::String::pathBasename(filename, true);
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

		name_ = g_desktop_app_info_get_string(gAppInfo, "Name");
		name = (name_ != nullptr) ? name_ : "";
		g_free(name_);

		if (!name.empty())
		{
			name = Help::String::toLowercase(Help::String::trim(name));

			if (name.find(' ') == std::string::npos)
				if (name != lower_id)
					mAppInfoNames.set(name, info);
		}

		std::string exec;
		char* exec_ = g_desktop_app_info_get_string(gAppInfo, "Exec");
		if (exec_ != nullptr && exec_[0] != '\0')
		{
			std::string execLine = Help::String::toLowercase(Help::String::pathBasename(Help::String::trim(exec_)));
			exec = Help::String::getWord(execLine, 0);

			if (exec != lower_id && exec != name && mExcludedBinaries.find(exec) == mExcludedBinaries.end())
				mAppInfoNames.set(exec, info);
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

	static bool addUserSetApp(std::string classId, std::string filename)
	{
		loadDesktopEntry(Help::String::pathDirname(filename), Help::String::pathBasename(filename));

		std::string id = Help::String::toLowercase(Help::String::pathBasename(filename, true));
		std::shared_ptr<AppInfo> info = mAppInfoIds.get(id);
		if (info != nullptr)
		{
			mAppInfoUserSet.set(Help::String::toLowercase(classId), info);
			g_debug("Added user-set app '%s' for launcher '%s'", classId.c_str(), filename.c_str());
			return true;
		}

		g_debug("Failed to add user-set app '%s' for launcher '%s'", classId.c_str(), filename.c_str());
		return false;
	}

	void init()
	{
		mMonitor = Store::AutoPtr<GAppInfoMonitor>(g_app_info_monitor_get(), g_object_unref);

		g_signal_connect(G_OBJECT(mMonitor.get()), "changed",
			G_CALLBACK(+[](GAppInfoMonitor* monitor) {
				mAppInfoIds.clear();
				mAppInfoNames.clear();
				mAppInfoWMClasses.clear();
				loadXDGDirectories();
				Dock::drawGroups();
			}),
			nullptr);

		findXDGDirectories();
		loadXDGDirectories();

		std::list<std::string> ids = Settings::userSetApps.get().first;
		std::list<std::string> paths = Settings::userSetApps.get().second;
		for (auto id = ids.begin(), path = paths.begin();
			id != ids.end() && path != paths.end();
			id++, path++)
		{
			addUserSetApp(*id, *path);
		}
	}

	void finalize()
	{
		mXdgDataDirs.clear();
		mAppInfoWMClasses.clear();
		mAppInfoIds.clear();
		mAppInfoNames.clear();
		mAppInfoUserSet.clear();
		mMonitor.reset();
	}

	// some aliases we are obliged to use: these should be reserved for our apps
	// and restricted as much as possible
	std::map<std::string, std::string> mIdMap = {
		// the best we can do is to group all panel/plugin dialogs together
		{"xfce4-panel", "panel-preferences"},
		{"wrapper-2.0", "panel-preferences"},
	};

	static void translateId(std::string& id)
	{
		auto it = mIdMap.find(id);
		if (it != mIdMap.end())
			id = it->second;
	}

	std::shared_ptr<AppInfo> search(std::string id)
	{
		translateId(id);

		g_debug("Searching a match for '%s'", id.c_str());

		std::shared_ptr<AppInfo> ai = mAppInfoWMClasses.get(id);
		if (ai != nullptr)
		{
			g_debug("App WMClass match");
			return ai;
		}

		ai = mAppInfoIds.get(id);
		if (ai != nullptr)
		{
			g_debug("App id match");
			return ai;
		}

		ai = mAppInfoNames.get(id);
		if (ai != nullptr)
		{
			g_debug("App name match");
			return ai;
		}

		// Try to use just the first word of the window class; so that
		// virtualbox manager, virtualbox machine get grouped together etc.
		auto pos = id.find(' ');
		if (pos != std::string::npos)
		{
			id = id.substr(0, pos);
			g_debug("No match for whole string, searching a match for first word '%s'", id.c_str());

			ai = mAppInfoIds.get(id);
			if (ai != nullptr)
			{
				g_debug("App id match");
				return ai;
			}

			ai = mAppInfoNames.get(id);
			if (ai != nullptr)
			{
				g_debug("App name match");
				return ai;
			}
		}

		ai = mAppInfoUserSet.get(id);
		if (ai != nullptr)
		{
			g_debug("User-set app match");
			return ai;
		}

		g_debug("No match");

		return std::make_shared<AppInfo>("", "", "", id);
	}

	bool selectLauncher(const gchar* classId)
	{
		GtkWidget* dialog = gtk_file_chooser_dialog_new(
			_("Select Launcher"), nullptr, GTK_FILE_CHOOSER_ACTION_OPEN,
			_("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, nullptr);
		GtkFileFilter* filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, "*.desktop");
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), true);

		if (g_file_test("/usr/share/applications", G_FILE_TEST_IS_DIR))
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "/usr/share/applications");
		else
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), mXdgDataDirs.front().c_str());
		for (auto dir : mXdgDataDirs)
		{
			if (g_str_has_suffix(dir.c_str(), "/applications/"))
				gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog), dir.c_str(), nullptr);
		}

		bool added = false;
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			if (filename != nullptr)
			{
				added = addUserSetApp(classId, filename);
				if (added)
				{
					std::list<std::string> ids = Settings::userSetApps.get().first;
					std::list<std::string> paths = Settings::userSetApps.get().second;
					ids.push_back(classId);
					paths.push_back(filename);
					Settings::userSetApps.set(std::pair<std::list<std::string>, std::list<std::string>>(ids, paths));
				}
				g_free(filename);
			}
		}

		gtk_widget_destroy(dialog);

		return added;
	}

	void createLauncher(const gchar* classId)
	{
		GError* error = nullptr;
		gchar* basename = g_strconcat(classId, ".desktop", nullptr);
		gchar* path = g_build_filename(g_get_user_data_dir(), "applications", basename, nullptr);
		gchar* quotedPath = g_shell_quote(path);
		gchar* quotedId = g_shell_quote(classId);
#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
		gchar* command = g_strdup_printf("xfce-desktop-item-edit --create-new --name %s %s", quotedId, quotedPath);
#else
		gchar* command = g_strdup_printf("exo-desktop-item-edit --create-new --name %s %s", quotedId, quotedPath);
#endif

		if (!g_spawn_command_line_async(command, &error))
		{
			g_warning("Failed to open create launcher dialog: %s", error->message);
			g_error_free(error);
		}

		g_free(command);
		g_free(quotedId);
		g_free(quotedPath);
		g_free(path);
		g_free(basename);
	}
} // namespace AppInfos

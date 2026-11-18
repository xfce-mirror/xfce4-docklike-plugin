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

#ifndef APPINFOS_HPP
#define APPINFOS_HPP

#include "Plugin.hpp"
#include "Store.ipp"

#include <gio/gdesktopappinfo.h>

#include <memory>
#include <string>

class Plugin;

class AppInfo
{
public:
	AppInfo(std::string id, std::string path, std::string icon, std::string name, GDesktopAppInfo* gAppInfo = nullptr) : mId(id), mPath(path), mIcon(icon), mName(name), mGAppInfo(gAppInfo, [](gpointer o) { if (o) g_object_unref(o); }) {}
	const gchar* const* getActions() { return mGAppInfo ? g_desktop_app_info_list_actions(mGAppInfo.get()) : nullptr; };
	std::string getId() { return mId.empty() ? mName : mId; };
	void launch();
	void launchAction(const gchar* action);
	void edit();

	const std::string mId;
	const std::string mPath;
	const std::string mIcon;
	const std::string mName;
	const Store::AutoPtr<GDesktopAppInfo> mGAppInfo;
};

class AppInfos
{
public:
	explicit AppInfos(Plugin* plugin);

	std::shared_ptr<AppInfo> search(std::string id);
	bool selectLauncher(const gchar* classId);
	void createLauncher(const gchar* classId);

private:
	void findXDGDirectories();
	void loadDesktopEntry(const std::string& xdgDir, const std::string& filename);
	void loadXDGDirectories();
	bool addUserSetApp(const std::string& classId, const std::string& filename);
	std::shared_ptr<AppInfo> searchByPrefix(const std::string& id, char sep);

	Plugin* mPlugin;
	std::list<std::string> mXdgDataDirs;
	Store::Map<const std::string, std::shared_ptr<AppInfo>> mAppInfoWMClasses;
	Store::Map<const std::string, std::shared_ptr<AppInfo>> mAppInfoIds;
	Store::Map<const std::string, std::shared_ptr<AppInfo>> mAppInfoNames;
	Store::Map<const std::string, std::shared_ptr<AppInfo>> mAppInfoUserSet;
	Store::AutoPtr<GAppInfoMonitor> mMonitor;
};

#endif // APPINFOS_HPP

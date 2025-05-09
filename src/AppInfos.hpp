/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef APPINFOS_HPP
#define APPINFOS_HPP

#include "Helpers.hpp"
#include "Store.ipp"

#include <ftw.h>
#include <gio/gdesktopappinfo.h>

#include <iostream>
#include <memory>

struct AppInfo
{
	const std::string mId;
	const std::string mPath;
	const std::string mIcon;
	const std::string mName;
	const Store::AutoPtr<GDesktopAppInfo> mGAppInfo;

	AppInfo(std::string id, std::string path, std::string icon, std::string name, GDesktopAppInfo* gAppInfo = nullptr)
		: mId(id), mPath(path), mIcon(icon), mName(name), mGAppInfo(gAppInfo, [](gpointer o) { if (o) g_object_unref(o); }) {}
	const gchar* const* getActions() { return mGAppInfo ? g_desktop_app_info_list_actions(mGAppInfo.get()) : nullptr; };
	void launch();
	void launchAction(const gchar* action);
	void edit();
};

namespace AppInfos
{
	void init();
	void finalize();
	std::shared_ptr<AppInfo> search(std::string id);
	bool selectLauncher(const gchar* classId);
	void createLauncher(const gchar* classId);
} // namespace AppInfos

#endif // APPINFOS_HPP

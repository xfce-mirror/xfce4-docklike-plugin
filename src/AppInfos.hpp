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
	const std::string id;
	const std::string path;
	const std::string icon;
	const std::string name;
	const Store::AutoPtr<GDesktopAppInfo> gAppInfo;

	AppInfo(std::string _id, std::string _path, std::string _icon, std::string _name, GDesktopAppInfo* _gAppInfo = nullptr)
		: id(_id), path(_path), icon(_icon), name(_name), gAppInfo(_gAppInfo, [](gpointer o) { if (o) g_object_unref(o); }) {}
	const gchar* const* get_actions() { return gAppInfo ? g_desktop_app_info_list_actions(gAppInfo.get()) : nullptr; };
	void launch();
	void launch_action(const gchar* action);
	void edit();
};

namespace AppInfos
{
	void init();
	void finalize();
	std::shared_ptr<AppInfo> search(std::string id);
	bool selectLauncher(const gchar* classId);
} // namespace AppInfos

#endif // APPINFOS_HPP

/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef APPINFOS_HPP
#define APPINFOS_HPP

#include <ftw.h>

#include <gio/gdesktopappinfo.h>

#include <iostream>
#include <memory>

#include "Helpers.hpp"
#include "Store.tpp"

struct AppInfo
{
	const std::string id;
	const std::string path;
	const std::string icon;
	const std::string name;
	const Store::AutoPtr<GDesktopAppInfo> gAppInfo;

	AppInfo(std::string _id, std::string _path, std::string _icon, std::string _name, GDesktopAppInfo* _gAppInfo = NULL)
		: id(_id), path(_path), icon(_icon), name(_name), gAppInfo(_gAppInfo, [](gpointer o) { if (o) g_object_unref(o); }) {}
	const gchar* const* get_actions() { return gAppInfo ? g_desktop_app_info_list_actions(gAppInfo.get()) : NULL; };
	void launch();
	void launch_action(const gchar* action);
	void edit();
};

namespace AppInfos
{
	void init();
	std::shared_ptr<AppInfo> search(std::string id);
} // namespace AppInfos

#endif // APPINFOS_HPP

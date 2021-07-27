/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef APPINFOS_HPP
#define APPINFOS_HPP

#include <ftw.h>
#include <pthread.h>
#include <sys/inotify.h>

#include <gio/gdesktopappinfo.h>

#include <iostream>

#include "Helpers.hpp"
#include "Store.tpp"

struct AppInfo
{
	const std::string path;
	const std::string icon;
	const std::string name;
	const gchar* const* actions;

	void launch();
	void launch_action(const gchar* action);
	void edit();
};

namespace AppInfos
{
	void init();
	AppInfo* search(std::string id);

	extern bool modified;
} // namespace AppInfos

#endif // APPINFOS_HPP

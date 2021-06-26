/*
 * Docklike Taskbar - A modern, minimalist taskbar for XFCE
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef GARCON_HPP
#define GARCON_HPP

#include <garcon/garcon.h>
#include <gio/gdesktopappinfo.h>

#include "Helpers.hpp"
#include "Store.tpp"

namespace Garcon
{
	extern std::list<GarconMenuItem*> menuItems;

	void init();
} // namespace Garcon

#endif
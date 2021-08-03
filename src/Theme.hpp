/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef THEME_HPP
#define THEME_HPP

#define DEFAULT_THEME ".menu { margin: 0; padding: 0; border: 0; background-color: @menu_bgcolor; }\n" \
					  ".hover_menu_item { background-color: alpha(@menu_item_color_hover, 0.2); }\n"   \
					  ".active_group { background-color: alpha(@menu_item_bgcolor_hover, 0.25); }\n"   \
					  ".hover_group { background-color: alpha(@menu_item_bgcolor_hover, 0.1); }\n";

#include <gtk/gtk.h>

#include <iostream>
#include <string>

#include "Dock.hpp"

namespace Theme
{
	void init();
	void load();
	std::string get_theme_colors();
} // namespace Theme

#endif // THEME_HPP

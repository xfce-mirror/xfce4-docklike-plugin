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

#ifndef THEME_HPP
#define THEME_HPP

#define DEFAULT_THEME ".menu { margin: 0; padding: 0; border: 0; background-color: @menu_bgcolor; }\n" \
					  ".hover_menu_item { background-color: alpha(@menu_item_color_hover, 0.2); }\n";

#include "Dock.hpp"

#include <gtk/gtk.h>

#include <iostream>
#include <string>

namespace Theme
{
	void init();
	void load();
	std::string get_theme_colors();
} // namespace Theme

#endif // THEME_HPP

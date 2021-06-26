/*
 * Docklike Taskbar - A modern, minimalist taskbar for XFCE
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Garcon.hpp"

namespace Garcon
{
	std::list<GarconMenuItem*> menuItems;

	void init()
	{
		// Traverse the applications menu and build a list of .desktop files
		GarconMenu* gMenu = garcon_menu_new_applications();
		if (garcon_menu_load(gMenu, NULL, NULL))
		{
			GList* menus = garcon_menu_get_menus(gMenu);
			for (menus; menus != NULL; menus = menus->next)
			{
				GList* items = garcon_menu_get_items(GARCON_MENU(menus->data));
				for (items; items != NULL; items = items->next)
				{
					menuItems.push_back(GARCON_MENU_ITEM(items->data));
				}
				g_list_free(items);
			}
			g_list_free(menus);
		}
		g_object_unref(gMenu);
	}
} // namespace Garcon
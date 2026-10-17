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

#ifndef DOCK_HPP
#define DOCK_HPP

#include "Group.hpp"
#include "Plugin.hpp"
#include "Store.ipp"

#include <gtk/gtk.h>

#include <memory>

class AppInfo;
class Group;
class Plugin;

class Dock
{
public:
	explicit Dock(Plugin* plugin);

	Group* prepareGroup(std::shared_ptr<AppInfo> appInfo);

	void pinApp(std::shared_ptr<AppInfo> appInfo, Group* at = nullptr);
	void pinFromUris(const gchar* const* uris, Group* at = nullptr);

	void moveButton(Group* moving, Group* dest);
	void savePinned();
	void drawGroups();

	void activateGroup(int nb);
	void activateGroup(const std::string& appId);

	void onPanelResize(int size = -1);
	void onPanelOrientationChange(GtkOrientation orientation);

	GtkWidget* mBox;
	Store::KeyStore<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> mGroups;

	int mPanelSize = 0;
	int mIconSize = 0;

private:
	void activateGroup(Group* group);

	Plugin* mPlugin;
};

#endif // DOCK_HPP

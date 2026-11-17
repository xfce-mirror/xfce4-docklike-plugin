/*
 * Copyright (c) 2026 Archisman Panigrahi <apandada1@gmail.com>
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

#ifndef LAUNCHERENTRY_HPP
#define LAUNCHERENTRY_HPP

#include "Plugin.hpp"

#include <gio/gio.h>

#include <cstdint>
#include <map>
#include <string>
#include <utility>

class Dock;
class Group;
class Settings;

class LauncherEntry
{
public:
	LauncherEntry(Plugin* plugin);
	~LauncherEntry();

	void refreshGroups();

private:
	static std::string appIdFromUri(const gchar* appUri);
	void applyEntryToGroup(Group* group);
	static void onLauncherUpdate(GDBusConnection*, const gchar* senderName, const gchar*, const gchar*, const gchar*, GVariant* parameters, gpointer userData);
	static void onNameOwnerChanged(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant* parameters, gpointer userData);

	Plugin* mPlugin;

	GDBusConnection* mConnection = nullptr;
	guint mLauncherSignalId = 0;
	guint mNameOwnerChangedSignalId = 0;
	guint mUnityNameOwnerId = 0;
	std::uint64_t mUpdateSerial = 0;
	using EntryKey = std::pair<std::string, std::string>;
	struct Entry
	{
		gint64 count = 0;
		bool countVisible = false;
		std::uint64_t updateSerial = 0;
	};
	std::map<EntryKey, Entry> mEntries;
};

#endif // LAUNCHERENTRY_HPP

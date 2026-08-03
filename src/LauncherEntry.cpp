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

#include "LauncherEntry.hpp"
#include "Dock.hpp"
#include "Group.hpp"
#include "Settings.hpp"

#include <gio/gio.h>

#include <cstdint>
#include <map>
#include <string>
#include <utility>

struct LauncherEntry::Impl
{
	using EntryKey = std::pair<std::string, std::string>;

	struct Entry
	{
		gint64 count = 0;
		bool countVisible = false;
		std::uint64_t updateSerial = 0;
	};

	GDBusConnection* connection = nullptr;
	guint launcherSignalId = 0;
	guint nameOwnerChangedSignalId = 0;
	guint unityNameOwnerId = 0;
	std::uint64_t updateSerial = 0;
	std::map<EntryKey, Entry> entries;

	static std::string appIdFromUri(const gchar* appUri)
	{
		const gchar prefix[] = "application://";
		if (appUri == nullptr || !g_str_has_prefix(appUri, prefix))
			return "";

		gchar* unescaped = g_uri_unescape_string(appUri + sizeof(prefix) - 1, nullptr);
		if (unescaped == nullptr || unescaped[0] == '\0')
		{
			g_free(unescaped);
			return "";
		}

		gchar* lowercase = g_utf8_strdown(unescaped, -1);
		std::string appId = lowercase;
		g_free(lowercase);
		g_free(unescaped);

		// AppInfos stores desktop file IDs without the .desktop suffix.
		const std::string desktopSuffix = ".desktop";
		if (appId.size() > desktopSuffix.size() && appId.compare(appId.size() - desktopSuffix.size(), desktopSuffix.size(), desktopSuffix) == 0)
			appId.erase(appId.size() - desktopSuffix.size());

		return appId;
	}

	void applyEntryToGroup(Group* group)
	{
		if (Settings::disableLauncherCounts)
		{
			group->setLauncherCount(0, false);
			return;
		}

		const Entry* selected = nullptr;
		std::string groupId = group->mAppInfo == nullptr
			? ""
			: Help::String::toLowercase(group->mAppInfo->mId);

		for (const auto& item : entries)
		{
			const Entry& entry = item.second;
			if (item.first.second == groupId && (selected == nullptr || entry.updateSerial > selected->updateSerial))
				selected = &entry;
		}

		if (selected == nullptr)
			group->setLauncherCount(0, false);
		else
			group->setLauncherCount(selected->count, selected->countVisible);
	}

	void refreshGroups()
	{
		Dock::mGroups.forEach([this](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> group) {
			applyEntryToGroup(group.second.get());
		});
	}

	static void onLauncherUpdate(GDBusConnection*, const gchar* senderName, const gchar*,
		const gchar*, const gchar*, GVariant* parameters, gpointer userData)
	{
		Impl* impl = static_cast<Impl*>(userData);
		if (senderName == nullptr || parameters == nullptr || !g_variant_is_of_type(parameters, G_VARIANT_TYPE("(sa{sv})")))
			return;

		const gchar* appUri;
		GVariant* properties;
		g_variant_get(parameters, "(&s@a{sv})", &appUri, &properties);

		std::string appId = appIdFromUri(appUri);
		if (appId.empty())
		{
			g_variant_unref(properties);
			return;
		}

		Entry& entry = impl->entries[EntryKey(senderName, appId)];
		entry.updateSerial = ++impl->updateSerial;
		g_variant_lookup(properties, "count", "x", &entry.count);
		gboolean countVisible;
		if (g_variant_lookup(properties, "count-visible", "b", &countVisible))
			entry.countVisible = countVisible;
		g_variant_unref(properties);

		if (!Settings::disableLauncherCounts)
			impl->refreshGroups();
		g_debug("Launcher count update for '%s': count=%" G_GINT64_FORMAT ", visible=%s",
			appId.c_str(), entry.count, entry.countVisible ? "true" : "false");
	}

	static void onNameOwnerChanged(GDBusConnection*, const gchar*, const gchar*,
		const gchar*, const gchar*, GVariant* parameters, gpointer userData)
	{
		Impl* impl = static_cast<Impl*>(userData);
		const gchar* name;
		const gchar* previousOwner;
		const gchar* newOwner;
		g_variant_get(parameters, "(&s&s&s)", &name, &previousOwner, &newOwner);
		(void)previousOwner;

		if (newOwner[0] != '\0')
			return;

		bool removed = false;
		for (auto entry = impl->entries.begin(); entry != impl->entries.end();)
		{
			if (entry->first.first == name)
			{
				entry = impl->entries.erase(entry);
				removed = true;
			}
			else
				++entry;
		}

		if (removed && !Settings::disableLauncherCounts)
			impl->refreshGroups();
	}

	void init()
	{
		GError* error = nullptr;
		connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
		if (connection == nullptr)
		{
			g_warning("Unable to listen for Unity launcher counts: %s",
				error == nullptr ? "unknown error" : error->message);
			g_clear_error(&error);
			return;
		}

		launcherSignalId = g_dbus_connection_signal_subscribe(connection,
			nullptr, "com.canonical.Unity.LauncherEntry", "Update", nullptr, nullptr,
			G_DBUS_SIGNAL_FLAGS_NONE, onLauncherUpdate, this, nullptr);
		nameOwnerChangedSignalId = g_dbus_connection_signal_subscribe(connection,
			"org.freedesktop.DBus", "org.freedesktop.DBus", "NameOwnerChanged",
			"/org/freedesktop/DBus", nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
			onNameOwnerChanged, this, nullptr);

		unityNameOwnerId = g_bus_own_name(G_BUS_TYPE_SESSION, "com.canonical.Unity",
			G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT, nullptr, nullptr, nullptr, nullptr, nullptr);
	}

	void finalize()
	{
		if (connection != nullptr)
		{
			if (launcherSignalId != 0)
				g_dbus_connection_signal_unsubscribe(connection, launcherSignalId);
			if (nameOwnerChangedSignalId != 0)
				g_dbus_connection_signal_unsubscribe(connection, nameOwnerChangedSignalId);
		}

		if (unityNameOwnerId != 0)
			g_bus_unown_name(unityNameOwnerId);

		g_clear_object(&connection);
		launcherSignalId = 0;
		nameOwnerChangedSignalId = 0;
		unityNameOwnerId = 0;
		updateSerial = 0;
		entries.clear();
	}
};

LauncherEntry::Impl LauncherEntry::mImpl;

void LauncherEntry::init() { mImpl.init(); }

void LauncherEntry::finalize() { mImpl.finalize(); }

void LauncherEntry::refreshGroups() { mImpl.refreshGroups(); }

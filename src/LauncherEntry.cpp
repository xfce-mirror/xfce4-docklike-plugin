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

std::string LauncherEntry::appIdFromUri(const gchar* appUri)
{
	const gchar prefix[] = "application://";
	if (appUri == nullptr || !g_str_has_prefix(appUri, prefix))
		return "";

	gchar* unescaped = g_uri_unescape_string(appUri + sizeof(prefix) - 1, nullptr);
	if (xfce_str_is_empty(unescaped))
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

void LauncherEntry::applyEntryToGroup(Group* group)
{
	if (mSettings->disableLauncherCounts)
	{
		group->setLauncherCount(0, false);
		return;
	}

	const Entry* selected = nullptr;
	std::string groupId = group->mAppInfo == nullptr
		? ""
		: Help::String::toLowercase(group->mAppInfo->mId);

	for (const auto& item : mEntries)
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

void LauncherEntry::refreshGroups()
{
	mDock->mGroups.forEach([this](std::pair<std::shared_ptr<AppInfo>, std::shared_ptr<Group>> group) {
		applyEntryToGroup(group.second.get());
	});
}

void LauncherEntry::onLauncherUpdate(GDBusConnection*, const gchar* senderName, const gchar*, const gchar*, const gchar*, GVariant* parameters, gpointer userData)
{
	LauncherEntry* me = static_cast<LauncherEntry*>(userData);
	if (senderName == nullptr || parameters == nullptr || !g_variant_is_of_type(parameters, G_VARIANT_TYPE("(sa{sv})")))
		return;

	const gchar* appUri;
	GVariant* properties;
	g_variant_get(parameters, "(&s@a{sv})", &appUri, &properties);

	std::string appId = me->appIdFromUri(appUri);
	if (appId.empty())
	{
		g_variant_unref(properties);
		return;
	}

	Entry& entry = me->mEntries[EntryKey(senderName, appId)];
	entry.updateSerial = ++me->mUpdateSerial;
	g_variant_lookup(properties, "count", "x", &entry.count);
	gboolean countVisible;
	if (g_variant_lookup(properties, "count-visible", "b", &countVisible))
		entry.countVisible = countVisible;
	g_variant_unref(properties);

	if (!me->mSettings->disableLauncherCounts)
		me->refreshGroups();
	g_debug("Launcher count update for '%s': count=%" G_GINT64_FORMAT ", visible=%s",
		appId.c_str(), entry.count, entry.countVisible ? "true" : "false");
}

void LauncherEntry::onNameOwnerChanged(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant* parameters, gpointer userData)
{
	LauncherEntry* me = static_cast<LauncherEntry*>(userData);
	const gchar* name;
	const gchar* previousOwner;
	const gchar* newOwner;
	g_variant_get(parameters, "(&s&s&s)", &name, &previousOwner, &newOwner);

	if (!xfce_str_is_empty(newOwner))
		return;

	bool removed = false;
	for (auto entry = me->mEntries.begin(); entry != me->mEntries.end();)
	{
		if (entry->first.first == name)
		{
			entry = me->mEntries.erase(entry);
			removed = true;
		}
		else
			++entry;
	}

	if (removed && !me->mSettings->disableLauncherCounts)
		me->refreshGroups();
}

LauncherEntry::LauncherEntry(Settings* settings, Dock* dock) : mSettings(settings), mDock(dock)
{
	GError* error = nullptr;
	mConnection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
	if (mConnection == nullptr)
	{
		g_warning("Unable to listen for Unity launcher counts: %s", error->message);
		g_clear_error(&error);
		return;
	}

	mLauncherSignalId = g_dbus_connection_signal_subscribe(mConnection,
		nullptr, "com.canonical.Unity.LauncherEntry", "Update", nullptr, nullptr,
		G_DBUS_SIGNAL_FLAGS_NONE, onLauncherUpdate, this, nullptr);
	mNameOwnerChangedSignalId = g_dbus_connection_signal_subscribe(mConnection,
		"org.freedesktop.DBus", "org.freedesktop.DBus", "NameOwnerChanged",
		"/org/freedesktop/DBus", nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
		onNameOwnerChanged, this, nullptr);

	mUnityNameOwnerId = g_bus_own_name(G_BUS_TYPE_SESSION, "com.canonical.Unity",
		G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT, nullptr, nullptr, nullptr, nullptr, nullptr);
}

LauncherEntry::~LauncherEntry()
{
	if (mConnection != nullptr)
	{
		if (mLauncherSignalId != 0)
			g_dbus_connection_signal_unsubscribe(mConnection, mLauncherSignalId);
		if (mNameOwnerChangedSignalId != 0)
			g_dbus_connection_signal_unsubscribe(mConnection, mNameOwnerChangedSignalId);
		g_object_unref(mConnection);
	}

	if (mUnityNameOwnerId != 0)
		g_bus_unown_name(mUnityNameOwnerId);
}

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

#include "Hotkeys.hpp"
#include "Settings.hpp"

#include <xfconf/xfconf.h>

namespace Hotkeys
{
	bool mKeyAloneGrabbed = false;
	int mGrabbedKeys = 0;
	bool mAddShortcutUIAvailable = false;

	XfconfChannel* mChannel = nullptr;
	auto mStrEqual = [](gpointer key, gpointer _value, gpointer data) {
		GValue* value = (GValue*)_value;
		if (G_VALUE_HOLDS_STRING(value))
			return (gboolean)g_str_equal(g_value_get_string(value), (gchar*)data);
		return (gboolean) false;
	};

	// =========================================================================

	static void addKeyComboShortcuts()
	{
		GHashTable* commands = xfconf_channel_get_properties(mChannel, "/commands/custom");
		mGrabbedKeys = mNbHotkeys;
		for (gint n = 0; n < mNbHotkeys; n++)
		{
			gchar* command = g_strdup_printf(
				"xfce4-panel --plugin-event=docklike-%d:activate-group:int:%d",
				xfce_panel_plugin_get_unique_id(Plugin::mXfPlugin),
				n);
			if (g_hash_table_find(commands, mStrEqual, command) == nullptr)
			{
				GdkKeymap* keymap = gdk_keymap_get_for_display(gdk_display_get_default());
				GdkKeymapKey* keys;
				gint nKeys = 0;
				gchar* shortcut = nullptr;

				gdk_keymap_get_entries_for_keyval(keymap, GDK_KEY_1 + n, &keys, &nKeys);
				if (nKeys > 0)
				{
					guint* keyvals = nullptr;
					guint keycode = keys[0].keycode;
					g_free(keys);
					keys = nullptr;
					gdk_keymap_get_entries_for_keycode(keymap, keycode, &keys, &keyvals, &nKeys);
					gint m = 0;
					for (m = 0; m < nKeys; m++)
					{
						if (keys[m].group == 0 && keys[m].level == 0)
						{
							shortcut = gtk_accelerator_name(keyvals[m], GDK_SUPER_MASK);
							break;
						}
					}
					if (m == nKeys)
					{
						g_debug("Failed to map keycode %d to keyval", keycode);
					}
					g_free(keys);
					g_free(keyvals);
				}
				else
				{
					g_debug("Failed to map keyval %d to keycode", GDK_KEY_1 + n);
				}

				if (shortcut == nullptr)
				{
					g_debug("Falling back to direct mapping of GDK_KEY_%d", n + 1);
					shortcut = gtk_accelerator_name(GDK_KEY_1 + n, GDK_SUPER_MASK);
				}

				std::string property = "/commands/custom/";
				property += shortcut;
				g_free(shortcut);
				if (xfconf_channel_has_property(mChannel, property.c_str()))
				{
					mGrabbedKeys = n;
					break;
				}
				else
				{
					xfconf_channel_set_string(mChannel, property.c_str(), command);
				}
			}
			g_free(command);
		}
		g_hash_table_destroy(commands);
	}

	static void addKeyAloneShortcut()
	{
		GHashTable* commands = xfconf_channel_get_properties(mChannel, "/commands/custom");
		gchar* command = g_strdup_printf(
			"xfce4-panel --plugin-event=docklike-%d:switch-to-last-window",
			xfce_panel_plugin_get_unique_id(Plugin::mXfPlugin));
		mKeyAloneGrabbed = true;
		if (g_hash_table_find(commands, mStrEqual, command) == nullptr)
		{
			gchar* shortcut = gtk_accelerator_name(GDK_KEY_Super_L, (GdkModifierType)0);
			gchar* property = g_strdup_printf("/commands/custom/%s", shortcut);
			if (xfconf_channel_has_property(mChannel, property))
				mKeyAloneGrabbed = false;
			else
				xfconf_channel_set_string(mChannel, property, command);
			g_free(property);
			g_free(shortcut);
		}
		g_free(command);
		g_hash_table_destroy(commands);
	}

	// =========================================================================

	void init()
	{
		GError* error = nullptr;
		if (xfconf_init(&error))
		{
			mChannel = xfconf_channel_get("xfce4-keyboard-shortcuts");
		}
		else
		{
			g_critical("Failed to initialize Xfconf: %s", error->message);
			g_clear_error(&error);
		}

		gchar* path = g_find_program_in_path("xfce4-keyboard-settings");
		if (path != nullptr)
		{
			// --shortcuts option is only available since xfce4-settings 4.21.2
			gchar* output = nullptr;
			const gchar* command = "xfce4-keyboard-settings --help";
			if (g_spawn_command_line_sync(command, &output, nullptr, nullptr, &error))
			{
				if (output != nullptr && g_strstr_len(output, -1, "--shortcuts") != nullptr)
					mAddShortcutUIAvailable = true;
				g_free(output);
			}
			else
			{
				g_warning("Failed to launch %s: %s", command, error->message);
				g_clear_error(&error);
			}
			g_free(path);
		}
	}

	void finalize()
	{
		if (mChannel != nullptr)
			xfconf_shutdown();
	}

	void updateSettings()
	{
		if (mChannel == nullptr)
			return;

		if (Settings::keyAloneActive)
			addKeyAloneShortcut();
		if (Settings::keyComboActive)
			addKeyComboShortcuts();
	}

	void resetShortcuts()
	{
		if (mChannel == nullptr)
			return;

		GHashTable* commands = xfconf_channel_get_properties(mChannel, "/commands/custom");
		gchar* prefix = g_strdup_printf(
			"xfce4-panel --plugin-event=docklike-%d:",
			xfce_panel_plugin_get_unique_id(Plugin::mXfPlugin));
		const gchar* property;
		GValue* command;
		GHashTableIter iter;
		g_hash_table_iter_init(&iter, commands);
		while (g_hash_table_iter_next(&iter, (gpointer*)&property, (gpointer*)&command))
		{
			if (G_VALUE_HOLDS_STRING(command) && g_str_has_prefix(g_value_get_string(command), prefix))
				xfconf_channel_reset_property(mChannel, property, true);
		}
		g_free(prefix);
		g_hash_table_destroy(commands);
	}
} // namespace Hotkeys

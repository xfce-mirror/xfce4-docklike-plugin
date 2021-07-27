/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#ifndef SETTINGS_DIALOG_HPP
#define SETTINGS_DIALOG_HPP

#include <gtk/gtk.h>

#include <string>

#include "Helpers.hpp"
#include "Plugin.hpp"
#include "Settings.hpp"

namespace SettingsDialog
{
	void popup();
	void updateKeyComboActiveWarning(GtkWidget* widget);
} // namespace SettingsDialog

#endif // SETTINGS_DIALOG_HPP

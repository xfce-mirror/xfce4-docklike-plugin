/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include <libxfce4panel/libxfce4panel.h>

extern void construct(XfcePanelPlugin* xfPlugin);

XFCE_PANEL_PLUGIN_REGISTER(construct);

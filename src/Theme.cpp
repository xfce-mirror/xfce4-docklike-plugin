/*
 * Docklike Taskbar - A modern, minimalist taskbar for XFCE
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Theme.hpp"

void Theme::init()
{
	load();

	g_signal_connect(G_OBJECT(gtk_widget_get_style_context(Dock::mBox)), "changed",
		G_CALLBACK(+[](GtkStyleContext* stylecontext) { load(); }), NULL);
}

void Theme::load()
{
	GtkCssProvider* css_provider = gtk_css_provider_new();
	std::string css = get_theme_colors();
	const gchar* filename;

	if (g_environ_getenv(g_get_environ(), "XDG_CONFIG_HOME") != NULL)
		filename = g_build_filename(g_environ_getenv(g_get_environ(), "XDG_CONFIG_HOME"),
			"xfce4-docklike-plugin/gtk.css", NULL);
	else
		filename = g_build_filename(g_environ_getenv(g_get_environ(), "HOME"),
			".config/xfce4-docklike-plugin/gtk.css", NULL);

	if (filename != NULL)
	{
		if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
		{
			FILE* f = fopen(filename, "r");

			if (f != NULL)
			{
				int read_char;
				while ((read_char = getc(f)) != EOF)
					css += read_char;
				fclose(f);
			}
		}
	}
	else // Defaults from https://github.com/nsz32/docklike-plugin/blob/master/src/Theme.cpp
		css += ".drop_target { box-shadow: inset 4px 0px 0px 0px darkviolet; }\n.menu { margin: 0; padding: 0; border: 0; background-color: @menu_bgcolor; }\n.hover_menu_item { background-color: alpha(@menu_item_color_hover, 0.2); }\n.active_group { background-color: alpha(@menu_item_bgcolor_hover, 0.35); }\n.hover_group { background-color: alpha(@menu_item_bgcolor_hover, 0.2); }\n";

	if (gtk_css_provider_load_from_data(css_provider, css.c_str(), -1, NULL))
		gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
			GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

std::string Theme::get_theme_colors()
{
	GtkWidget* menu = gtk_menu_new();
	GtkStyleContext* sc = gtk_widget_get_style_context(menu);

	GValue gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "background-color", GTK_STATE_FLAG_NORMAL, &gv);
	std::string menuBg = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "color", GTK_STATE_FLAG_NORMAL, &gv);
	std::string itemLabel = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "color", GTK_STATE_FLAG_PRELIGHT, &gv);
	std::string itemLabelHover = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "background-color", GTK_STATE_FLAG_PRELIGHT, &gv);
	std::string itemBgHover = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));

	gtk_widget_destroy(menu);

	return "@define-color menu_bgcolor " + menuBg + ";\n@define-color menu_item_color " + itemLabel + ";\n@define-color menu_item_color_hover " + itemLabelHover + ";\n@define-color menu_item_bgcolor_hover " + itemBgHover + ";\n";
}

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
	std::string css = get_theme_colors();
	const gchar* filename;

	// Prefer to load user CSS
	if (g_environ_getenv(g_get_environ(), "XDG_CONFIG_HOME") != NULL)
		filename = g_build_filename(g_environ_getenv(g_get_environ(), "XDG_CONFIG_HOME"),
			"xfce4-docklike-plugin/gtk.css", NULL);
	else
		filename = g_build_filename(g_environ_getenv(g_get_environ(), "HOME"),
			".config/xfce4-docklike-plugin/gtk.css", NULL);

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
	else // Create default CSS
	{
		switch (Settings::indicatorOrientation)
		{
		case 0: // Top
			css += "#active_group { box-shadow: inset 0px -4px 0px 0px @indicator_color; }\n"
				   ".open_group { box-shadow: inset 0px -4px 0px 0px @inactive_color; }\n";
			break;
		case 1: // Left
			css += "#active_group { box-shadow: inset -4px 0px 0px 0px @indicator_color; }\n"
				   ".open_group { box-shadow: inset -4px 0px 0px 0px @inactive_color; }\n";
			break;
		case 2: // Bottom
			css += "#active_group { box-shadow: inset 0px 4px 0px 0px @indicator_color; }\n"
				   ".open_group { box-shadow: inset 0px 4px 0px 0px @inactive_color; }\n";
			break;
		case 3: // Right
			css += "#active_group { box-shadow: inset 4px 0px 0px 0px @indicator_color; }\n"
				   ".open_group { box-shadow: inset 4px 0px 0px 0px @inactive_color; }\n";
			break;
		}

		css += "#drop_target { background-color: alpha(@menu_item_color_hover, 0.9); }\n"
			   "#active_group { background-color: alpha(@menu_item_bgcolor_hover, 0.35); }\n"
			   "#hover_menu_item { background-color: alpha(@menu_item_color_hover, 0.2); }\n"
			   "#hover_group { background-color: alpha(@menu_item_bgcolor_hover, 0.2); }\n"
			   ".open_group { background-color: alpha(@menu_item_bgcolor_hover, 0.2); }\n"
			   ".menu { margin: 0; padding: 0; border: 0; background-color: @menu_bgcolor; }\n";
	}

	GtkCssProvider* css_provider = gtk_css_provider_new();
	if (gtk_css_provider_load_from_data(css_provider, css.c_str(), -1, NULL))
		gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
			GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	css.clear();
}

std::string Theme::get_theme_colors()
{
	GtkWidget* menu = gtk_menu_new();
	GtkStyleContext* sc = gtk_widget_get_style_context(menu);
	std::string css = "@define-color menu_bgcolor ";

	GValue gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "background-color", GTK_STATE_FLAG_NORMAL, &gv);
	css += gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "color", GTK_STATE_FLAG_NORMAL, &gv);
	css += ";\n@define-color menu_item_color ";
	css += gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "color", GTK_STATE_FLAG_PRELIGHT, &gv);
	css += ";\n@define-color menu_item_color_hover ";
	css += gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "background-color", GTK_STATE_FLAG_PRELIGHT, &gv);
	css += ";\n@define-color menu_item_bgcolor_hover ";
	css += gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));

	gtk_widget_destroy(menu);

	css += ";\n@define-color indicator_color ";
	css += gdk_rgba_to_string(Settings::indicatorColor);
	css += ";\n@define-color inactive_color ";
	css += gdk_rgba_to_string(Settings::inactiveColor);
	css += ";\n";

	return css;
}

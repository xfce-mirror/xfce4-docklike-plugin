/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Theme.hpp"

void Theme::init()
{
	g_signal_connect(G_OBJECT(gtk_widget_get_style_context(Dock::mBox)), "changed",
		G_CALLBACK(+[](GtkStyleContext* stylecontext)
				   { load(); }),
		NULL);
}

void Theme::load()
{
	GtkCssProvider* css_provider = gtk_css_provider_new();
	std::string css = get_theme_colors();
	gchar* filename = xfce_resource_lookup(XFCE_RESOURCE_CONFIG, "xfce4-docklike-plugin/gtk.css");

	if (filename != NULL && g_file_test(filename, G_FILE_TEST_IS_REGULAR))
	{
		FILE* f = fopen(filename, "r");

		if (f != NULL)
		{
			int read_char;
			while ((read_char = getc(f)) != EOF)
				css += read_char;
			fclose(f);
		}
		else // Empty file
			css += DEFAULT_THEME;
	}
	else // No file
		css += DEFAULT_THEME;

	if (gtk_css_provider_load_from_data(css_provider, css.c_str(), -1, NULL))
		gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
			GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_free(filename);
	g_object_unref(css_provider);
}

std::string Theme::get_theme_colors()
{
	GtkWidget* menu = gtk_menu_new();
	GtkStyleContext* sc = gtk_widget_get_style_context(menu);
	gchar* str;

	GValue gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "background-color", GTK_STATE_FLAG_NORMAL, &gv);
	str = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));
	std::string menuBg = str;
	g_free(str);
	g_value_unset(&gv);

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "color", GTK_STATE_FLAG_NORMAL, &gv);
	str = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));
	std::string itemLabel = str;
	g_free(str);
	g_value_unset(&gv);

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "color", GTK_STATE_FLAG_PRELIGHT, &gv);
	str = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));
	std::string itemLabelHover = str;
	g_free(str);
	g_value_unset(&gv);

	gv = G_VALUE_INIT;
	gtk_style_context_get_property(sc, "background-color", GTK_STATE_FLAG_PRELIGHT, &gv);
	str = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));
	std::string itemBgHover = str;
	g_free(str);
	g_value_unset(&gv);

	str = gdk_rgba_to_string(Settings::indicatorColor.get().get());
	std::string indicatorColor = str;
	g_free(str);
	str = gdk_rgba_to_string(Settings::inactiveColor.get().get());
	std::string inactiveColor = str;
	g_free(str);

	if (Settings::indicatorColorFromTheme)
	{
		gv = G_VALUE_INIT;
		gtk_style_context_get_property(sc, "color", GTK_STATE_FLAG_NORMAL, &gv);
		str = gdk_rgba_to_string((GdkRGBA*)g_value_get_boxed(&gv));
		indicatorColor = str;
		inactiveColor = str;
		g_free(str);
		g_value_unset(&gv);
	}

	gtk_widget_destroy(menu);

	std::string css = "@define-color menu_bgcolor " + menuBg + ";\n";
	css += "@define-color menu_item_color " + itemLabel + ";\n";
	css += "@define-color menu_item_color_hover " + itemLabelHover + ";\n";
	css += "@define-color menu_item_bgcolor_hover " + itemBgHover + ";\n";
	css += "@define-color active_indicator_color " + indicatorColor + ";\n";
	css += "@define-color inactive_indicator_color " + inactiveColor + ";\n";
	return css;
}

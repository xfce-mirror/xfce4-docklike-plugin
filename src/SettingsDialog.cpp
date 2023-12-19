/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "SettingsDialog.hpp"
#include "Plugin.hpp"

namespace SettingsDialog
{
	void popup()
	{
		/* Hook to make sure GtkBuilder knows this is an XfceTitledDialog object */
		if (xfce_titled_dialog_get_type() == 0)
			return;

		GtkBuilder* builder = gtk_builder_new_from_resource("/_dialogs.xml");
		GtkWidget* dialog = (GtkWidget*)gtk_builder_get_object(builder, "dialog");
		gtk_window_set_role(GTK_WINDOW(dialog), "xfce4-panel");
		gtk_widget_show(dialog);
		xfce_panel_plugin_block_menu(Plugin::mXfPlugin);

		g_signal_connect(
			gtk_builder_get_object(builder, "b_close"), "clicked",
			G_CALLBACK(+[](GtkButton* button, GtkWidget* dialogWindow) {
				gtk_widget_hide(dialogWindow);
				gtk_dialog_response(GTK_DIALOG(dialogWindow), 0);
			}),
			dialog);

		g_signal_connect(
			gtk_builder_get_object(builder, "b_help"), "clicked",
			G_CALLBACK(+[](GtkButton* button, GtkWindow* dialogWindow) {
				gtk_show_uri_on_window(dialogWindow, HELP_WEBSITE, GDK_CURRENT_TIME, nullptr);
			}),
			dialog);

		g_signal_connect(dialog, "close",
			G_CALLBACK(+[](GtkDialog* _dialog, GtkBuilder* _builder) {
				xfce_panel_plugin_unblock_menu(Plugin::mXfPlugin);
				g_object_unref(_builder);
			}),
			builder);

		g_signal_connect(dialog, "response",
			G_CALLBACK(+[](GtkDialog* _dialog, gint response, GtkBuilder* _builder) {
				xfce_panel_plugin_unblock_menu(Plugin::mXfPlugin);
				g_object_unref(_builder);
			}),
			builder);

		// =====================================================================

		GObject* noListForSingleWindow = gtk_builder_get_object(builder, "c_noListForSingleWindow");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(noListForSingleWindow), Settings::noWindowsListIfSingle);
		g_signal_connect(noListForSingleWindow, "toggled",
			G_CALLBACK(+[](GtkToggleButton* noWindowsListIfSingle) {
				Settings::noWindowsListIfSingle.set(gtk_toggle_button_get_active(noWindowsListIfSingle));
			}),
			nullptr);

		GObject* onlyDisplayVisible = gtk_builder_get_object(builder, "c_onlyDisplayVisible");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onlyDisplayVisible), Settings::onlyDisplayVisible);
		g_signal_connect(onlyDisplayVisible, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _onlyDisplayVisible) {
				Settings::onlyDisplayVisible.set(gtk_toggle_button_get_active(_onlyDisplayVisible));
				Wnck::setVisibleGroups();
			}),
			nullptr);

		GObject* onlyDisplayScreen = gtk_builder_get_object(builder, "c_onlyDisplayScreen");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onlyDisplayScreen), Settings::onlyDisplayScreen);
		g_signal_connect(onlyDisplayScreen, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _onlyDisplayScreen) {
				Settings::onlyDisplayScreen.set(gtk_toggle_button_get_active(_onlyDisplayScreen));
				Wnck::setVisibleGroups();
			}),
			nullptr);

		GObject* showPreviews = gtk_builder_get_object(builder, "c_showPreviews");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showPreviews), Settings::showPreviews);
		g_signal_connect(showPreviews, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _showPreviews) {
				Settings::showPreviews.set(gtk_toggle_button_get_active(_showPreviews));
			}),
			nullptr);
		
		GObject* showWindowCount = gtk_builder_get_object(builder, "c_showWindowCount");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showWindowCount), Settings::showWindowCount);
		g_signal_connect(showWindowCount, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _showWindowCount) {
				Settings::showWindowCount.set(gtk_toggle_button_get_active(_showWindowCount));
			}),
			nullptr);

		GObject* middleButtonBehavior = gtk_builder_get_object(builder, "co_middleButtonBehavior");
		gtk_combo_box_set_active(GTK_COMBO_BOX(middleButtonBehavior), Settings::middleButtonBehavior);
		g_signal_connect(middleButtonBehavior, "changed",
			G_CALLBACK(+[](GtkComboBox* _middleButtonBehavior, GtkWidget* g) {
				Settings::middleButtonBehavior.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_middleButtonBehavior)));
			}),
			dialog);

		// =====================================================================

		GObject* indicatorOrientation = gtk_builder_get_object(builder, "co_indicatorOrientation");
		gtk_combo_box_set_active(GTK_COMBO_BOX(indicatorOrientation), Settings::indicatorOrientation);
		g_signal_connect(indicatorOrientation, "changed",
			G_CALLBACK(+[](GtkComboBox* _indicatorOrientation, GtkWidget* g) {
				Settings::indicatorOrientation.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_indicatorOrientation)));
			}),
			dialog);

		GObject* indicatorStyle = gtk_builder_get_object(builder, "co_indicatorStyle");
		gtk_combo_box_set_active(GTK_COMBO_BOX(indicatorStyle), Settings::indicatorStyle);
		g_signal_connect(indicatorStyle, "changed",
			G_CALLBACK(+[](GtkComboBox* _indicatorStyle, GtkWidget* g) {
				Settings::indicatorStyle.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_indicatorStyle)));
			}),
			dialog);

		GObject* inactiveIndicatorStyle = gtk_builder_get_object(builder, "co_inactiveIndicatorStyle");
		gtk_combo_box_set_active(GTK_COMBO_BOX(inactiveIndicatorStyle), Settings::inactiveIndicatorStyle);
		g_signal_connect(inactiveIndicatorStyle, "changed",
			G_CALLBACK(+[](GtkComboBox* _inactiveIndicatorStyle, GtkWidget* g) {
				Settings::inactiveIndicatorStyle.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_inactiveIndicatorStyle)));
			}),
			dialog);

		GObject* customIndicatorColors = gtk_builder_get_object(builder, "g_customIndicatorColors");
		gtk_widget_set_sensitive(GTK_WIDGET(customIndicatorColors), !Settings::indicatorColorFromTheme);

		GObject* indicatorColor = gtk_builder_get_object(builder, "cp_indicatorColor");
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(indicatorColor), Settings::indicatorColor.get().get());
		g_signal_connect(indicatorColor, "color-set",
			G_CALLBACK(+[](GtkColorButton* _indicatorColor, GtkWidget* g) {
				std::shared_ptr<GdkRGBA> color(g_new(GdkRGBA, 1), g_free);
				gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(_indicatorColor), color.get());
				Settings::indicatorColor.set(color);
			}),
			dialog);

		GObject* inactiveColor = gtk_builder_get_object(builder, "cp_inactiveColor");
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(inactiveColor), Settings::inactiveColor.get().get());
		g_signal_connect(inactiveColor, "color-set",
			G_CALLBACK(+[](GtkColorButton* _inactiveColor, GtkWidget* g) {
				std::shared_ptr<GdkRGBA> color(g_new(GdkRGBA, 1), g_free);
				gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(_inactiveColor), color.get());
				Settings::inactiveColor.set(color);
			}),
			dialog);

		GObject* indicatorColorFromTheme = gtk_builder_get_object(builder, "c_indicatorColorFromTheme");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(indicatorColorFromTheme), Settings::indicatorColorFromTheme);
		g_signal_connect(indicatorColorFromTheme, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _indicatorColorFromTheme, GtkWidget* _customIndicatorColors) {
				Settings::indicatorColorFromTheme.set(gtk_toggle_button_get_active(_indicatorColorFromTheme));
				gtk_widget_set_sensitive(GTK_WIDGET(_customIndicatorColors), !Settings::indicatorColorFromTheme);
			}),
			customIndicatorColors);

		// =====================================================================

		GObject* iconSize = gtk_builder_get_object(builder, "e_iconSize");
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(iconSize))), std::to_string(Settings::iconSize).c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(iconSize), Settings::forceIconSize);
		g_signal_connect(iconSize, "changed",
			G_CALLBACK(+[](GtkComboBox* _iconSize) {
				GtkEntry* entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(_iconSize)));
				std::string svalue = Help::String::numericOnly(gtk_entry_get_text(entry));
				int value = std::stoi("0" + svalue);
				Settings::iconSize.set(value);
				gtk_entry_set_text(entry, svalue.c_str());
				if (value < Settings::minIconSize || value > Settings::maxIconSize)
					gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(entry)), GTK_STYLE_CLASS_ERROR);
				else
					gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(entry)), GTK_STYLE_CLASS_ERROR);
			}),
			nullptr);

		GObject* forceIconSize = gtk_builder_get_object(builder, "c_forceIconSize");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(forceIconSize), Settings::forceIconSize);
		g_signal_connect(forceIconSize, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _forceIconSize, GtkWidget* _iconSize) {
				Settings::forceIconSize.set(gtk_toggle_button_get_active(_forceIconSize));
				gtk_widget_set_sensitive(GTK_WIDGET(_iconSize), Settings::forceIconSize);
			}),
			iconSize);

		// =====================================================================

		GObject* keyComboActiveWarning = gtk_builder_get_object(builder, "c_keyComboActiveWarning");
		GObject* keyComboActive = gtk_builder_get_object(builder, "c_keyComboActive");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyComboActive), Settings::keyComboActive);
		g_signal_connect(keyComboActive, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _keyComboActive, GtkWidget* tooltip) {
				Settings::keyComboActive.set(gtk_toggle_button_get_active(_keyComboActive));
				updateKeyComboActiveWarning(tooltip);
			}),
			keyComboActiveWarning);

		GObject* keyAloneActive = gtk_builder_get_object(builder, "c_keyAloneActive");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyAloneActive), Settings::keyAloneActive);
		g_signal_connect(keyAloneActive, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _keyAloneActive) {
				Settings::keyAloneActive.set(gtk_toggle_button_get_active(_keyAloneActive));
			}),
			nullptr);

		if (!Hotkeys::mXIExtAvailable)
		{
			gtk_widget_set_sensitive(GTK_WIDGET(keyAloneActive), false);
			gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "c_keyAloneActiveWarning")));
		}

		updateKeyComboActiveWarning(GTK_WIDGET(keyComboActiveWarning));
	}

	void updateKeyComboActiveWarning(GtkWidget* widget)
	{
		if (!Settings::keyComboActive || Hotkeys::mGrabbedKeys == Hotkeys::NbHotkeys)
			gtk_widget_hide(widget);
		else
		{
			std::string tooltip = "";
			gchar* markup;

			if (Hotkeys::mGrabbedKeys > 0)
			{
				markup = g_strdup_printf(_("<b>Only the first %u hotkeys(s) are enabled.</b>\n"), Hotkeys::mGrabbedKeys);
				tooltip += markup;
				g_free(markup);
			}

			markup = g_strdup_printf(_("The &lt;SUPER&gt;+%u combination seems already in use by another process.\nCheck your Xfce settings."), Hotkeys::mGrabbedKeys + 1);
			tooltip += markup;
			g_free(markup);

			gtk_widget_set_tooltip_markup(widget, tooltip.c_str());
			gtk_image_set_from_icon_name(GTK_IMAGE(widget), (Hotkeys::mGrabbedKeys == 0) ? "dialog-error" : "dialog-warning", GTK_ICON_SIZE_SMALL_TOOLBAR);
			gtk_widget_show(widget);
		}
	}
} // namespace SettingsDialog

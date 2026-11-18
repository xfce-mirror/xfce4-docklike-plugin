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

#include "SettingsDialog.hpp"

#include <libxfce4ui/libxfce4ui.h>

#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#endif

namespace SettingsDialog
{
	Plugin* mPlugin = nullptr;
	GtkWidget* mSettingsDialog = nullptr;

	static void updateKeyComboActiveWarning(GtkWidget* widget)
	{
		if (!mPlugin->mSettings->keyComboActive || mPlugin->mHotkeys->mGrabbedKeys == mPlugin->mHotkeys->mNbHotkeys)
			gtk_widget_hide(widget);
		else
		{
			std::string tooltip = "";
			gchar* markup;

			if (mPlugin->mHotkeys->mGrabbedKeys > 1)
			{
				markup = g_strdup_printf(
					_("<b>Only the first %u hotkeys are enabled.</b>\n"),
					mPlugin->mHotkeys->mGrabbedKeys);
				tooltip += markup;
				g_free(markup);
			}
			else if (mPlugin->mHotkeys->mGrabbedKeys == 1)
			{
				tooltip += _("<b>Only the first hotkey is enabled.</b>\n");
			}
			else
			{
				tooltip += _("<b>No hotkeys enabled.</b>\n");
			}

			markup = g_strdup_printf(
				_("The &lt;SUPER&gt;+%u combination seems already mapped to another command.\nCheck your Xfce keyboard settings."),
				mPlugin->mHotkeys->mGrabbedKeys + 1);
			tooltip += markup;
			g_free(markup);

			gtk_widget_set_tooltip_markup(widget, tooltip.c_str());
			gtk_widget_show(widget);
		}
	}

	static void updateKeyAloneActiveWarning(GtkWidget* widget)
	{
		gtk_widget_set_visible(widget, mPlugin->mSettings->keyAloneActive && !mPlugin->mHotkeys->mKeyAloneGrabbed);
	}

	void popup(Plugin* plugin)
	{
		if (mSettingsDialog != nullptr)
		{
			gtk_window_present(GTK_WINDOW(mSettingsDialog));
			return;
		}

		/* Hook to make sure GtkBuilder knows this is an XfceTitledDialog object */
		if (xfce_titled_dialog_get_type() == 0)
			return;

		mPlugin = plugin;
		GtkBuilder* builder = gtk_builder_new_from_resource("/_dialogs.ui");
		GtkWidget* dialog = (GtkWidget*)gtk_builder_get_object(builder, "dialog");
		mSettingsDialog = dialog;
		g_object_add_weak_pointer(G_OBJECT(dialog), (gpointer*)&mSettingsDialog);
		gtk_window_set_role(GTK_WINDOW(dialog), "xfce4-panel");
		gtk_widget_show(dialog);

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
				gtk_show_uri_on_window(dialogWindow, PACKAGE_URL, GDK_CURRENT_TIME, nullptr);
			}),
			dialog);

		g_signal_connect(dialog, "destroy",
			G_CALLBACK(+[](GtkDialog* _dialog, GtkBuilder* _builder) {
				g_object_unref(_builder);
			}),
			builder);

		g_signal_connect(dialog, "response",
			G_CALLBACK(+[](GtkDialog* _dialog, gint response, GtkBuilder* _builder) {
				gtk_widget_destroy(GTK_WIDGET(_dialog));
			}),
			builder);

		// =====================================================================

		GObject* noListForSingleWindow = gtk_builder_get_object(builder, "c_noListForSingleWindow");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(noListForSingleWindow), mPlugin->mSettings->noWindowsListIfSingle);
		g_signal_connect(noListForSingleWindow, "toggled",
			G_CALLBACK(+[](GtkToggleButton* noWindowsListIfSingle) {
				mPlugin->mSettings->noWindowsListIfSingle.set(gtk_toggle_button_get_active(noWindowsListIfSingle));
			}),
			nullptr);

		GObject* onlyDisplayVisible = gtk_builder_get_object(builder, "c_onlyDisplayVisible");
#ifdef ENABLE_X11
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
		{
			// window<->workspace association only works on X11
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onlyDisplayVisible), mPlugin->mSettings->onlyDisplayVisible);
			g_signal_connect(onlyDisplayVisible, "toggled",
				G_CALLBACK(+[](GtkToggleButton* _onlyDisplayVisible) {
					mPlugin->mSettings->onlyDisplayVisible.set(gtk_toggle_button_get_active(_onlyDisplayVisible));
					mPlugin->mXfw->setVisibleGroups();
				}),
				nullptr);
		}
		else
#endif
		{
			gtk_widget_hide(GTK_WIDGET(onlyDisplayVisible));
		}

		GObject* onlyDisplayScreen = gtk_builder_get_object(builder, "c_onlyDisplayScreen");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onlyDisplayScreen), mPlugin->mSettings->onlyDisplayScreen);
		g_signal_connect(onlyDisplayScreen, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _onlyDisplayScreen) {
				mPlugin->mSettings->onlyDisplayScreen.set(gtk_toggle_button_get_active(_onlyDisplayScreen));
				mPlugin->mXfw->setVisibleGroups();
			}),
			nullptr);

		GObject* showPreviews = gtk_builder_get_object(builder, "c_showPreviews");
#ifdef ENABLE_X11
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showPreviews), mPlugin->mSettings->showPreviews);
			g_signal_connect(showPreviews, "toggled",
				G_CALLBACK(+[](GtkToggleButton* _showPreviews) {
					mPlugin->mSettings->showPreviews.set(gtk_toggle_button_get_active(_showPreviews));
				}),
				nullptr);
		}
		else
#endif
		{
			gtk_widget_hide(GTK_WIDGET(showPreviews));
		}

		GObject* showWindowCount = gtk_builder_get_object(builder, "c_showWindowCount");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showWindowCount), mPlugin->mSettings->showWindowCount);
		g_signal_connect(showWindowCount, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _showWindowCount) {
				mPlugin->mSettings->showWindowCount.set(gtk_toggle_button_get_active(_showWindowCount));
			}),
			nullptr);

		GObject* disableLauncherCounts = gtk_builder_get_object(builder, "c_disableLauncherCounts");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disableLauncherCounts), mPlugin->mSettings->disableLauncherCounts);
		g_signal_connect(disableLauncherCounts, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _disableLauncherCounts) {
				mPlugin->mSettings->disableLauncherCounts.set(gtk_toggle_button_get_active(_disableLauncherCounts));
			}),
			nullptr);

		GObject* middleButtonBehavior = gtk_builder_get_object(builder, "co_middleButtonBehavior");
		gtk_combo_box_set_active(GTK_COMBO_BOX(middleButtonBehavior), mPlugin->mSettings->middleButtonBehavior);
		g_signal_connect(middleButtonBehavior, "changed",
			G_CALLBACK(+[](GtkComboBox* _middleButtonBehavior, GtkWidget* g) {
				mPlugin->mSettings->middleButtonBehavior.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_middleButtonBehavior)));
			}),
			dialog);

		// =====================================================================

		GObject* indicatorOrientation = gtk_builder_get_object(builder, "co_indicatorOrientation");
		gtk_combo_box_set_active(GTK_COMBO_BOX(indicatorOrientation), mPlugin->mSettings->indicatorOrientation);
		g_signal_connect(indicatorOrientation, "changed",
			G_CALLBACK(+[](GtkComboBox* _indicatorOrientation, GtkWidget* g) {
				mPlugin->mSettings->indicatorOrientation.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_indicatorOrientation)));
			}),
			dialog);

		GObject* indicatorStyle = gtk_builder_get_object(builder, "co_indicatorStyle");
		gtk_combo_box_set_active(GTK_COMBO_BOX(indicatorStyle), mPlugin->mSettings->indicatorStyle);
		g_signal_connect(indicatorStyle, "changed",
			G_CALLBACK(+[](GtkComboBox* _indicatorStyle, GtkWidget* g) {
				mPlugin->mSettings->indicatorStyle.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_indicatorStyle)));
			}),
			dialog);

		GObject* inactiveIndicatorStyle = gtk_builder_get_object(builder, "co_inactiveIndicatorStyle");
		gtk_combo_box_set_active(GTK_COMBO_BOX(inactiveIndicatorStyle), mPlugin->mSettings->inactiveIndicatorStyle);
		g_signal_connect(inactiveIndicatorStyle, "changed",
			G_CALLBACK(+[](GtkComboBox* _inactiveIndicatorStyle, GtkWidget* g) {
				mPlugin->mSettings->inactiveIndicatorStyle.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_inactiveIndicatorStyle)));
			}),
			dialog);

		GObject* customIndicatorColors = gtk_builder_get_object(builder, "g_customIndicatorColors");
		gtk_widget_set_sensitive(GTK_WIDGET(customIndicatorColors), !mPlugin->mSettings->indicatorColorFromTheme);

		GObject* indicatorColor = gtk_builder_get_object(builder, "cp_indicatorColor");
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(indicatorColor), mPlugin->mSettings->indicatorColor.get().get());
		g_signal_connect(indicatorColor, "color-set",
			G_CALLBACK(+[](GtkColorButton* _indicatorColor, GtkWidget* g) {
				std::shared_ptr<GdkRGBA> color(g_new(GdkRGBA, 1), g_free);
				gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(_indicatorColor), color.get());
				mPlugin->mSettings->indicatorColor.set(color);
			}),
			dialog);

		GObject* inactiveColor = gtk_builder_get_object(builder, "cp_inactiveColor");
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(inactiveColor), mPlugin->mSettings->inactiveColor.get().get());
		g_signal_connect(inactiveColor, "color-set",
			G_CALLBACK(+[](GtkColorButton* _inactiveColor, GtkWidget* g) {
				std::shared_ptr<GdkRGBA> color(g_new(GdkRGBA, 1), g_free);
				gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(_inactiveColor), color.get());
				mPlugin->mSettings->inactiveColor.set(color);
			}),
			dialog);

		GObject* indicatorColorFromTheme = gtk_builder_get_object(builder, "c_indicatorColorFromTheme");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(indicatorColorFromTheme), mPlugin->mSettings->indicatorColorFromTheme);
		g_signal_connect(indicatorColorFromTheme, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _indicatorColorFromTheme, GtkWidget* _customIndicatorColors) {
				mPlugin->mSettings->indicatorColorFromTheme.set(gtk_toggle_button_get_active(_indicatorColorFromTheme));
				gtk_widget_set_sensitive(GTK_WIDGET(_customIndicatorColors), !mPlugin->mSettings->indicatorColorFromTheme);
			}),
			customIndicatorColors);

		// =====================================================================

		GObject* iconSize = gtk_builder_get_object(builder, "e_iconSize");
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(iconSize))), std::to_string(mPlugin->mSettings->iconSize).c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(iconSize), mPlugin->mSettings->forceIconSize);
		g_signal_connect(iconSize, "changed",
			G_CALLBACK(+[](GtkComboBox* _iconSize) {
				GtkEntry* entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(_iconSize)));
				std::string svalue = Help::String::numericOnly(gtk_entry_get_text(entry));
				int value = std::stoi("0" + svalue);
				mPlugin->mSettings->iconSize.set(value);
				gtk_entry_set_text(entry, svalue.c_str());
				if (value < mPlugin->mSettings->minIconSize || value > mPlugin->mSettings->maxIconSize)
					gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(entry)), GTK_STYLE_CLASS_ERROR);
				else
					gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(entry)), GTK_STYLE_CLASS_ERROR);
			}),
			nullptr);

		GObject* forceIconSize = gtk_builder_get_object(builder, "c_forceIconSize");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(forceIconSize), mPlugin->mSettings->forceIconSize);
		g_signal_connect(forceIconSize, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _forceIconSize, GtkWidget* _iconSize) {
				mPlugin->mSettings->forceIconSize.set(gtk_toggle_button_get_active(_forceIconSize));
				gtk_widget_set_sensitive(GTK_WIDGET(_iconSize), mPlugin->mSettings->forceIconSize);
			}),
			iconSize);

		GObject* previewWidthButton = gtk_builder_get_object(builder, "previewWidthButton");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(previewWidthButton), mPlugin->mSettings->previewWidth);
		g_object_bind_property(showPreviews, "active", previewWidthButton, "sensitive", G_BINDING_SYNC_CREATE);
		g_signal_connect(previewWidthButton, "value-changed",
			G_CALLBACK(+[](GtkSpinButton* _previewWidthButton) {
				mPlugin->mSettings->previewWidth.set(gtk_spin_button_get_value_as_int(_previewWidthButton));
			}),
			nullptr);

		GObject* previewHeightButton = gtk_builder_get_object(builder, "previewHeightButton");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(previewHeightButton), mPlugin->mSettings->previewHeight);
		g_object_bind_property(showPreviews, "active", previewHeightButton, "sensitive", G_BINDING_SYNC_CREATE);
		g_signal_connect(previewHeightButton, "value-changed",
			G_CALLBACK(+[](GtkSpinButton* _previewHeightButton) {
				mPlugin->mSettings->previewHeight.set(gtk_spin_button_get_value_as_int(_previewHeightButton));
			}),
			nullptr);

		// =====================================================================

		GObject* keyComboActiveWarning = gtk_builder_get_object(builder, "c_keyComboActiveWarning");
		GObject* keyComboActive = gtk_builder_get_object(builder, "c_keyComboActive");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyComboActive), mPlugin->mSettings->keyComboActive);
		g_signal_connect(keyComboActive, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _keyComboActive, GtkWidget* tooltip) {
				mPlugin->mSettings->keyComboActive.set(gtk_toggle_button_get_active(_keyComboActive));
				updateKeyComboActiveWarning(tooltip);
			}),
			keyComboActiveWarning);

		GObject* keyComboMinimize = gtk_builder_get_object(builder, "c_keyComboMinimize");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyComboMinimize), mPlugin->mSettings->keyComboMinimize);
		g_signal_connect(keyComboMinimize, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _keyComboMinimize, GtkWidget* tooltip) {
				mPlugin->mSettings->keyComboMinimize.set(gtk_toggle_button_get_active(_keyComboMinimize));
			}),
			nullptr);

		GObject* keyAloneActiveWarning = gtk_builder_get_object(builder, "c_keyAloneActiveWarning");
		GObject* keyAloneActive = gtk_builder_get_object(builder, "c_keyAloneActive");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyAloneActive), mPlugin->mSettings->keyAloneActive);
		g_signal_connect(keyAloneActive, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _keyAloneActive, GtkWidget* tooltip) {
				mPlugin->mSettings->keyAloneActive.set(gtk_toggle_button_get_active(_keyAloneActive));
				updateKeyAloneActiveWarning(tooltip);
			}),
			keyAloneActiveWarning);

		updateKeyComboActiveWarning(GTK_WIDGET(keyComboActiveWarning));
		updateKeyAloneActiveWarning(GTK_WIDGET(keyAloneActiveWarning));

		GObject* shortcutsButton = gtk_builder_get_object(builder, "c_shortcutsButton");
		gchar* path = g_find_program_in_path("xfce4-keyboard-settings");
		if (path != nullptr)
		{
			g_signal_connect(shortcutsButton, "clicked",
				G_CALLBACK(+[](GtkButton* _shortcutsButton) {
					std::string command = "xfce4-keyboard-settings";
					if (mPlugin->mHotkeys->mAddShortcutUIAvailable)
						command += " --shortcuts";

					GError* error = nullptr;
					if (!g_spawn_command_line_async(command.c_str(), &error))
					{
						g_warning("Failed to launch %s: %s", command.c_str(), error->message);
						g_error_free(error);
					}
				}),
				nullptr);
			g_free(path);
		}
		else
		{
			gtk_widget_hide(GTK_WIDGET(shortcutsButton));
		}
	}
} // namespace SettingsDialog

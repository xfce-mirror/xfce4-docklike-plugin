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
#include "Helpers.hpp"

#include <libxfce4ui/libxfce4ui.h>

#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#endif

SettingsDialog::SettingsDialog(Plugin* plugin) : mPlugin(plugin)
{
}

SettingsDialog::~SettingsDialog()
{
	if (mSettingsDialog != nullptr)
		gtk_widget_destroy(mSettingsDialog);
}

void SettingsDialog::updateKeyComboActiveWarning(GtkWidget* widget)
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

void SettingsDialog::updateKeyAloneActiveWarning(GtkWidget* widget)
{
	gtk_widget_set_visible(widget, mPlugin->mSettings->keyAloneActive && !mPlugin->mHotkeys->mKeyAloneGrabbed);
}

void SettingsDialog::popup()
{
	if (mSettingsDialog != nullptr)
	{
		gtk_window_present(GTK_WINDOW(mSettingsDialog));
		return;
	}

	/* Hook to make sure GtkBuilder knows this is an XfceTitledDialog object */
	if (xfce_titled_dialog_get_type() == 0)
		return;

	mBuilder = gtk_builder_new_from_resource("/_dialogs.ui");
	mSettingsDialog = (GtkWidget*)gtk_builder_get_object(mBuilder, "dialog");
	gtk_window_set_role(GTK_WINDOW(mSettingsDialog), "xfce4-panel");
	gtk_widget_show(mSettingsDialog);

	g_signal_connect(
		gtk_builder_get_object(mBuilder, "b_close"), "clicked",
		G_CALLBACK(+[](GtkButton* button, GtkWidget* dialogWindow) {
			gtk_widget_hide(dialogWindow);
			gtk_dialog_response(GTK_DIALOG(dialogWindow), GTK_RESPONSE_NONE);
		}),
		mSettingsDialog);

	g_signal_connect(
		gtk_builder_get_object(mBuilder, "b_help"), "clicked",
		G_CALLBACK(+[](GtkButton* button, GtkWindow* dialogWindow) {
			gtk_show_uri_on_window(dialogWindow, PACKAGE_URL, GDK_CURRENT_TIME, nullptr);
		}),
		mSettingsDialog);

	g_signal_connect(mSettingsDialog, "destroy",
		G_CALLBACK(+[](GtkDialog* dialog, SettingsDialog* me) {
			g_object_unref(me->mBuilder);
			me->mSettingsDialog = nullptr;
		}),
		this);

	g_signal_connect(mSettingsDialog, "response",
		G_CALLBACK(+[](GtkDialog* dialog, gint response) {
			gtk_widget_destroy(GTK_WIDGET(dialog));
		}),
		nullptr);

	// =====================================================================

	GObject* noListForSingleWindow = gtk_builder_get_object(mBuilder, "c_noListForSingleWindow");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(noListForSingleWindow), mPlugin->mSettings->noWindowsListIfSingle);
	g_signal_connect(noListForSingleWindow, "toggled",
		G_CALLBACK(+[](GtkToggleButton* noWindowsListIfSingle, SettingsDialog* me) {
			me->mPlugin->mSettings->noWindowsListIfSingle.set(gtk_toggle_button_get_active(noWindowsListIfSingle));
		}),
		this);

	GObject* onlyDisplayVisible = gtk_builder_get_object(mBuilder, "c_onlyDisplayVisible");
#ifdef ENABLE_X11
	if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
	{
		// window<->workspace association only works on X11
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onlyDisplayVisible), mPlugin->mSettings->onlyDisplayVisible);
		g_signal_connect(onlyDisplayVisible, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _onlyDisplayVisible, SettingsDialog* me) {
				me->mPlugin->mSettings->onlyDisplayVisible.set(gtk_toggle_button_get_active(_onlyDisplayVisible));
				me->mPlugin->mXfw->setVisibleGroups();
			}),
			this);
	}
	else
#endif
	{
		gtk_widget_hide(GTK_WIDGET(onlyDisplayVisible));
	}

	GObject* onlyDisplayScreen = gtk_builder_get_object(mBuilder, "c_onlyDisplayScreen");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onlyDisplayScreen), mPlugin->mSettings->onlyDisplayScreen);
	g_signal_connect(onlyDisplayScreen, "toggled",
		G_CALLBACK(+[](GtkToggleButton* _onlyDisplayScreen, SettingsDialog* me) {
			me->mPlugin->mSettings->onlyDisplayScreen.set(gtk_toggle_button_get_active(_onlyDisplayScreen));
			me->mPlugin->mXfw->setVisibleGroups();
		}),
		this);

	GObject* showPreviews = gtk_builder_get_object(mBuilder, "c_showPreviews");
#ifdef ENABLE_X11
	if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showPreviews), mPlugin->mSettings->showPreviews);
		g_signal_connect(showPreviews, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _showPreviews, SettingsDialog* me) {
				me->mPlugin->mSettings->showPreviews.set(gtk_toggle_button_get_active(_showPreviews));
			}),
			this);
	}
	else
#endif
	{
		gtk_widget_hide(GTK_WIDGET(showPreviews));
	}

	GObject* showWindowCount = gtk_builder_get_object(mBuilder, "c_showWindowCount");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showWindowCount), mPlugin->mSettings->showWindowCount);
	g_signal_connect(showWindowCount, "toggled",
		G_CALLBACK(+[](GtkToggleButton* _showWindowCount, SettingsDialog* me) {
			me->mPlugin->mSettings->showWindowCount.set(gtk_toggle_button_get_active(_showWindowCount));
		}),
		this);

	GObject* disableLauncherCounts = gtk_builder_get_object(mBuilder, "c_disableLauncherCounts");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disableLauncherCounts), mPlugin->mSettings->disableLauncherCounts);
	g_signal_connect(disableLauncherCounts, "toggled",
		G_CALLBACK(+[](GtkToggleButton* _disableLauncherCounts, SettingsDialog* me) {
			me->mPlugin->mSettings->disableLauncherCounts.set(gtk_toggle_button_get_active(_disableLauncherCounts));
		}),
		this);

	GObject* middleButtonBehavior = gtk_builder_get_object(mBuilder, "co_middleButtonBehavior");
	gtk_combo_box_set_active(GTK_COMBO_BOX(middleButtonBehavior), mPlugin->mSettings->middleButtonBehavior);
	g_signal_connect(middleButtonBehavior, "changed",
		G_CALLBACK(+[](GtkComboBox* _middleButtonBehavior, SettingsDialog* me) {
			me->mPlugin->mSettings->middleButtonBehavior.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_middleButtonBehavior)));
		}),
		this);

	// =====================================================================

	GObject* indicatorOrientation = gtk_builder_get_object(mBuilder, "co_indicatorOrientation");
	gtk_combo_box_set_active(GTK_COMBO_BOX(indicatorOrientation), mPlugin->mSettings->indicatorOrientation);
	g_signal_connect(indicatorOrientation, "changed",
		G_CALLBACK(+[](GtkComboBox* _indicatorOrientation, SettingsDialog* me) {
			me->mPlugin->mSettings->indicatorOrientation.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_indicatorOrientation)));
		}),
		this);

	GObject* indicatorStyle = gtk_builder_get_object(mBuilder, "co_indicatorStyle");
	gtk_combo_box_set_active(GTK_COMBO_BOX(indicatorStyle), mPlugin->mSettings->indicatorStyle);
	g_signal_connect(indicatorStyle, "changed",
		G_CALLBACK(+[](GtkComboBox* _indicatorStyle, SettingsDialog* me) {
			me->mPlugin->mSettings->indicatorStyle.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_indicatorStyle)));
		}),
		this);

	GObject* inactiveIndicatorStyle = gtk_builder_get_object(mBuilder, "co_inactiveIndicatorStyle");
	gtk_combo_box_set_active(GTK_COMBO_BOX(inactiveIndicatorStyle), mPlugin->mSettings->inactiveIndicatorStyle);
	g_signal_connect(inactiveIndicatorStyle, "changed",
		G_CALLBACK(+[](GtkComboBox* _inactiveIndicatorStyle, SettingsDialog* me) {
			me->mPlugin->mSettings->inactiveIndicatorStyle.set(gtk_combo_box_get_active(GTK_COMBO_BOX(_inactiveIndicatorStyle)));
		}),
		this);

	GObject* customIndicatorColors = gtk_builder_get_object(mBuilder, "g_customIndicatorColors");
	gtk_widget_set_sensitive(GTK_WIDGET(customIndicatorColors), !mPlugin->mSettings->indicatorColorFromTheme);

	GObject* indicatorColor = gtk_builder_get_object(mBuilder, "cp_indicatorColor");
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(indicatorColor), mPlugin->mSettings->indicatorColor.get().get());
	g_signal_connect(indicatorColor, "color-set",
		G_CALLBACK(+[](GtkColorButton* _indicatorColor, SettingsDialog* me) {
			std::shared_ptr<GdkRGBA> color(g_new(GdkRGBA, 1), g_free);
			gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(_indicatorColor), color.get());
			me->mPlugin->mSettings->indicatorColor.set(color);
		}),
		this);

	GObject* inactiveColor = gtk_builder_get_object(mBuilder, "cp_inactiveColor");
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(inactiveColor), mPlugin->mSettings->inactiveColor.get().get());
	g_signal_connect(inactiveColor, "color-set",
		G_CALLBACK(+[](GtkColorButton* _inactiveColor, SettingsDialog* me) {
			std::shared_ptr<GdkRGBA> color(g_new(GdkRGBA, 1), g_free);
			gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(_inactiveColor), color.get());
			me->mPlugin->mSettings->inactiveColor.set(color);
		}),
		this);

	GObject* indicatorColorFromTheme = gtk_builder_get_object(mBuilder, "c_indicatorColorFromTheme");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(indicatorColorFromTheme), mPlugin->mSettings->indicatorColorFromTheme);
	g_signal_connect(indicatorColorFromTheme, "toggled",
		G_CALLBACK(+[](GtkToggleButton* _indicatorColorFromTheme, SettingsDialog* me) {
			GObject* _customIndicatorColors = gtk_builder_get_object(me->mBuilder, "g_customIndicatorColors");
			me->mPlugin->mSettings->indicatorColorFromTheme.set(gtk_toggle_button_get_active(_indicatorColorFromTheme));
			gtk_widget_set_sensitive(GTK_WIDGET(_customIndicatorColors), !me->mPlugin->mSettings->indicatorColorFromTheme);
		}),
		this);

	// =====================================================================

	GObject* iconSize = gtk_builder_get_object(mBuilder, "e_iconSize");
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(iconSize))), std::to_string(mPlugin->mSettings->iconSize).c_str());
	gtk_widget_set_sensitive(GTK_WIDGET(iconSize), mPlugin->mSettings->forceIconSize);
	g_signal_connect(iconSize, "changed",
		G_CALLBACK(+[](GtkComboBox* _iconSize, SettingsDialog* me) {
			GtkEntry* entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(_iconSize)));
			std::string svalue = Help::String::numericOnly(gtk_entry_get_text(entry));
			int value = std::stoi("0" + svalue);
			me->mPlugin->mSettings->iconSize.set(value);
			gtk_entry_set_text(entry, svalue.c_str());
			if (value < me->mPlugin->mSettings->minIconSize || value > me->mPlugin->mSettings->maxIconSize)
				gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(entry)), GTK_STYLE_CLASS_ERROR);
			else
				gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(entry)), GTK_STYLE_CLASS_ERROR);
		}),
		this);

	GObject* forceIconSize = gtk_builder_get_object(mBuilder, "c_forceIconSize");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(forceIconSize), mPlugin->mSettings->forceIconSize);
	g_signal_connect(forceIconSize, "toggled",
		G_CALLBACK(+[](GtkToggleButton* _forceIconSize, SettingsDialog* me) {
			GObject* _iconSize = gtk_builder_get_object(me->mBuilder, "e_iconSize");
			me->mPlugin->mSettings->forceIconSize.set(gtk_toggle_button_get_active(_forceIconSize));
			gtk_widget_set_sensitive(GTK_WIDGET(_iconSize), me->mPlugin->mSettings->forceIconSize);
		}),
		this);

	GObject* previewWidthButton = gtk_builder_get_object(mBuilder, "previewWidthButton");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(previewWidthButton), mPlugin->mSettings->previewWidth);
	g_object_bind_property(showPreviews, "active", previewWidthButton, "sensitive", G_BINDING_SYNC_CREATE);
	g_signal_connect(previewWidthButton, "value-changed",
		G_CALLBACK(+[](GtkSpinButton* _previewWidthButton, SettingsDialog* me) {
			me->mPlugin->mSettings->previewWidth.set(gtk_spin_button_get_value_as_int(_previewWidthButton));
		}),
		this);

	GObject* previewHeightButton = gtk_builder_get_object(mBuilder, "previewHeightButton");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(previewHeightButton), mPlugin->mSettings->previewHeight);
	g_object_bind_property(showPreviews, "active", previewHeightButton, "sensitive", G_BINDING_SYNC_CREATE);
	g_signal_connect(previewHeightButton, "value-changed",
		G_CALLBACK(+[](GtkSpinButton* _previewHeightButton, SettingsDialog* me) {
			me->mPlugin->mSettings->previewHeight.set(gtk_spin_button_get_value_as_int(_previewHeightButton));
		}),
		this);

	// =====================================================================

	GObject* keyComboActiveWarning = gtk_builder_get_object(mBuilder, "c_keyComboActiveWarning");
	GObject* keyComboActive = gtk_builder_get_object(mBuilder, "c_keyComboActive");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyComboActive), mPlugin->mSettings->keyComboActive);
	g_signal_connect(keyComboActive, "toggled",
		G_CALLBACK(+[](GtkToggleButton* _keyComboActive, SettingsDialog* me) {
			GObject* _keyComboActiveWarning = gtk_builder_get_object(me->mBuilder, "c_keyComboActiveWarning");
			me->mPlugin->mSettings->keyComboActive.set(gtk_toggle_button_get_active(_keyComboActive));
			me->updateKeyComboActiveWarning(GTK_WIDGET(_keyComboActiveWarning));
		}),
		this);

	GObject* keyComboMinimize = gtk_builder_get_object(mBuilder, "c_keyComboMinimize");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyComboMinimize), mPlugin->mSettings->keyComboMinimize);
	g_signal_connect(keyComboMinimize, "toggled",
		G_CALLBACK(+[](GtkToggleButton* _keyComboMinimize, SettingsDialog* me) {
			me->mPlugin->mSettings->keyComboMinimize.set(gtk_toggle_button_get_active(_keyComboMinimize));
		}),
		this);

	GObject* keyAloneActiveWarning = gtk_builder_get_object(mBuilder, "c_keyAloneActiveWarning");
	GObject* keyAloneActive = gtk_builder_get_object(mBuilder, "c_keyAloneActive");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyAloneActive), mPlugin->mSettings->keyAloneActive);
	g_signal_connect(keyAloneActive, "toggled",
		G_CALLBACK(+[](GtkToggleButton* _keyAloneActive, SettingsDialog* me) {
			GObject* _keyAloneActiveWarning = gtk_builder_get_object(me->mBuilder, "c_keyAloneActiveWarning");
			me->mPlugin->mSettings->keyAloneActive.set(gtk_toggle_button_get_active(_keyAloneActive));
			me->updateKeyAloneActiveWarning(GTK_WIDGET(_keyAloneActiveWarning));
		}),
		this);

	updateKeyComboActiveWarning(GTK_WIDGET(keyComboActiveWarning));
	updateKeyAloneActiveWarning(GTK_WIDGET(keyAloneActiveWarning));

	GObject* shortcutsButton = gtk_builder_get_object(mBuilder, "c_shortcutsButton");
	gchar* path = g_find_program_in_path("xfce4-keyboard-settings");
	if (path != nullptr)
	{
		g_signal_connect(shortcutsButton, "clicked",
			G_CALLBACK(+[](GtkButton* _shortcutsButton, SettingsDialog* me) {
				std::string command = "xfce4-keyboard-settings";
				if (me->mPlugin->mHotkeys->mAddShortcutUIAvailable)
					command += " --shortcuts";

				GError* error = nullptr;
				if (!g_spawn_command_line_async(command.c_str(), &error))
				{
					g_warning("Failed to launch %s: %s", command.c_str(), error->message);
					g_error_free(error);
				}
			}),
			this);
		g_free(path);
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(shortcutsButton));
	}
}

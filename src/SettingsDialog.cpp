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
#include "Plugin.hpp"
#ifdef ENABLE_X11
#include "Hotkeys.hpp"
#endif

namespace SettingsDialog
{
	GtkWidget* mSettingsDialog = nullptr;

// ---------------------------------------------------------------------------
// Pinned Shortcuts panel: rebuild rows inside the given GtkBox container.
// ---------------------------------------------------------------------------

static void rebuildPinnedPanel(GtkBox* listBox, GtkWindow* parentWindow);

static void rebuildPinnedPanel(GtkBox* listBox, GtkWindow* parentWindow)
{
	// Remove all existing children
	GList* children = gtk_container_get_children(GTK_CONTAINER(listBox));
	for (GList* l = children; l != nullptr; l = l->next)
		gtk_widget_destroy(GTK_WIDGET(l->data));
	g_list_free(children);

	std::vector<PinnedAppEntry> entries = Settings::loadPinnedAppEntries();
	const int totalEntries = (int)entries.size();

	static const char defaultKeyLabels[10][4] = {
		"1","2","3","4","5","6","7","8","9","0"
	};

	const int KEY_BTN_WIDTH  = 140;
	const int REMOVE_BTN_WIDTH = 20;
	const int KEY_ROW_SPACING = 2;
	const int ADD_KEY_WIDTH  = KEY_BTN_WIDTH + KEY_ROW_SPACING + REMOVE_BTN_WIDTH;

	for (int idx = 0; idx < totalEntries; ++idx)
	{
		const PinnedAppEntry& entry = entries[idx];
		std::shared_ptr<AppInfo> appInfo = AppInfos::search(entry.id);

		// ---- Card frame for this row ----
		GtkWidget* rowFrame = gtk_frame_new(nullptr);
		gtk_frame_set_shadow_type(GTK_FRAME(rowFrame), GTK_SHADOW_ETCHED_IN);
		gtk_widget_set_margin_bottom(rowFrame, 6);

		// ---- Outer grid: 5 columns [app-info | sep | custom-keys | sep | reorder] ----
		GtkWidget* grid = gtk_grid_new();
		gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
		gtk_widget_set_margin_start(grid, 10);
		gtk_widget_set_margin_end(grid, 6);
		gtk_widget_set_margin_top(grid, 8);
		gtk_widget_set_margin_bottom(grid, 8);
		gtk_container_add(GTK_CONTAINER(rowFrame), grid);

		// ========== COLUMN 0: App icon + name + default-key checkbox ==========
		GtkWidget* leftVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
		gtk_widget_set_hexpand(leftVBox, TRUE);
		gtk_widget_set_valign(leftVBox, GTK_ALIGN_CENTER);

		GtkWidget* appHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		GtkWidget* iconWidget = gtk_image_new();
		if (appInfo && !appInfo->mIcon.empty())
		{
			if (appInfo->mIcon[0] == '/' && g_file_test(appInfo->mIcon.c_str(), G_FILE_TEST_IS_REGULAR))
			{
				GdkPixbuf* pb = gdk_pixbuf_new_from_file_at_size(appInfo->mIcon.c_str(), 24, 24, nullptr);
				gtk_image_set_from_pixbuf(GTK_IMAGE(iconWidget), pb);
				if (pb) g_object_unref(pb);
			}
			else
				gtk_image_set_from_icon_name(GTK_IMAGE(iconWidget), appInfo->mIcon.c_str(), GTK_ICON_SIZE_LARGE_TOOLBAR);
		}
		else
			gtk_image_set_from_icon_name(GTK_IMAGE(iconWidget), "application-x-executable", GTK_ICON_SIZE_LARGE_TOOLBAR);

		GtkWidget* nameLabel = gtk_label_new(appInfo ? appInfo->mName.c_str() : entry.path.c_str());
		gtk_label_set_ellipsize(GTK_LABEL(nameLabel), PANGO_ELLIPSIZE_END);
		gtk_label_set_xalign(GTK_LABEL(nameLabel), 0.0f);
		gtk_widget_set_hexpand(nameLabel, TRUE);

		gtk_box_pack_start(GTK_BOX(appHBox), iconWidget, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(appHBox), nameLabel, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(leftVBox), appHBox, FALSE, FALSE, 0);

		// Default-key checkbox (positions 0..9 only)
		if (idx < 10)
		{
			gchar* checkMarkup = g_strdup_printf(
				"<span letter_spacing='1024'>Super + %s</span>",
				defaultKeyLabels[idx]);
			GtkWidget* defaultKeyCheck = gtk_check_button_new();
			GtkWidget* checkLbl = gtk_label_new(nullptr);
			gtk_label_set_markup(GTK_LABEL(checkLbl), checkMarkup);
			g_free(checkMarkup);
			gtk_widget_set_margin_start(checkLbl, 4);
			gtk_container_add(GTK_CONTAINER(defaultKeyCheck), checkLbl);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(defaultKeyCheck), !entry.defaultKeyDisabled);
			gtk_widget_set_margin_top(defaultKeyCheck, 4);

			struct DefaultKeyData {
				int index;
				GtkBox* listBox;
				GtkWindow* parentWindow;
			};
			DefaultKeyData* dkd = new DefaultKeyData{idx, listBox, parentWindow};

			g_signal_connect_data(defaultKeyCheck, "toggled",
				G_CALLBACK(+[](GtkToggleButton* btn, DefaultKeyData* data) {
					std::vector<PinnedAppEntry> es = Settings::loadPinnedAppEntries();
					if (data->index < (int)es.size())
					{
						es[data->index].defaultKeyDisabled = !gtk_toggle_button_get_active(btn);
						Settings::savePinnedAppEntries(es);
					}
				}),
				dkd,
				[](gpointer data, GClosure*) { delete static_cast<DefaultKeyData*>(data); },
				(GConnectFlags)0);

			gtk_box_pack_start(GTK_BOX(leftVBox), defaultKeyCheck, FALSE, FALSE, 0);
		}

		gtk_grid_attach(GTK_GRID(grid), leftVBox, 0, 0, 1, 1);

		// ---- Vertical separator ----
		GtkWidget* sep1 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
		gtk_widget_set_margin_start(sep1, 4);
		gtk_widget_set_margin_end(sep1, 4);
		gtk_grid_attach(GTK_GRID(grid), sep1, 1, 0, 1, 1);

		// ========== COLUMN 2: Custom Keys ==========
		GtkWidget* rightVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
		gtk_widget_set_valign(rightVBox, GTK_ALIGN_CENTER);

		GtkWidget* keysLabel = gtk_label_new(nullptr);
		gtk_label_set_markup(GTK_LABEL(keysLabel), "<b>Custom Keys</b>");
		gtk_widget_set_halign(keysLabel, GTK_ALIGN_START);
		gtk_widget_set_margin_bottom(keysLabel, 2);
		gtk_box_pack_start(GTK_BOX(rightVBox), keysLabel, FALSE, FALSE, 0);

		for (int ki = 0; ki < (int)entry.customKeys.size(); ++ki)
		{
			const std::string& keyStr = entry.customKeys[ki];
			GtkWidget* keyRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, KEY_ROW_SPACING);

#ifdef ENABLE_X11
			std::string displayLabel = Hotkeys::accelToReadableLabel(keyStr);
#else
			std::string displayLabel = keyStr;
#endif
			GtkWidget* keyBtn = gtk_button_new_with_label(displayLabel.c_str());
			gtk_widget_set_size_request(keyBtn, KEY_BTN_WIDTH, -1);
			gtk_widget_set_margin_top(keyBtn, 1);
			gtk_widget_set_margin_bottom(keyBtn, 1);

			struct KeyAssignData {
				int appIdx;
				int keyIdx;
				GtkBox* listBox;
				GtkWindow* parentWindow;
			};
			KeyAssignData* kad = new KeyAssignData{idx, ki, listBox, parentWindow};

			g_signal_connect_data(keyBtn, "clicked",
				G_CALLBACK(+[](GtkButton* btn, KeyAssignData* data) {
#ifdef ENABLE_X11
					std::string newKey = Hotkeys::captureKey(data->parentWindow);
					if (newKey.empty()) return;
					std::vector<PinnedAppEntry> es = Settings::loadPinnedAppEntries();
					if (data->appIdx < (int)es.size() && data->keyIdx < (int)es[data->appIdx].customKeys.size())
					{
						es[data->appIdx].customKeys[data->keyIdx] = newKey;
						Settings::savePinnedAppEntries(es);
						rebuildPinnedPanel(data->listBox, data->parentWindow);
					}
#endif
				}),
				kad,
				[](gpointer data, GClosure*) { delete static_cast<KeyAssignData*>(data); },
				(GConnectFlags)0);

			GtkWidget* removeBtn = gtk_button_new_from_icon_name("list-remove-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
			gtk_button_set_relief(GTK_BUTTON(removeBtn), GTK_RELIEF_NONE);
			gtk_widget_set_size_request(removeBtn, 20, 20);
			gtk_widget_set_tooltip_text(removeBtn, _("Remove this key"));
			gtk_widget_set_valign(removeBtn, GTK_ALIGN_CENTER);

			struct KeyRemoveData {
				int appIdx;
				int keyIdx;
				GtkBox* listBox;
				GtkWindow* parentWindow;
			};
			KeyRemoveData* krd = new KeyRemoveData{idx, ki, listBox, parentWindow};

			g_signal_connect_data(removeBtn, "clicked",
				G_CALLBACK(+[](GtkButton* btn, KeyRemoveData* data) {
					std::vector<PinnedAppEntry> es = Settings::loadPinnedAppEntries();
					if (data->appIdx < (int)es.size() && data->keyIdx < (int)es[data->appIdx].customKeys.size())
					{
						es[data->appIdx].customKeys.erase(es[data->appIdx].customKeys.begin() + data->keyIdx);
						Settings::savePinnedAppEntries(es);
						rebuildPinnedPanel(data->listBox, data->parentWindow);
					}
				}),
				krd,
				[](gpointer data, GClosure*) { delete static_cast<KeyRemoveData*>(data); },
				(GConnectFlags)0);

			gtk_box_pack_start(GTK_BOX(keyRow), keyBtn,    FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(keyRow), removeBtn, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(rightVBox), keyRow, FALSE, FALSE, 0);
		}

		// "+ Add Key" button
		GtkWidget* addKeyBtn = gtk_button_new_with_label(_("+ Add Key"));
		gtk_widget_set_size_request(addKeyBtn, ADD_KEY_WIDTH, -1);
		gtk_widget_set_halign(addKeyBtn, GTK_ALIGN_START);
		gtk_widget_set_margin_top(addKeyBtn, 2);

		struct KeyAddData {
			int appIdx;
			GtkBox* listBox;
			GtkWindow* parentWindow;
		};
		KeyAddData* kadd = new KeyAddData{idx, listBox, parentWindow};

		g_signal_connect_data(addKeyBtn, "clicked",
			G_CALLBACK(+[](GtkButton* btn, KeyAddData* data) {
#ifdef ENABLE_X11
				std::string newKey = Hotkeys::captureKey(data->parentWindow);
				if (newKey.empty()) return;
				std::vector<PinnedAppEntry> es = Settings::loadPinnedAppEntries();
				if (data->appIdx < (int)es.size())
				{
					es[data->appIdx].customKeys.push_back(newKey);
					Settings::savePinnedAppEntries(es);
					rebuildPinnedPanel(data->listBox, data->parentWindow);
				}
#endif
			}),
			kadd,
			[](gpointer data, GClosure*) { delete static_cast<KeyAddData*>(data); },
			(GConnectFlags)0);

		gtk_box_pack_start(GTK_BOX(rightVBox), addKeyBtn, FALSE, FALSE, 0);

		gtk_grid_attach(GTK_GRID(grid), rightVBox, 2, 0, 1, 1);

		// ---- Vertical separator ----
		GtkWidget* sep2 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
		gtk_widget_set_margin_start(sep2, 2);
		gtk_widget_set_margin_end(sep2, 2);
		gtk_grid_attach(GTK_GRID(grid), sep2, 3, 0, 1, 1);

		// ========== COLUMN 4: Reorder ==========
		GtkWidget* reorderVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_valign(reorderVBox, GTK_ALIGN_CENTER);

		GtkWidget* upBtn   = gtk_button_new_from_icon_name("go-up-symbolic",   GTK_ICON_SIZE_SMALL_TOOLBAR);
		GtkWidget* downBtn = gtk_button_new_from_icon_name("go-down-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_button_set_relief(GTK_BUTTON(upBtn),   GTK_RELIEF_NONE);
		gtk_button_set_relief(GTK_BUTTON(downBtn), GTK_RELIEF_NONE);
		gtk_widget_set_size_request(upBtn,   20, 20);
		gtk_widget_set_size_request(downBtn, 20, 20);
		gtk_widget_set_sensitive(upBtn,   idx > 0);
		gtk_widget_set_sensitive(downBtn, idx < totalEntries - 1);
		gtk_widget_set_tooltip_text(upBtn,   _("Move up"));
		gtk_widget_set_tooltip_text(downBtn, _("Move down"));

		struct MoveData {
			int appIdx;
			GtkBox* listBox;
			GtkWindow* parentWindow;
		};

		MoveData* moveUp = new MoveData{idx, listBox, parentWindow};
		g_signal_connect_data(upBtn, "clicked",
			G_CALLBACK(+[](GtkButton* btn, MoveData* md) {
				std::vector<PinnedAppEntry> es = Settings::loadPinnedAppEntries();
				int i = md->appIdx;
				if (i > 0)
				{
					std::swap(es[i], es[i - 1]);
					// Reorder the canonical pinnedAppList
					std::list<std::string> newIds;
					for (const auto& e : es) newIds.push_back(e.id);
					Settings::pinnedAppList.set(newIds);
					// Save only the extra metadata (keys / disabled flags)
					Settings::savePinnedAppEntries(es);
					Dock::drawGroups();
					rebuildPinnedPanel(md->listBox, md->parentWindow);
				}
			}),
			moveUp,
			[](gpointer data, GClosure*) { delete static_cast<MoveData*>(data); },
			(GConnectFlags)0);

		MoveData* moveDown = new MoveData{idx, listBox, parentWindow};
		g_signal_connect_data(downBtn, "clicked",
			G_CALLBACK(+[](GtkButton* btn, MoveData* md) {
				std::vector<PinnedAppEntry> es = Settings::loadPinnedAppEntries();
				int i = md->appIdx;
				if (i < (int)es.size() - 1)
				{
					std::swap(es[i], es[i + 1]);
					// Reorder the canonical pinnedAppList
					std::list<std::string> newIds;
					for (const auto& e : es) newIds.push_back(e.id);
					Settings::pinnedAppList.set(newIds);
					// Save only the extra metadata (keys / disabled flags)
					Settings::savePinnedAppEntries(es);
					Dock::drawGroups();
					rebuildPinnedPanel(md->listBox, md->parentWindow);
				}
			}),
			moveDown,
			[](gpointer data, GClosure*) { delete static_cast<MoveData*>(data); },
			(GConnectFlags)0);

		gtk_box_pack_start(GTK_BOX(reorderVBox), upBtn,   FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(reorderVBox), downBtn, FALSE, FALSE, 0);

		gtk_grid_attach(GTK_GRID(grid), reorderVBox, 4, 0, 1, 1);

		gtk_box_pack_start(GTK_BOX(listBox), rowFrame, FALSE, TRUE, 0);
		gtk_widget_show_all(rowFrame);
	}

	if (totalEntries == 0)
	{
		GtkWidget* emptyLabel = gtk_label_new(_("No pinned applications."));
		gtk_widget_set_margin_top(emptyLabel, 12);
		gtk_widget_set_margin_bottom(emptyLabel, 12);
		gtk_widget_set_halign(emptyLabel, GTK_ALIGN_CENTER);
		gtk_box_pack_start(GTK_BOX(listBox), emptyLabel, FALSE, FALSE, 0);
		gtk_widget_show(emptyLabel);
	}
}

// ---------------------------------------------------------------------------

	void popup()
	{
		if (mSettingsDialog != nullptr)
		{
			gtk_window_present(GTK_WINDOW(mSettingsDialog));
			return;
		}

		/* Hook to make sure GtkBuilder knows this is an XfceTitledDialog object */
		if (xfce_titled_dialog_get_type() == 0)
			return;

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
				gtk_show_uri_on_window(dialogWindow, HELP_WEBSITE, GDK_CURRENT_TIME, nullptr);
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
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(noListForSingleWindow), Settings::noWindowsListIfSingle);
		g_signal_connect(noListForSingleWindow, "toggled",
			G_CALLBACK(+[](GtkToggleButton* noWindowsListIfSingle) {
				Settings::noWindowsListIfSingle.set(gtk_toggle_button_get_active(noWindowsListIfSingle));
			}),
			nullptr);

		GObject* onlyDisplayVisible = gtk_builder_get_object(builder, "c_onlyDisplayVisible");
#ifdef ENABLE_X11
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
		{
			// window<->workspace association only works on X11
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onlyDisplayVisible), Settings::onlyDisplayVisible);
			g_signal_connect(onlyDisplayVisible, "toggled",
				G_CALLBACK(+[](GtkToggleButton* _onlyDisplayVisible) {
					Settings::onlyDisplayVisible.set(gtk_toggle_button_get_active(_onlyDisplayVisible));
					Xfw::setVisibleGroups();
				}),
				nullptr);
		}
		else
#endif
		{
			gtk_widget_hide(GTK_WIDGET(onlyDisplayVisible));
		}

		GObject* onlyDisplayScreen = gtk_builder_get_object(builder, "c_onlyDisplayScreen");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(onlyDisplayScreen), Settings::onlyDisplayScreen);
		g_signal_connect(onlyDisplayScreen, "toggled",
			G_CALLBACK(+[](GtkToggleButton* _onlyDisplayScreen) {
				Settings::onlyDisplayScreen.set(gtk_toggle_button_get_active(_onlyDisplayScreen));
				Xfw::setVisibleGroups();
			}),
			nullptr);

		GObject* showPreviews = gtk_builder_get_object(builder, "c_showPreviews");
#ifdef ENABLE_X11
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(showPreviews), Settings::showPreviews);
			g_signal_connect(showPreviews, "toggled",
				G_CALLBACK(+[](GtkToggleButton* _showPreviews) {
					Settings::showPreviews.set(gtk_toggle_button_get_active(_showPreviews));
				}),
				nullptr);
		}
		else
#endif
		{
			gtk_widget_hide(GTK_WIDGET(showPreviews));
		}

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

		GObject* previewWidthButton = gtk_builder_get_object(builder, "previewWidthButton");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(previewWidthButton), Settings::previewWidth);
		g_object_bind_property(showPreviews, "active", previewWidthButton, "sensitive", G_BINDING_SYNC_CREATE);
		g_signal_connect(previewWidthButton, "value-changed",
			G_CALLBACK(+[](GtkSpinButton* _previewWidthButton) {
				Settings::previewWidth.set(gtk_spin_button_get_value_as_int(_previewWidthButton));
			}),
			nullptr);

		GObject* previewHeightButton = gtk_builder_get_object(builder, "previewHeightButton");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(previewHeightButton), Settings::previewHeight);
		g_object_bind_property(showPreviews, "active", previewHeightButton, "sensitive", G_BINDING_SYNC_CREATE);
		g_signal_connect(previewHeightButton, "value-changed",
			G_CALLBACK(+[](GtkSpinButton* _previewHeightButton) {
				Settings::previewHeight.set(gtk_spin_button_get_value_as_int(_previewHeightButton));
			}),
			nullptr);

		// =====================================================================

#ifdef ENABLE_X11
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
		{
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
		else
#endif
		{
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "hotkeysFrame")));
		}

		// =====================================================================
		// Pinned Application Shortcuts panel — lives in its own notebook tab
		// =====================================================================

		GtkWidget* notebook = GTK_WIDGET(gtk_builder_get_object(builder, "notebook_main"));
		if (notebook != nullptr)
		{
			// Create the "Pinned Shortcuts" tab box
			GtkWidget* pinnedTabBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
			gtk_widget_set_margin_start(pinnedTabBox, 6);
			gtk_widget_set_margin_end(pinnedTabBox, 6);
			gtk_widget_set_margin_top(pinnedTabBox, 6);
			gtk_widget_set_margin_bottom(pinnedTabBox, 6);
			gtk_widget_show(pinnedTabBox);

			// Scrolled window
			GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
			gtk_widget_set_vexpand(scrolled, TRUE);
			gtk_widget_set_size_request(scrolled, -1, 200);

			// Inner list box
			GtkBox* listBox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 2));
			gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(listBox));
			gtk_box_pack_start(GTK_BOX(pinnedTabBox), scrolled, TRUE, TRUE, 0);

			// Note label at the bottom of the tab
			GtkWidget* noteLabel = gtk_label_new(nullptr);
			gtk_label_set_markup(GTK_LABEL(noteLabel),
				"<small><i>Default keys are position-based (Super+1 … Super+0) and apply to the first 10 pinned apps.</i></small>");
			gtk_label_set_line_wrap(GTK_LABEL(noteLabel), TRUE);
			gtk_label_set_xalign(GTK_LABEL(noteLabel), 0.0f);
			gtk_widget_set_margin_top(noteLabel, 4);
			gtk_widget_set_margin_start(noteLabel, 4);
			gtk_widget_set_margin_end(noteLabel, 4);
			gtk_box_pack_start(GTK_BOX(pinnedTabBox), noteLabel, FALSE, FALSE, 0);

			// Tab label
			GtkWidget* tabLabel = gtk_label_new(_("Pinned Shortcuts"));

			gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pinnedTabBox, tabLabel);
			gtk_widget_show_all(pinnedTabBox);

			// Populate rows
			rebuildPinnedPanel(listBox, GTK_WINDOW(dialog));
		}
	}

#ifdef ENABLE_X11
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
#endif
} // namespace SettingsDialog

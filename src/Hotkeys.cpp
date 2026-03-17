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

#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>
#include <gdk/gdkx.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define ModifierChange 85 //? this event type isn't listed in libX11

namespace Hotkeys
{
	int mGrabbedKeys;
	bool mHotkeysHandling;

	bool mXIExtAvailable;
	int mXIOpcode;
	pthread_t mThread;

	uint mSuperLKeycode, mSuperRKeycode, m1Keycode;

	// Custom-key grab tracking: keycode+modmask → app-path
	struct CustomGrab
	{
		unsigned int keycode;
		unsigned int modmask;
		std::string appPath;
	};
	std::vector<CustomGrab> mCustomGrabs;

	// =========================================================================

	// Parse an accel string like "ctrl+alt+f" or "<Control><Alt>F".
	// Returns false if parsing fails.
	static bool parseAccel(const std::string& accel, guint& keyval, GdkModifierType& mods)
	{
		// Try GTK-style "<Modifier>key" first
		gtk_accelerator_parse(accel.c_str(), &keyval, &mods);
		if (keyval != 0)
			return true;

		// Try simple "+"-separated form: "ctrl+alt+f"
		guint parsedMods = 0;
		keyval = 0;
		std::string tok;
		std::istringstream ss(accel);
		while (std::getline(ss, tok, '+'))
		{
			std::string lower = tok;
			for (auto& c : lower)
				c = tolower(c);
			if (lower == "ctrl" || lower == "control")
				parsedMods |= GDK_CONTROL_MASK;
			else if (lower == "alt" || lower == "mod1")
				parsedMods |= GDK_MOD1_MASK;
			else if (lower == "shift")
				parsedMods |= GDK_SHIFT_MASK;
			else if (lower == "super" || lower == "mod4")
				parsedMods |= GDK_MOD4_MASK;
			else
			{
				keyval = gdk_keyval_from_name(tok.c_str());
				if (keyval == GDK_KEY_VoidSymbol)
					keyval = gdk_keyval_from_name(lower.c_str());
			}
		}
		if (keyval != 0 && keyval != GDK_KEY_VoidSymbol)
		{
			mods = (GdkModifierType)parsedMods;
			return true;
		}
		return false;
	}

	static void grabUngrabCustomKey(GdkWindow* rootwin, GdkDisplay* display,
		unsigned int keycode, unsigned int modmask, bool grab)
	{
		for (int ignoredModifiers : {0, (int)GDK_MOD2_MASK, (int)GDK_LOCK_MASK,
				 (int)(GDK_MOD2_MASK | GDK_LOCK_MASK)})
		{
			if (grab)
			{
				gdk_x11_display_error_trap_push(display);
				XGrabKey(GDK_WINDOW_XDISPLAY(rootwin), keycode,
					modmask | ignoredModifiers,
					GDK_WINDOW_XID(rootwin), False, GrabModeAsync, GrabModeAsync);
				gint grabError = gdk_x11_display_error_trap_pop(display);
				(void)grabError;
			}
			else
			{
				XUngrabKey(GDK_WINDOW_XDISPLAY(rootwin), keycode,
					modmask | ignoredModifiers,
					GDK_WINDOW_XID(rootwin));
			}
		}
	}

	// =========================================================================

	static GdkFilterReturn hotkeysHandler(GdkXEvent* gdk_xevent, GdkEvent* event, gpointer data)
	{
		XEvent* xevent = (XEvent*)gdk_xevent;

		switch (xevent->type)
		{
		case ModifierChange:
			if (GDK_MOD4_MASK & xevent->xkey.keycode)
				Dock::hoverSupered(true);
			else
				Dock::hoverSupered(false);
			break;
		case KeyPress:
			if (xevent->xkey.keycode >= m1Keycode && xevent->xkey.keycode <= m1Keycode + NbHotkeys)
			{
				Dock::activateGroup(xevent->xkey.keycode - m1Keycode, xevent->xkey.time);
			}
			else
			{
				// Check custom grabs
				unsigned int stateMask = xevent->xkey.state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD4_MASK);
				for (const auto& cg : mCustomGrabs)
				{
					if (cg.keycode == xevent->xkey.keycode && cg.modmask == stateMask)
					{
						Dock::activateGroupByPath(cg.appPath, xevent->xkey.time);
						break;
					}
				}
			}
			break;
		}
		return GDK_FILTER_CONTINUE;
	}

	static void startStopHotkeysHandler(bool start)
	{
		if (start && !mHotkeysHandling)
		{
			gdk_window_add_filter(nullptr, hotkeysHandler, nullptr);
			mHotkeysHandling = true;
		}
		else if (!start && mHotkeysHandling)
		{
			gdk_window_remove_filter(nullptr, hotkeysHandler, nullptr);
			mHotkeysHandling = false;
		}
	}

	static void grabUngrabHotkeys(bool grab, unsigned int startKey = 0)
	{
		GdkWindow* rootwin = gdk_get_default_root_window();
		GdkDisplay* display = gdk_window_get_display(rootwin);

		if (grab)
			mGrabbedKeys = NbHotkeys;
		else
			mGrabbedKeys = startKey;

		for (uint k = m1Keycode + startKey; k < m1Keycode + NbHotkeys; k++)
		{
			for (int ignoredModifiers : {0, (int)GDK_MOD2_MASK, (int)GDK_LOCK_MASK, (int)(GDK_MOD2_MASK | GDK_LOCK_MASK)})
				if (grab)
				{
					gdk_x11_display_error_trap_push(display);

					XGrabKey(
						GDK_WINDOW_XDISPLAY(rootwin),
						k, GDK_MOD4_MASK | ignoredModifiers,
						GDK_WINDOW_XID(rootwin),
						False,
						GrabModeAsync,
						GrabModeAsync);

					if (gdk_x11_display_error_trap_pop(display))
					{
						grabUngrabHotkeys(false, k - m1Keycode);
						return;
					}
				}
				else
				{
					XUngrabKey(
						GDK_WINDOW_XDISPLAY(rootwin),
						k, GDK_MOD4_MASK | ignoredModifiers,
						GDK_WINDOW_XID(rootwin));
				}
		}
	}

	/* =========================================================================
	 *
	 * The method used here to listen keyboard events globaly is taken from :
	 * github.com/anko/xkbcat
	 * It create a direct connection to X11 keyboard events without any grabbing,
	 * allowing us to determine the state (consumed or not) of the modifier key when released.
	 */

	static gboolean threadSafeSwitch(gpointer data)
	{
		Xfw::switchToLastWindow(g_get_monotonic_time() / 1000);
		return false;
	}

	static void* threadedXIKeyListenner(void* data)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

		Display* display = XOpenDisplay(nullptr);

		// Register events
		Window root = DefaultRootWindow(display);
		XIEventMask m;
		m.deviceid = XIAllMasterDevices;
		m.mask_len = XIMaskLen(XI_LASTEVENT);
		m.mask = g_new0(unsigned char, m.mask_len);
		XISetMask(m.mask, XI_RawKeyPress);
		XISetMask(m.mask, XI_RawKeyRelease);
		XISelectEvents(display, root, &m, 1);
		XSync(display, false);
		free(m.mask);

		bool toTrigger = false;
		while (true)
		{
			XEvent event;
			XGenericEventCookie* cookie = (XGenericEventCookie*)&event.xcookie;
			XNextEvent(display, &event);
			if (XGetEventData(display, cookie) && cookie->type == GenericEvent && cookie->extension == mXIOpcode)
			{
				uint keycode = ((XIRawEvent*)cookie->data)->detail;
				if (cookie->evtype == XI_RawKeyRelease)
					if (keycode == mSuperLKeycode || keycode == mSuperRKeycode)
						if (toTrigger)
							gdk_threads_add_idle(threadSafeSwitch, nullptr);
				if (cookie->evtype == XI_RawKeyPress)
				{
					if (keycode == mSuperLKeycode || keycode == mSuperRKeycode)
						toTrigger = true;
					else
						toTrigger = false;
				}
			}
		}
	}

	static void startStopXIKeyListenner(bool start)
	{
		if (mXIExtAvailable && start)
		{
			if (!mThread)
				pthread_create(&mThread, nullptr, threadedXIKeyListenner, nullptr);
			else if (mThread)
			{
				pthread_cancel(mThread); // also close the XDisplay in the thread
				void* ret = nullptr;
				pthread_join(mThread, &ret);
				mThread = 0;
			}
		}
	}

	static void checkXIExtension(Display* display)
	{
		mXIExtAvailable = false;

		// Test for XInput 2 extension
		int queryEvent, queryError;
		if (!XQueryExtension(display, "XInputExtension", &mXIOpcode, &queryEvent, &queryError))
			return;

		// Request XInput 2.0, guarding against changes in future versions
		int major = 2, minor = 0;
		int queryResult = XIQueryVersion(display, &major, &minor);
		if (queryResult == BadRequest)
			return;
		else if (queryResult != Success)
			return;

		mXIExtAvailable = true;
		mThread = 0;
	}

	// =========================================================================

	void updateCustomKeys()
	{
		GdkWindow* rootwin = gdk_get_default_root_window();
		GdkDisplay* display = gdk_window_get_display(rootwin);
		Display* xdisplay = GDK_WINDOW_XDISPLAY(rootwin);

		// Ungrab all previous custom grabs
		for (const auto& cg : mCustomGrabs)
			grabUngrabCustomKey(rootwin, display, cg.keycode, cg.modmask, false);
		mCustomGrabs.clear();

		if (!Settings::keyComboActive)
		{
			startStopHotkeysHandler(mGrabbedKeys > 0);
			return;
		}

		std::vector<PinnedAppEntry> entries = Settings::loadPinnedAppEntries();
		for (const auto& entry : entries)
		{
			for (const auto& keyStr : entry.customKeys)
			{
				guint keyval = 0;
				GdkModifierType mods = (GdkModifierType)0;
				if (!parseAccel(keyStr, keyval, mods))
					continue;

				int keycode = XKeysymToKeycode(xdisplay, keyval);
				if (!keycode)
					continue;

				unsigned int modmask = (unsigned int)mods & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD4_MASK);
				grabUngrabCustomKey(rootwin, display, keycode, modmask, true);

				CustomGrab cg;
				cg.keycode = keycode;
				cg.modmask = modmask;
				cg.appPath = entry.path;
				mCustomGrabs.push_back(cg);
			}
		}

		// Ensure handler is active whenever there are any grabs
		startStopHotkeysHandler(mGrabbedKeys > 0 || !mCustomGrabs.empty());
	}

	void updateSettings()
	{
		startStopXIKeyListenner(Settings::keyAloneActive);

		grabUngrabHotkeys(Settings::keyComboActive);
		updateCustomKeys();
		startStopHotkeysHandler(mGrabbedKeys > 0 || !mCustomGrabs.empty());
	}

	void init()
	{
		Display* display = XOpenDisplay(nullptr);

		checkXIExtension(display);

		mSuperLKeycode = XKeysymToKeycode(display, XK_Super_L);
		mSuperRKeycode = XKeysymToKeycode(display, XK_Super_R);
		m1Keycode = XKeysymToKeycode(display, XK_1);

		XCloseDisplay(display);

		mGrabbedKeys = 0;
		mHotkeysHandling = false;

		updateSettings();
	}

	// =========================================================================
	// Convert a GTK accel string to a clean human-readable label.
	// gtk_accelerator_get_label() returns raw X11 names like "Mod4", "Mod1"
	// for Super/Alt on many distros, so we fix those up here.

	std::string accelToReadableLabel(const std::string& accel)
	{
		if (accel.empty())
			return accel;

		guint keyval = 0;
		GdkModifierType mods = (GdkModifierType)0;
		gtk_accelerator_parse(accel.c_str(), &keyval, &mods);

		// Build modifier prefix with friendly names
		std::string label;
		if (mods & GDK_MOD4_MASK)
			label += "Super+";
		if (mods & GDK_CONTROL_MASK)
			label += "Ctrl+";
		if (mods & GDK_MOD1_MASK)
			label += "Alt+";
		if (mods & GDK_SHIFT_MASK)
			label += "Shift+";

		// Key name: use GDK's name, upper-case the first character
		const gchar* keyName = gdk_keyval_name(keyval);
		if (keyName)
		{
			std::string k(keyName);
			if (!k.empty())
				k[0] = g_ascii_toupper(k[0]);
			label += k;
		}
		return label.empty() ? accel : label;
	}

	// =========================================================================
	// Key capture dialog: shows a dialog, waits for a key press, returns accel string.

	std::string captureKey(GtkWindow* parent)
	{
		std::string result = "";

		GtkWidget* dialog = gtk_dialog_new_with_buttons(
			_("Press a Key Combination"),
			parent,
			(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			nullptr);

		GtkWidget* label = gtk_label_new(_("Press the desired key combination.\nPress Escape to cancel."));
		gtk_widget_set_margin_start(label, 12);
		gtk_widget_set_margin_end(label, 12);
		gtk_widget_set_margin_top(label, 12);
		gtk_widget_set_margin_bottom(label, 12);

		GtkWidget* keyLabel = gtk_label_new(_("Waiting for key..."));
		gtk_widget_set_margin_bottom(keyLabel, 12);

		GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(content), keyLabel, FALSE, FALSE, 0);
		gtk_widget_show_all(dialog);

		struct CaptureData
		{
			std::string result;
			GtkWidget* keyLabel;
			GtkDialog* dialog;
			bool captured;
		};
		CaptureData cd = {"", keyLabel, GTK_DIALOG(dialog), false};

		gulong keyPressId = g_signal_connect(dialog, "key-press-event",
			G_CALLBACK(+[](GtkWidget* widget, GdkEventKey* event, CaptureData* captureData) -> gboolean {
				if (event->keyval == GDK_KEY_Escape)
				{
					gtk_dialog_response(captureData->dialog, GTK_RESPONSE_CANCEL);
					return TRUE;
				}
				// Ignore bare modifiers
				if (event->keyval == GDK_KEY_Control_L || event->keyval == GDK_KEY_Control_R || event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R || event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R || event->keyval == GDK_KEY_Super_L || event->keyval == GDK_KEY_Super_R)
					return TRUE;

				// Build accel string
				guint modMask = event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD4_MASK);
				gchar* accel = gtk_accelerator_name(event->keyval, (GdkModifierType)modMask);
				captureData->result = accel ? accel : "";
				g_free(accel);
				captureData->captured = true;

				// Show readable label
				std::string readableLabel = accelToReadableLabel(captureData->result);
				gtk_label_set_text(GTK_LABEL(captureData->keyLabel), readableLabel.c_str());

				gtk_dialog_response(captureData->dialog, GTK_RESPONSE_OK);
				return TRUE;
			}),
			&cd);

		gint response = gtk_dialog_run(GTK_DIALOG(dialog));
		g_signal_handler_disconnect(dialog, keyPressId);

		if (response == GTK_RESPONSE_OK && cd.captured)
			result = cd.result;

		gtk_widget_destroy(dialog);
		return result;
	}

} // namespace Hotkeys

/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Hotkeys.hpp"
#include "Settings.hpp"

#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>
#include <gdk/gdkx.h>

#include <iostream>
#include <string>

#define ModifierChange 85 //? this event type isn't listed in libX11

namespace Hotkeys
{
	int mGrabbedKeys;
	bool mHotkeysHandling;

	bool mXIExtAvailable;
	int mXIOpcode;
	pthread_t mThread;

	uint mSuperLKeycode, mSuperRKeycode, m1Keycode;

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
				Dock::activateGroup(xevent->xkey.keycode - m1Keycode, xevent->xkey.time);
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

	void updateSettings()
	{
		startStopXIKeyListenner(Settings::keyAloneActive);

		grabUngrabHotkeys(Settings::keyComboActive);
		startStopHotkeysHandler(mGrabbedKeys > 0);
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
} // namespace Hotkeys

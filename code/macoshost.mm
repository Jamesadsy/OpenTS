/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#import <AppKit/AppKit.h>

#include "hostruntime.hh"

#include "keyboard.h"

#include <chrono>
#include <cstdio>
#include <deque>
#include <thread>


class MacOpenTSHost final : public OpenTSHost
{
	public:
		virtual char const * Name(void) const override { return("Cocoa"); }

		virtual int Run_Game(int, char **) override
		{
			fprintf(stderr, "The macOS game runtime is not enabled by this host-only diagnostic.\n");
			return(1);
		}

		virtual bool Create_Diagnostic_Window(int width, int height) override
		{
			@try {
				Application = [NSApplication sharedApplication];
				[Application setActivationPolicy:NSApplicationActivationPolicyRegular];
				[Application finishLaunching];

				NSRect const frame = NSMakeRect(0.0, 0.0, width, height);
				NSWindowStyleMask const style = NSWindowStyleMaskTitled
					| NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
				Window = [[NSWindow alloc] initWithContentRect:frame
					styleMask:style backing:NSBackingStoreBuffered defer:NO];
				if (Window == nil) {
					return(false);
				}

				[Window setTitle:@"OpenTS Second Sun host proof"];
				[Window setReleasedWhenClosed:NO];
				[Window setAcceptsMouseMovedEvents:YES];
				[Window center];
				View = [Window contentView];
				[View setWantsLayer:YES];
				[Window makeKeyAndOrderFront:nil];
				[Application activateIgnoringOtherApps:YES];
				[Application updateWindows];

				Running = true;
				LastFocus = Is_Focused();
				OpenTSHostEvent focus;
				focus.Type = OPENTS_HOST_EVENT_FOCUS;
				focus.Focused = LastFocus;
				Events.push_back(focus);
				return(View != nil);
			} @catch (NSException * exception) {
				fprintf(stderr, "Cocoa window creation failed: %s\n", [[exception reason] UTF8String]);
				return(false);
			}
		}

		virtual NativeWindow Native_Window(void) const override
		{
			return({NATIVE_WINDOW_COCOA, nullptr, (__bridge void *)View});
		}

		virtual bool Drawable_Size(int & width, int & height) const override
		{
			if (View == nil || Window == nil) {
				return(false);
			}
			NSRect const pixels = [View convertRectToBacking:[View bounds]];
			width = (int)pixels.size.width;
			height = (int)pixels.size.height;
			return(width > 0 && height > 0);
		}

		virtual int Refresh_Rate(void) const override
		{
			NSScreen * screen = [Window screen];
			return(screen != nil ? (int)[screen maximumFramesPerSecond] : 0);
		}

		virtual bool Window_Is_Visible(void) const override
		{
			return(Window != nil && [Window isVisible] && [Window screen] != nil);
		}

		virtual bool Is_Focused(void) const override
		{
			return(Application != nil && [Application isActive] && Window != nil && [Window isKeyWindow]);
		}

		virtual bool Is_Running(void) const override
		{
			return(Running && Window != nil && [Window isVisible]);
		}

		virtual void Pump_Events(void) override
		{
			if (Application == nil) {
				return;
			}

			for (;;) {
				NSEvent * event = [Application nextEventMatchingMask:NSEventMaskAny
					untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
				if (event == nil) {
					break;
				}
				Translate_Event(event);
				[Application sendEvent:event];
			}
			[Application updateWindows];

			bool const focused = Is_Focused();
			if (focused != LastFocus) {
				LastFocus = focused;
				OpenTSHostEvent event;
				event.Type = OPENTS_HOST_EVENT_FOCUS;
				event.Focused = focused;
				Events.push_back(event);
			}
			if (Window == nil || ![Window isVisible]) {
				if (Running) {
					OpenTSHostEvent event;
					event.Type = OPENTS_HOST_EVENT_QUIT;
					Events.push_back(event);
				}
				Running = false;
			}
		}

		virtual bool Poll_Event(OpenTSHostEvent & event) override
		{
			if (Events.empty()) {
				return(false);
			}
			event = Events.front();
			Events.pop_front();
			return(true);
		}

		virtual void Wait_Milliseconds(unsigned int milliseconds) override
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
		}

		virtual void Request_Quit(void) override
		{
			Running = false;
			OpenTSHostEvent event;
			event.Type = OPENTS_HOST_EVENT_QUIT;
			Events.push_back(event);
			[Window orderOut:nil];
		}

		virtual void Destroy_Window(void) override
		{
			if (Window != nil) {
				[Window orderOut:nil];
				[Window close];
			}
			View = nil;
			Window = nil;
			Running = false;
			Events.clear();
		}

	private:
		static unsigned short Translate_Key(NSEvent * event)
		{
			NSString * characters = [event charactersIgnoringModifiers];
			if ([characters length] == 0) {
				return(VK_NONE);
			}

			unichar character = [characters characterAtIndex:0];
			if (character >= 'a' && character <= 'z') {
				character -= 'a' - 'A';
			}
			if ((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9')) {
				return((unsigned short)character);
			}

			switch (character) {
				case NSUpArrowFunctionKey: return(VK_UP);
				case NSDownArrowFunctionKey: return(VK_DOWN);
				case NSLeftArrowFunctionKey: return(VK_LEFT);
				case NSRightArrowFunctionKey: return(VK_RIGHT);
				case NSHomeFunctionKey: return(VK_HOME);
				case NSEndFunctionKey: return(VK_END);
				case NSPageUpFunctionKey: return(VK_PRIOR);
				case NSPageDownFunctionKey: return(VK_NEXT);
				case NSDeleteFunctionKey: return(VK_DELETE);
				case NSF1FunctionKey: return(VK_F1);
				case NSF2FunctionKey: return(VK_F2);
				case NSF3FunctionKey: return(VK_F3);
				case NSF4FunctionKey: return(VK_F4);
				case NSF5FunctionKey: return(VK_F5);
				case NSF6FunctionKey: return(VK_F6);
				case NSF7FunctionKey: return(VK_F7);
				case NSF8FunctionKey: return(VK_F8);
				case NSF9FunctionKey: return(VK_F9);
				case NSF10FunctionKey: return(VK_F10);
				case NSF11FunctionKey: return(VK_F11);
				case NSF12FunctionKey: return(VK_F12);
				case 0x1B: return(VK_ESCAPE);
				case '\r': return(VK_RETURN);
				case '\t': return(VK_TAB);
				case 0x7F: return(VK_BACK);
				case ' ': return(VK_SPACE);
				default: return(VK_NONE);
			}
		}

		void Push_Key(NSEvent * nativeevent, unsigned short key, bool release)
		{
			if (key == VK_NONE) {
				return;
			}
			NSEventModifierFlags const flags = [nativeevent modifierFlags];
			OpenTSHostEvent event;
			event.Type = OPENTS_HOST_EVENT_KEY;
			event.Key = key;
			event.Release = release;
			event.Shift = (flags & NSEventModifierFlagShift) != 0;
			event.Control = (flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0;
			event.Alt = (flags & NSEventModifierFlagOption) != 0;
			Events.push_back(event);
		}

		void Push_Mouse(NSEvent * nativeevent, OpenTSHostEventType type, unsigned short button, bool release)
		{
			NSPoint point = [View convertPoint:[nativeevent locationInWindow] fromView:nil];
			OpenTSHostEvent event;
			event.Type = type;
			event.Key = button;
			event.X = (int)point.x;
			NSRect const bounds = [View bounds];
			event.Y = (int)(bounds.size.height - point.y);
			event.Release = release;
			Events.push_back(event);
		}

		void Translate_Event(NSEvent * nativeevent)
		{
			switch ([nativeevent type]) {
				case NSEventTypeKeyDown:
					if (![nativeevent isARepeat]) {
						Push_Key(nativeevent, Translate_Key(nativeevent), false);
					}
					break;

				case NSEventTypeKeyUp:
					Push_Key(nativeevent, Translate_Key(nativeevent), true);
					break;

				case NSEventTypeFlagsChanged: {
					unsigned short key = VK_NONE;
					NSEventModifierFlags mask = 0;
					switch ([nativeevent keyCode]) {
						case 56: case 60: key = VK_SHIFT; mask = NSEventModifierFlagShift; break;
						case 59: case 62: key = VK_CONTROL; mask = NSEventModifierFlagControl; break;
						case 58: case 61: key = VK_MENU; mask = NSEventModifierFlagOption; break;
						default: break;
					}
					Push_Key(nativeevent, key, ([nativeevent modifierFlags] & mask) == 0);
					break;
				}

				case NSEventTypeMouseMoved:
				case NSEventTypeLeftMouseDragged:
				case NSEventTypeRightMouseDragged:
				case NSEventTypeOtherMouseDragged:
					Push_Mouse(nativeevent, OPENTS_HOST_EVENT_MOUSE_MOVE, VK_NONE, false);
					break;

				case NSEventTypeLeftMouseDown:
				case NSEventTypeLeftMouseUp:
					Push_Mouse(nativeevent, OPENTS_HOST_EVENT_MOUSE_BUTTON, VK_LBUTTON,
						[nativeevent type] == NSEventTypeLeftMouseUp);
					break;

				case NSEventTypeRightMouseDown:
				case NSEventTypeRightMouseUp:
					Push_Mouse(nativeevent, OPENTS_HOST_EVENT_MOUSE_BUTTON, VK_RBUTTON,
						[nativeevent type] == NSEventTypeRightMouseUp);
					break;

				case NSEventTypeOtherMouseDown:
				case NSEventTypeOtherMouseUp:
					Push_Mouse(nativeevent, OPENTS_HOST_EVENT_MOUSE_BUTTON, VK_MBUTTON,
						[nativeevent type] == NSEventTypeOtherMouseUp);
					break;

				default:
					break;
			}
		}

		NSApplication * Application = nil;
		NSWindow * Window = nil;
		NSView * View = nil;
		bool Running = false;
		bool LastFocus = false;
		std::deque<OpenTSHostEvent> Events;
};


int main(int argc, char ** argv)
{
	@autoreleasepool {
		MacOpenTSHost host;
		return(OpenTS_Run(argc, argv, host));
	}
}

# CnC TS iOS Public Test — testing and known issues

This page is for the **CnC TS for iPhone/iPad — Public Test 055** build.

Install and data setup are covered in [IOS_INSTALL.md](IOS_INSTALL.md).

File iOS-port reports in the fork's
[CnC TS iOS Public Test issue form](https://github.com/Jamesadsy/OpenTS/issues/new?template=public-test-055-ios.yml).
Do not attach original or proprietary game data.

## What this build is for

Public Test 055 carries the accepted single-player line and adds a physically accepted local LAN scenario for two iPhones over trusted private Wi-Fi.

The two-iPhone LAN test completed discovery, host/join/lobby, two-way chat, synchronized map/options, entry into the same match and gameplay commands. The session continued for about five minutes without a stall, desync or peer drop; controls and normal audio/input behavior remained intact. Public testing may broaden device, iOS and network coverage beyond that tested setup.

Expected working areas include:

- base Tiberian Sun and Firestorm selection;
- GDI and Nod campaign play;
- save, load and autosave;
- separate Tiberian Sun / Firestorm save handling;
- FMV/movie playback;
- music, speech and sound effects;
- iOS Ring/Silent-aware audio;
- normal iOS headphone routing;
- touch interaction;
- controller basics;
- tactical camera/pan controls;
- Repair/Sell mode feedback;
- CnC TS Home Screen identity.

## First two-iPhone LAN test

1. Install the same accepted Public Test 055 build on both iPhones and provide
   compatible game data on each.
2. Connect both to the same trusted private Wi-Fi and allow Local Network
   permission when prompted. For the initial test, avoid VPNs, guest Wi-Fi, or
   networks with client isolation.
3. On Device A, choose **Multiplayer Game > Network > Host**.
4. On Device B, choose **Multiplayer Game > Network**, discover or select
   Device A, join its lobby, and start.

Two-iPhone LAN is the physically accepted public scenario. PC-to-iOS
interoperability is not claimed.

## Current known limitations

Please do not report these as new regressions unless your result is materially different from the description below.

### Contextual tactical cursor artwork

The tactical pointer is intentionally kept on the accepted stable/static presentation fallback. The pointer itself can move and gameplay actions still resolve, but the displayed cursor artwork does not yet change reliably through every native Tiberian Sun contextual shape.

### Controller pointer can begin invisible

On a cold boot, the controller pointer can be logically active before its pointer image first becomes visible. Subsequent pointer/touch interaction can make it appear.

### Circle/Back is not universal

Circle/cancel behavior works through the native right-click/cancel path in gameplay, but not every legacy menu/screen has complete controller Back parity.

### Triangle/sidebar collapse is disabled

Triangle intentionally does not collapse the sidebar in this build. An earlier implementation caused presentation corruption, so the accepted product keeps this action inert until a safe replacement is proven.

### Legacy RestateMission recap

The old mission recap / RestateMission screen does not yet have complete controller navigation parity.

### LAN acceptance scope

Physical acceptance covers a two-iPhone session over trusted private Wi-Fi. Public testing may broaden device and network coverage beyond that setup.

## Useful touch behavior

The accepted iOS build maps touch into the game's native pointer/input path.

- one-finger tap: primary interaction at that point;
- one-finger drag: pointer/selection interaction;
- press and hold: native secondary/right-button behavior;
- two-finger drag: tactical camera pan;
- while a movie is playing, a stationary hold can skip it.

The build preserves the real game input semantics wherever possible rather than implementing a separate touch-only simulation layer.

## Controller smoke test

If you have a supported SDL3-recognized controller, useful checks are:

- pointer movement from the first front-end screens;
- menu navigation with the D-pad;
- primary/cancel input;
- tactical right-stick camera movement;
- R2 pointer-speed boost;
- Square Repair/Sell cycling;
- movie-skip hold behavior.

Triangle/sidebar collapse is intentionally disabled as described above.

When reporting controller issues, include the exact controller model and whether it was connected before or after launching CnC TS.

## Suggested 10-minute smoke

A good first test is:

1. cold-launch CnC TS;
2. choose base Tiberian Sun;
3. start a GDI or Nod campaign mission;
4. move/select/order units with touch;
5. play a movie or briefing with sound;
6. save the mission;
7. load the save;
8. return to the selector and launch Firestorm;
9. connect headphones and confirm iOS routes audio normally;
10. if using a controller, repeat a short menu/tactical input pass.

Also try the Ring/Silent switch. Public Test 055 intentionally becomes silent when the device is in silent mode and resumes normal game audio when silent mode is off.

## Diagnostics

The app's player state is under:

~~~text
Documents/OpenTS/User
~~~

Depending on the failure, that folder can contain saves, settings, engine logs or crash/debug information.

### Touch logs

Touch logging is enabled when this folder exists:

~~~text
Documents/OpenTS/User/touchlog
~~~

Create the **touchlog** folder before launching the game if you are trying to capture a touch/input problem. Each run can leave its own flushed diagnostic log there.

Before sharing logs publicly, inspect them and redact anything personal such as local paths, hostnames, usernames or network addresses.

**Do not upload Tiberian Sun MIX archives, movies, audio, maps or other proprietary game data with a bug report.**

## What to include in an issue

Use the fork-local
[CnC TS iOS Public Test issue form](https://github.com/Jamesadsy/OpenTS/issues/new?template=public-test-055-ios.yml).
It collects the device, iOS version, build, IPA hash, source commit, data
source, mode, reproduction steps, and logs for this test build.

Please include:

~~~text
Device:
iOS/iPadOS version:
CnC TS build: Public Test 055
IPA SHA-256:
Sideload method: Sideloadly / SideStore / other
Game data source: CnCNet/community installer / Steam / EA App / other lawful source
OpenTS source commit: 559d91546c54d1ff4e999b7a47d0a7298f69b0c9
Mode: Tiberian Sun / Firestorm
Campaign/mission or screen:
Controller model (if used):

Steps to reproduce:
1.
2.
3.

Expected result:

Actual result:

Does it reproduce after a cold launch?

Does touch still work?
Does controller input still work?
Does audio still work?
Does the game continue simulating?

Attachments:
- screenshot/video if useful
- relevant log(s) from OpenTS/User
- relevant touchlog file if this is an input issue
~~~

Never attach your original game archives or other proprietary owner data.
Use the fork's issue form for iOS-port reports; upstream OpenTS is not the
primary support destination for this fork's iOS build.

## Build identity

Public Test 055 is pinned to:

~~~text
Commit: 559d91546c54d1ff4e999b7a47d0a7298f69b0c9
Tree:   f0302eb04a94e4fe0f12392f9a97085a5cbf27d6
IPA:    OpenTS-unsigned.ipa
Size:   8,522,557 bytes
SHA256: 536A41CC0B8797D7C00CE61E097B7398FC24CE9AC888D77F5C3FFFF6308DD921
~~~

If your IPA does not have that SHA-256, say so in the report and do not assume it is the accepted Public Test 055 binary.

# CnC TS for iPhone/iPad — Public Test 055

CnC TS is the experimental iPhone/iPad build carried by this OpenTS fork.

It is an unofficial community test build. It is not affiliated with or endorsed by Electronic Arts, Westwood Studios, Apple, or the upstream OpenTS project.

The app contains the OpenTS engine only. **It does not include Tiberian Sun or Firestorm game data.** You must provide your own lawfully obtained compatible game data from a Tiberian Sun / Firestorm installation you own.

## Quick start

**Platform scope:** The easy Quickstart data-preparation path requires a
Windows PC. Sideloadly can run on Windows or macOS, but there is no tested
Mac-only workflow for acquiring and preparing Tiberian Sun / Firestorm data.
Mac users who already have a prepared `OpenTS` folder can use a Mac to sideload
and transfer it.

1. Obtain compatible Tiberian Sun / Firestorm data from a lawful source. The
   [CnCNet how-to](https://cncnet.org/tiberian-sun/how-to-play) links to the
   [C&C Communications Centre installer](https://cnc-comm.com/tiberian-sun/downloads/the-game/installer),
   a community/freeware acquisition route. Its installer output has not been
   directly qualified against the 055 helper; do not treat it as proven
   plug-and-play. Include its campaign, music, and movie components for the
   complete iOS experience. The [Steam](https://store.steampowered.com/bundle/39394/)
   and [EA App](https://www.ea.com/games/command-and-conquer/command-and-conquer-the-ultimate-collection)
   Ultimate Collection are lawful retail alternatives.
2. From the [Public Test 055 release](https://github.com/Jamesadsy/OpenTS/releases/tag/cnc-ts-ios-preview-055), download `OpenTS-unsigned.ipa` and `CnC-TS-iOS-Quickstart.zip`.
3. On Windows, extract the Quickstart ZIP and run `Prepare-CnC-TS-iOS.cmd`.
   Paste or drag the installed game folder and IPA into the prompts. The
   launcher uses the Windows Documents known folder, including a redirected
   or OneDrive-backed Documents folder. Press Enter to use
   `Windows Documents\CnC-TS-iOS-Ready\OpenTS`.
4. Sign and sideload the unchanged IPA with [Sideloadly](https://sideloadly.io/)
   on Windows or macOS, or use [SideStore](https://docs.sidestore.io/docs/installation/install).
   On iOS/iPadOS 16 or later, Developer Mode may be required when prompted.
   Free Apple developer signing commonly needs refresh or re-signing every
   seven days; follow the sideloading tool's current instructions:
   [Sideloadly FAQ](https://sideloadly.io/faq) or [SideStore guide](https://docs.sidestore.io/docs/installation/install).
5. Launch CnC TS once, then close it. This initializes and exposes the app's
   file area; do not expect gameplay until `OpenTS/Data` is present.
6. For a full-folder transfer, use iCloud Drive. On Windows with iCloud for
   Windows set up, copy the complete `OpenTS` folder from
   `Windows Documents\CnC-TS-iOS-Ready` into iCloud Drive and wait for sync.
   On the iPhone or iPad, open Files > Browse > iCloud Drive, long-press
   `OpenTS`, choose Move, then select On My iPhone/iPad > CnC TS. Confirm the
   final paths are `On My iPhone/iPad > CnC TS > OpenTS > Data` and
   `On My iPhone/iPad > CnC TS > OpenTS > User`. See Apple's
   [iCloud for Windows file guide](https://support.apple.com/en-gb/guide/icloud-windows/icwddbc813bd/icloud)
   and [iPhone Files guide](https://support.apple.com/guide/iphone/set-up-icloud-drive-iphbbcf8827d/27/ios/27).
   Apple Devices File Sharing and Sideloadly App File Sharing are alternatives;
   nested-folder behavior through those routes has not been physically verified
   here.
7. On an existing install, preserve `OpenTS/User` and its saves. Update or
   replace `OpenTS/Data` separately; do not leave a duplicate `OpenTS 2` folder.
   Launch CnC TS again after the data is in place.

The C&C community installer's output was not directly tested for this release.
The launcher accepts the installed Tiberian Sun / Firestorm folder and the helper
copies its files recursively, including subfolders. Select the folder containing
the installed game files, not the installer download. The Quickstart validates
the accepted Public Test 055 IPA and creates `OpenTS\Data` and `OpenTS\User`
without modifying the source folder.

## Supported public-test target

- iPhone or iPad
- arm64
- iOS/iPadOS 16.0 or later
- landscape orientation
- unsigned IPA, signed by the tester during sideloading

Public Test 055 is built from:

- source commit: **559d91546c54d1ff4e999b7a47d0a7298f69b0c9**
- source tree: **f0302eb04a94e4fe0f12392f9a97085a5cbf27d6**
- IPA filename: **OpenTS-unsigned.ipa**
- expected size: **8,522,557 bytes**
- SHA-256: **536A41CC0B8797D7C00CE61E097B7398FC24CE9AC888D77F5C3FFFF6308DD921**

Download it from the [Public Test 055 release](https://github.com/Jamesadsy/OpenTS/releases/tag/cnc-ts-ios-preview-055).

## 1. Verify the IPA

On Windows PowerShell:

~~~powershell
Get-FileHash .\OpenTS-unsigned.ipa -Algorithm SHA256
~~~

The result must be:

~~~text
536A41CC0B8797D7C00CE61E097B7398FC24CE9AC888D77F5C3FFFF6308DD921
~~~

If the hash does not match, do not install that file.

## 2. Prepare your own Tiberian Sun data

The on-device layout is:

~~~text
Documents/
└── OpenTS/
    ├── Data/   <- Tiberian Sun / Firestorm runtime data
    └── User/   <- saves, settings, logs and diagnostics
~~~

The engine uses **Documents/OpenTS/Data** as its game-data root and **Documents/OpenTS/User** as writable player state.

### Direct PowerShell use

If you prefer to invoke the authoritative helper directly, download this
repository and run it on Windows:

~~~powershell
powershell -ExecutionPolicy Bypass -File .\scripts\public\prepare-ios-data.ps1 -SourceDir "C:\Path\To\Your\Tiberian Sun" -Ipa "C:\Path\To\OpenTS-unsigned.ipa" -OutputDir "C:\Temp\CnC-TS-iOS"
~~~

The script:

- reads only your local Tiberian Sun installation;
- verifies the Public Test 055 IPA size and SHA-256;
- excludes Windows executables, libraries, editor/manual/debug material and local saves/settings that the iOS build does not need;
- preserves the legally owned runtime archives, including required movie data;
- extracts the engine-owned **ui** directory and **Language.dat** from the IPA;
- creates a transfer-ready **OpenTS/Data** and empty **OpenTS/User** layout;
- never uploads or transmits your game data.

If the chosen output folder already contains files, the script stops unless you explicitly pass **-Force**.

To validate the script with synthetic files only:

~~~powershell
powershell -ExecutionPolicy Bypass -File .\scripts\public\prepare-ios-data.ps1 -SelfTest
~~~

It should end with:

~~~text
SELF-TEST PASS
~~~

### Manual preparation

These folder-layout instructions do not establish a tested Mac-only
game-data acquisition or preparation workflow. The supplied preparation helper
requires Windows.

If you do not use the script, create an **OpenTS** folder with **Data** and **User** beneath it.

Copy the runtime files from your own Tiberian Sun / Firestorm installation into **OpenTS/Data**. Do not copy the Windows executable, DLLs, editor, manuals, existing saves or SUN.INI.

The iOS engine also needs the **ui** directory and **Language.dat** that are shipped inside the IPA at:

~~~text
Payload/OpenTS.app/ui
Payload/OpenTS.app/Language.dat
~~~

Those engine-owned files belong in:

~~~text
OpenTS/Data/ui
OpenTS/Data/Language.dat
~~~

The supplied PowerShell helper does this automatically and is less error-prone.

## 3. Sideload the unsigned IPA

You are responsible for signing the unsigned IPA with your own Apple account/certificate during sideloading.

### Option A — Sideloadly

Use only the official Sideloadly site:

https://sideloadly.io/

At a high level:

1. Install Sideloadly on Windows or macOS.
2. Connect and trust your iPhone/iPad.
3. Load **OpenTS-unsigned.ipa**.
4. Select your device and provide the Apple account Sideloadly will use for signing.
5. Start the sideload.
6. Follow any trust or Developer Mode prompts shown by iOS.

Sideloadly's own site and FAQ are the authority for current signing/setup requirements:

https://sideloadly.io/faq

Do not download Sideloadly from unofficial mirrors.

### Option B — SideStore

Follow the current SideStore prerequisites and installation guide:

https://docs.sidestore.io/docs/installation/prerequisites

https://docs.sidestore.io/docs/installation/install

Once SideStore is working, import/install **OpenTS-unsigned.ipa** through SideStore. Keep the SideStore-required local VPN/refresh setup working according to its current documentation.

Apple's development signing rules can require periodic app refreshes, especially with a free Apple account. That is a sideloading-platform limitation rather than a CnC TS game limitation.

## 4. Launch CnC TS once

After installation, launch **CnC TS** once and then exit. This first launch
only initializes and exposes the app's file area. Do not expect gameplay until
`OpenTS/Data` is present; no particular error or blank-screen behavior is
promised.

The app creates its writable Files-visible structure beneath its Documents container.

Because the app enables iOS File Sharing, you should be able to reach it through the Files app as the CnC TS app folder.

## 5. Copy the prepared game data

For a full-folder transfer from Windows, copy the complete **OpenTS** folder
from Windows Documents into iCloud Drive with iCloud for Windows. Wait for
sync. In Files on the iPhone or iPad, long-press **OpenTS**, choose **Move**,
then choose **On My iPhone/iPad > CnC TS**. The [Windows iCloud guide](https://support.apple.com/en-gb/guide/icloud-windows/icwddbc813bd/icloud)
and [iPhone Files guide](https://support.apple.com/guide/iphone/set-up-icloud-drive-iphbbcf8827d/27/ios/27)
explain access on each device.

The final paths must be:

~~~text
On My iPhone/iPad / CnC TS / OpenTS / Data
On My iPhone/iPad / CnC TS / OpenTS / User
~~~

Apple Devices File Sharing and Sideloadly App File Sharing are alternatives;
nested-folder behavior through those routes has not been physically verified
here.

If **OpenTS/User** already contains saves or settings you want to keep, leave
that folder in place and update **Data** separately. Do not accept a duplicate
**OpenTS 2** folder as the final result.

The complete runtime data is large because the movie archives are included. Allow enough free storage and enough time for the transfer to finish.

## 6. Launch and test

Launch CnC TS again.

A useful first smoke test is:

1. reach the product selector;
2. start base Tiberian Sun;
3. enter a GDI or Nod campaign;
4. confirm music, speech and movie audio;
5. save and reload once;
6. return to the selector and confirm Firestorm is available.

If the game is silent, first check the iPhone/iPad Ring/Silent setting. Public Test 055 intentionally follows the iOS silent switch. Connected headphones should be selected by normal iOS audio routing.

See [IOS_TESTING.md](IOS_TESTING.md) for controls, known issues and what to include in a bug report.

## Two-iPhone LAN testing

Public Test 055 passed physical acceptance for LAN play between two iPhones over trusted private Wi-Fi. Public testing may broaden device and network coverage beyond that tested setup.

For a first test:

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

## Saves and reinstalls

Player state lives in:

~~~text
Documents/OpenTS/User
~~~

Back up that folder before deleting/reinstalling the app if you care about your saves or settings. iOS can remove an app's data container when the app is deleted.

Do not upload original MIX archives, movies, audio files or other proprietary Tiberian Sun data when reporting a bug.

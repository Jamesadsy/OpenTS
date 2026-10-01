# CnC TS for iPhone/iPad — Public Test 055

CnC TS is the experimental iPhone/iPad build carried by this OpenTS fork.

It is an unofficial community test build. It is not affiliated with or endorsed by Electronic Arts, Westwood Studios, Apple, or the upstream OpenTS project.

The app contains the OpenTS engine only. **It does not include Tiberian Sun or Firestorm game data.** You must provide your own lawfully obtained compatible game data from a Tiberian Sun / Firestorm installation you own.

## Quick start

1. Get data from the [CnCNet Tiberian Sun how-to](https://cncnet.org/tiberian-sun/how-to-play), which links to the
   [C&C Communications Centre installer](https://cnc-comm.com/tiberian-sun/downloads/the-game/installer).
   Both pages describe Tiberian Sun / Firestorm as freeware released by
   Electronic Arts in 2010. If you use the community installer, include the
   campaign, music and movie components for the complete iOS experience.
   Lawful retail alternatives are the [Steam](https://store.steampowered.com/bundle/39394/)
   or [EA App](https://www.ea.com/games/command-and-conquer/command-and-conquer-the-ultimate-collection)
   Command & Conquer The Ultimate Collection.
2. From the [Public Test 055 release](https://github.com/Jamesadsy/OpenTS/releases/tag/cnc-ts-ios-preview-055), download `OpenTS-unsigned.ipa` and `CnC-TS-iOS-Quickstart.zip`.
3. Extract the Quickstart ZIP and run `Prepare-CnC-TS-iOS.cmd`. Paste or drag
   the installed Tiberian Sun / Firestorm folder and the downloaded IPA into
   its prompts. Press Enter to create
   `Documents\CnC-TS-iOS-Ready\OpenTS`.
4. Sign and sideload the unchanged IPA with [Sideloadly](https://sideloadly.io/)
   or [SideStore](https://docs.sidestore.io/docs/installation/install). Follow
   the sideloading tool's current setup instructions.
5. Launch CnC TS once and close it. In Files, copy the generated `OpenTS` folder into the CnC TS app's Documents area, then launch the game.

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

After installation, launch **CnC TS** once and then exit.

The app creates its writable Files-visible structure beneath its Documents container.

Because the app enables iOS File Sharing, you should be able to reach it through the Files app as the CnC TS app folder.

## 5. Copy the prepared game data

Using the Files app, Finder/Apple Devices file sharing, iCloud Drive, or another normal local file-transfer method, place the prepared **OpenTS** folder into the CnC TS app's Documents area.

The final paths must be:

~~~text
CnC TS / OpenTS / Data
CnC TS / OpenTS / User
~~~

If **OpenTS/User** already contains saves or settings you want to keep, do not overwrite that folder. Copy/update **Data** separately.

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

## Saves and reinstalls

Player state lives in:

~~~text
Documents/OpenTS/User
~~~

Back up that folder before deleting/reinstalling the app if you care about your saves or settings. iOS can remove an app's data container when the app is deleted.

Do not upload original MIX archives, movies, audio files or other proprietary Tiberian Sun data when reporting a bug.

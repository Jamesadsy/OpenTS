# CnC TS for iPhone/iPad — Public Test 053

CnC TS is the experimental iPhone/iPad build carried by this OpenTS fork.

It is an unofficial community test build. It is not affiliated with or endorsed by Electronic Arts, Westwood Studios, Apple, or the upstream OpenTS project.

The app contains the OpenTS engine only. **It does not include Tiberian Sun or Firestorm game data.** You must provide your own lawfully obtained compatible game data from a Tiberian Sun / Firestorm installation you own.

## Supported public-test target

- iPhone or iPad
- arm64
- iOS/iPadOS 16.0 or later
- landscape orientation
- unsigned IPA, signed by the tester during sideloading

Public Test 053 is built from:

- source commit: **9ad5cd0edd0072e08cc1ba45bcb79c6c2dff0a2c**
- source tree: **96a490760e411a74c02e7ae21ff8091fccc28f75**
- IPA filename: **OpenTS-unsigned.ipa**
- expected size: **8,512,047 bytes**
- SHA-256: **ef264ba977dc70586aa622b89a40528096b600a17732685e75a032d7aab6ff2e**

Download it from the [Public Test 053 release](https://github.com/Jamesadsy/OpenTS/releases/tag/cnc-ts-ios-preview-053).

## 1. Verify the IPA

On Windows PowerShell:

~~~powershell
Get-FileHash .\OpenTS-unsigned.ipa -Algorithm SHA256
~~~

The result must be:

~~~text
EF264BA977DC70586AA622B89A40528096B600A17732685E75A032D7AAB6FF2E
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

### Recommended: preparation script

On Windows, clone or download this repository and run:

~~~powershell
powershell -ExecutionPolicy Bypass -File .\scripts\public\prepare-ios-data.ps1 -SourceDir "C:\Path\To\Your\Tiberian Sun" -Ipa "C:\Path\To\OpenTS-unsigned.ipa" -OutputDir "C:\Temp\CnC-TS-iOS"
~~~

The script:

- reads only your local Tiberian Sun installation;
- verifies the Public Test 053 IPA size and SHA-256;
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

If the game is silent, first check the iPhone/iPad Ring/Silent setting. Public Test 053 intentionally follows the iOS silent switch. Connected headphones should be selected by normal iOS audio routing.

See [IOS_TESTING.md](IOS_TESTING.md) for controls, known issues and what to include in a bug report.

## Saves and reinstalls

Player state lives in:

~~~text
Documents/OpenTS/User
~~~

Back up that folder before deleting/reinstalling the app if you care about your saves or settings. iOS can remove an app's data container when the app is deleted.

Do not upload original MIX archives, movies, audio files or other proprietary Tiberian Sun data when reporting a bug.

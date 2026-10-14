<h1 align="center">NanoSync</h1>

<p align="center">
  Put MP3s on your iPod nano 7 straight from the device. No computer, no iTunes.
</p>

<p align="center">
  <img alt="License: GPL-3.0" src="https://img.shields.io/badge/license-GPL--3.0-blue">
  <img alt="Device: iPod nano 7th gen" src="https://img.shields.io/badge/device-iPod%20nano%207G-lightgrey">
  <img alt="Language: C" src="https://img.shields.io/badge/language-C-orange">
</p>

NanoSync is a homebrew app for the iPod nano 7th generation. Copy MP3 files into a folder, open the app, tap
**Import**, and the songs show up in the stock **Music** app with their tags and cover art.

It writes the same library files the iPod itself uses, so there is nothing to sync later and no extra player to learn.

> **Requires a modified iPod.** NanoSync only runs on an iPod nano 7th generation that already runs the untethered
> homebrew firmware. A stock iPod cannot run it. See [Before you start](#before-you-start).

## Features

- **Imports into the native Music library.** Artist, album, title, track and disc numbers, year, genre, composer and more
  are read from the ID3 tags (v1, v2.2, v2.3, v2.4).
- **Cover art.** Embedded JPEG covers are decoded on the device and added in all the sizes the iPod uses.
- **Safe by design.** Every file it changes is copied to a `.bak` first. If an import fails, or power is lost halfway,
  the library is restored automatically the next time the app starts.
- **Verified writes.** Copies and database files are read back and checked before the source files are deleted.
- **Dry run first.** The first run only checks everything and writes nothing.
- **Small.** About 450 KB, integer-only code, no floating point.

## Before you start

NanoSync is a third-party app. It needs the iPod to be modified first with the **untethered homebrew firmware**, which
adds the loader that starts apps like this one. That setup is a separate project and is not part of NanoSync:

1. Go to **[olievans123/nano7-untethered](https://github.com/olievans123/nano7-untethered)** and follow the installation
   instructions in its README to modify the iPod. Read all of it first, including its warnings.
2. Once the iPod boots with that firmware and can start apps, come back here to build and install NanoSync.

Modifying an iPod can render it unusable and voids any warranty. Back up the iPod before you begin, and do not use an
iPod you cannot afford to lose.

## Quick start

1. [Modify the iPod](#before-you-start) with the untethered firmware, then [build and install](#build) NanoSync once (needs a Linux PC).
2. Plug the iPod in and copy your `.mp3` files into `/Apps/Data/Importar`.
3. Open **NanoSync** on the iPod.
4. The first run is a dry run. Delete `/Apps/Data/Importar/dryrun.txt`, open the app again and tap **Import**.
5. When it says *Import complete*, restart the iPod. Your songs are in **Music**.

## Requirements

| What | Details |
|---|---|
| iPod | nano 7th generation with the untethered homebrew firmware from [olievans123/nano7-untethered](https://github.com/olievans123/nano7-untethered) |
| Linux PC | to build and install: `arm-none-eabi-gcc`, `make`, `python3`, `mtools` (`mcopy`, `mdir`, `mdel`), optionally `udevadm` |
| SDK | the NanoApps SDK used by that firmware project (`nanoapps/` in nano7-untethered, or [nfzerox/NanoApps](https://github.com/nfzerox/NanoApps)). It is not copied here; run its `./start build` once so it fetches LVGL and the other dependencies |

> **Use at your own risk.** NanoSync rewrites the iPod's music database. It keeps a backup of every file it changes and
> can undo a failed import, but make your own backup of the iPod first. See [Risks](#risks).

## Build

The library signature (HashAB) is keyed with the iPod's FireWire GUID, which is the 16 hex digit USB serial number of
the device. The build embeds it, so a build works only with the iPod it was built for.

```sh
# 1. Find the serial number (16 hex digits) of the attached iPod
udevadm info --query=property --name /dev/disk/by-id/usb-Apple_iPod_<SERIAL>-0:0 | grep ID_SERIAL_SHORT

# 2. Build
tools/build.sh --sdk /path/to/nanoapps --guid <16 hex digits> --out build
# -> build/nanosync.hbapp
```

`tools/build.sh` copies `src/` and `core/` into `<sdk>/apps/nanosync`, runs the SDK's `make`, and checks the resulting
ELF (`CHECK_ARM_OK`).

## Install

The SDK packs every app of a slot into one `AllApps.pack`, so the pack must be built with **all** the apps you want to
keep, not only NanoSync. The installer does not merge into an existing pack; it writes the pack you give it.

1. Build the apps you use in the SDK, then build the pack:

   ```sh
   cd /path/to/nanoapps
   python3 tools/build_apps.py blocks gameboy probe nanosync   # list every app you want, plus nanosync
   ```

   The result is `/tmp/hbapps/Apps`, containing `AllApps.pack`, `Executables/` and `Icons/`.

2. Attach the iPod in disk mode and **unmount its data partition** (do not eject the iPod).

3. Optional rehearsal on a FAT32 image, no iPod needed:

   ```sh
   tools/install-nanosync.sh --apps-dir /tmp/hbapps/Apps --image test.img
   ```

4. Install:

   ```sh
   sudo tools/install-nanosync.sh --apps-dir /tmp/hbapps/Apps \
        --device /dev/disk/by-id/usb-Apple_iPod_<SERIAL>-0:0
   ```

   Optional flags: `--usb-serial <SERIAL>`, `--expected-size <bytes>`, `--slot auto|A|B`, `--backup-dir DIR`.

5. Restart the iPod and re-run the firmware project's resident installer so the new app appears.

**What the installer checks:** it must run as root, the device must be a block device with model `iPod` (and match
`--usb-serial` and `--expected-size` if you pass them), no partition of the disk may be mounted, and the maintenance lock
must be free. It detects the active slot, saves the files it will overwrite, writes the executable, icon and pack, reads
each file back and compares SHA-256, and restores the previous files on any failure. It never touches the firmware, the
loaders, `/Apps/Data` or `/iPod_Control`.

**What it does not do:** it does not build or merge packs, and it does not check that the build's GUID matches the iPod
(a mismatch makes the app refuse to import with "invalid HashAB signature"). Slot A has not been tested on a device.

**Update** (replaces only `Nanosync.hbapp`, plus the icon and pack if they differ):

```sh
sudo tools/update-nanosync.sh --apps-dir DIR --device DEV
```

**Remove** (needs a pack built without NanoSync, or the `before-nanosync` backup the installer made):

```sh
sudo tools/remove-nanosync.sh --pack PACK --device DEV
```

Imported songs stay in the library.

## Using the app

1. Copy MP3 files into `/Apps/Data/Importar` (up to 20 per run).
2. Open **NanoSync**. The list shows the files and their tags. Battery must be at least 40%.
3. The first run is a dry run (it creates `dryrun.txt`). Delete that file and run again to import for real.
4. Tap **Import**. When it finishes, restart the iPod.

Other buttons: **Reload** rescans the folder, **Free RAM** releases the working memory (also released after 5 minutes
idle). To import without cover art, create an empty `/Apps/Data/Importar/noart.txt`.

## How it works

```
 MP3 files in /Apps/Data/Importar
            |
            v
  read ID3 tags + MPEG header, decode cover
            |
            v
  back up every file it will change (.bak, hash checked)  -->  journal
            |
            v
  copy MP3s  -->  write library databases  -->  write artwork
            |
            v
  read back and verify  -->  delete journal (commit)  -->  delete sources
```

The library files it writes:

- `iPod_Control/Music/Fxx/ABCD.mp3`: the copied songs (random folder `F00` to `F49`, random 4 letter name).
- `iPod_Control/iTunes/iTunesCDB` (signed with HashAB) and `iTunes Library.itlp/{Library,Locations,Dynamic}.itdb`
  with `Locations.itdb.cbk`.
- `iPod_Control/Artwork/ArtworkDB` and `F1010_1.ithmb` (240x240), `F1013_1.ithmb` (50x50), `F1015_1.ithmb` (58x58),
  `F1016_1.ithmb` (57x57), RGB565.
- In `/Apps/Data/Importar`: `log.txt` (what happened, with error codes), plus `journal.txt` and `done.txt` during an
  import, and the marker files `dryrun.txt`, `ns_init.txt`, `noart.txt`.

`Extras.itdb`, `Genius.itdb`, `Play Counts` and the songs that are already there are not modified. The format notes are
in [`docs/library-format.md`](docs/library-format.md).

## Undo

- **Failed or interrupted import:** automatic. At the next start the app finds `journal.txt` and restores every file
  from its `.bak`, verifying hashes. The MP3 files you queued are still in the folder, and `log.txt` says what happened.
- **A finished import:** the `.bak` files still hold the previous library until the next import. In disk mode with the
  partition unmounted, copy them back over the originals (`iTunesCDB.bak` to `iTunesCDB`, the `.itdb.bak` and
  `.cbk.bak` files to the names without `.bak` in `iTunes Library.itlp`, the `Artwork` `.bak` files likewise) and delete
  the new songs from `iPod_Control/Music`. Do this before importing again.
- **Best option:** your own backup of the iPod's data partition made before the first import.

## Known limits

- **MP3 only** (files ending in `.mp3`). No AAC, FLAC, podcasts, audiobooks or video.
- **Covers:** baseline JPEG, and progressive JPEG when the picture is at least about 800 px. A PNG or damaged cover is
  skipped with a warning and the song is imported without a cover. Up to 3 to 8 distinct covers per import; the same
  cover in one batch is stored once.
- **ID3 tags up to 640 KB.** A file with a larger tag is refused.
- **Library size:** the whole library is regenerated in RAM with a working block of at most 2 MiB. In the PC simulator,
  a library grew to 61 tracks and the next 20 were refused; expect a limit of roughly 70 to 80 tracks in total. The app
  refuses cleanly and changes nothing.
- 20 files per run. Lyrics and the compilation flag are ignored with a warning in `log.txt`, and so are sort-order,
  podcast and TV tags.
- Titles and artists are shown ASCII-only in NanoSync's own list; the library itself keeps the full UTF-8 text.
- Duplicates are not detected: importing the same file twice adds the song twice.

## FAQ

**Do I need iTunes or a computer to import songs?**
You need a Linux PC once, to build and install the app. After that, importing needs only the iPod and a way to put files
in the folder.

**Will it delete my existing music?**
No. Existing songs are not modified, and only the files listed above are touched.

**The app says "invalid HashAB signature".**
The build was made with a different GUID than the iPod it runs on. Rebuild with the serial number of that iPod.

**Can I use it on another iPod?**
Rebuild with that iPod's serial number. Only the nano 7th generation with the untethered firmware is supported.

## Risks

- This writes the iPod's library database. The signature is checked before and after, and a library whose signature
  does not verify is never modified, but the stock firmware's behavior with databases it did not write itself cannot be
  guaranteed. **Back up the iPod before use.** Do not connect a modified iPod to iTunes or restore it with Apple software
  unless you know what you are doing.
- Do not remove power or the cable during an import. The recovery above handles it, but it is a last resort.
- The PC test suite runs the same engine code against a simulated file system with injected failures and crashes. It is
  not a substitute for testing on a device.
- No warranty (see `LICENSE`).

## Tests

```sh
tests/run_tests.sh       # engine, library, rollback, crash recovery (needs gcc, python3)
tests/test_scripts.sh    # installer, updater and remover on a FAT32 image (needs mtools, dosfstools)
```

`tests/README.md` lists what is covered.

## Project layout

```
src/     the app: ns_engine.c (import engine), ns_art.c (covers), ns_hb.c (device layer), nanosync.c (UI)
core/    portable code: HashAB, CDB writer, SQLite writer, tag reader, deflate, TJpgDec
tools/   build, install, update and remove scripts, ARM sanity check
tests/   PC simulator, fixtures generator, test scripts
docs/    library format notes
```

## Acknowledgments

- [iOpenPod](https://github.com/TheRealSavi/iOpenPod) (GPL-3.0): its handling of the iTunesCDB, ArtworkDB and SQLite
  library formats was the reference used to build and verify NanoSync's database writer.
- [dstaley/hashab](https://github.com/dstaley/hashab): HashAB signature implementation.
- [olievans123/nano7-untethered](https://github.com/olievans123/nano7-untethered) and
  [nfzerox/NanoApps](https://github.com/nfzerox/NanoApps): the untethered firmware, loader and app SDK.
- ChaN's TJpgDec: JPEG decoder used for cover art.

## License

Copyright (C) 2026 tenspav. NanoSync is free software released under the GNU General Public License, version 3
(see `LICENSE`). Third-party code and its licenses: `THIRD_PARTY.md`.

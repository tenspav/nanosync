# NanoSync

NanoSync is a homebrew app for the iPod nano 7th generation. It imports MP3 files into the **native
Music library** of the iPod, on the device itself, with no computer attached: copy MP3 files into a folder,
open the app, tap Import. The songs then show up in the stock Music app, with cover art.

It runs on the iPod nano 7G with the untethered homebrew firmware from
[olievans123/nano7-untethered](https://github.com/olievans123/nano7-untethered)
(built on [nfzerox/NanoApps](https://github.com/nfzerox/NanoApps)).

> **Use at your own risk.** NanoSync rewrites the iPod's music database. It keeps a backup of every file it
> changes and can undo a failed import, but you should back up the iPod first (see "Risks").

## What it does

For every import it:

1. reads the ID3 tags (v2.2, v2.3, v2.4 and v1) and the MPEG header of each MP3,
2. copies the file to `iPod_Control/Music/Fxx/ABCD.mp3` and verifies the copy (SHA-1),
3. adds the tracks to the library: `iTunesCDB` (signed with HashAB), `Library.itdb`, `Locations.itdb` (+ `.cbk`),
   `Dynamic.itdb` (SQLite files written by the app itself),
4. decodes the embedded cover (JPEG), scales it to the four sizes the nano uses and appends it to `ArtworkDB` and the
   `.ithmb` files,
5. reads everything back, compares hashes, and only then deletes the source files.

Safety order: `.bak` copy of every file it will change (hash checked) -> journal -> copies -> database (the
`iTunesCDB` last) -> read back and verify -> delete the journal (commit) -> delete the sources. A failure, or power loss,
after the journal is written restores the library from the `.bak` files the next time the app starts.

## What you need

- iPod nano 7th generation running the untethered homebrew firmware (see the link above), with the A/B loader
  installed. Developed and tested on the author's iPod; other units and firmware versions are untested.
- A Linux PC to build and install: `arm-none-eabi-gcc` (GNU Arm Embedded toolchain), `make`, `python3`, `mtools`
  (`mcopy`, `mdir`, `mdel`), `udevadm` (optional, to read the serial number).
- The NanoApps SDK tree used by the firmware project (`nanoapps/` in nano7-untethered, or upstream NanoApps). It is not
  copied into this repository (see `THIRD_PARTY.md`). Run the SDK's `./start build` once on any app so it fetches LVGL and the
  other build dependencies.

## Build

NanoSync is signed with the iPod's own identifier: the HashAB signature of the library is keyed with the device's
FireWire GUID, which is the 16 hex digit USB serial number of the iPod. The build embeds it, so a build works only
with the iPod it was built for.

```sh
# serial number (16 hex digits) of the attached iPod
udevadm info --query=property --name /dev/disk/by-id/usb-Apple_iPod_<SERIAL>-0:0 | grep ID_SERIAL_SHORT

tools/build.sh --sdk /path/to/nanoapps --guid <16 hex digits> --out build
# -> build/nanosync.hbapp  (and a sanity check of the ELF: CHECK_ARM_OK)
```

`tools/build.sh` copies `src/` and `core/` to `<sdk>/apps/nanosync` (replacing any older copy) and runs the SDK's `make`.

## Install

The SDK packs all apps of a slot into one `AllApps.pack`, so the pack must be built with **every** app you want to keep,
not only NanoSync. NanoSync's installer does not merge into an existing pack; it writes the pack you give it.

1. Build the other apps you use in the SDK (`apps/<name>/build/<name>.hbapp` must exist), then build the pack:
   ```sh
   cd /path/to/nanoapps
   python3 tools/build_apps.py blocks gameboy probe nanosync     # list every app you want, plus nanosync
   ```
   The result is the directory `/tmp/hbapps/Apps` containing `AllApps.pack`, `Executables/` and `Icons/`.
2. Attach the iPod in disk mode and **unmount its data partition** (do not eject the iPod).
3. Rehearse on an image (no iPod needed): `tools/install-nanosync.sh --apps-dir /tmp/hbapps/Apps --image test.img`
   with a FAT32 image that has `/Apps` (the rehearsal is what `tests/test_scripts.sh` does).
4. Install:
   ```sh
   sudo tools/install-nanosync.sh --apps-dir /tmp/hbapps/Apps \
        --device /dev/disk/by-id/usb-Apple_iPod_<SERIAL>-0:0 \
        [--usb-serial <SERIAL>] [--expected-size <disk size in bytes>] [--backup-dir DIR]
   ```
5. Restart the iPod and re-run the firmware project's resident installer so the new app shows up.

What `install-nanosync.sh` does: refuses unless it is root, the device is a block device with model `iPod` (and, if you pass
them, the USB serial and the disk size match), no partition of the disk is mounted, and the maintenance lock is free. It
detects the active slot (`/Apps/Startup/resident-B.active` present = slot B, pack `/Apps/AllApps-B.pack`; otherwise slot A,
`/Apps/AllApps.pack`; override with `--slot`). It saves the files it will overwrite to the backup directory, writes
`Executables/*.hbapp`, `Icons/*.bin` and finally the pack, reading every file back and comparing SHA-256; on any failure
it restores the previous files. It never touches the firmware, the loaders, the marker, `/Apps/Data` or `/iPod_Control`.

What it does **not** do: it does not build or merge packs, it does not check that the build's GUID matches the iPod
(a mismatch makes the app refuse to import: "invalid HashAB signature"), and slot A has not been tested on a device.

Update (replaces only `Nanosync.hbapp`, plus the icon/pack if they differ):
`sudo tools/update-nanosync.sh --apps-dir DIR --device DEV`.
Remove (needs a pack built without nanosync, or the `before-nanosync` backup the installer made):
`sudo tools/remove-nanosync.sh --pack PACK --device DEV`. Imported songs stay in the library.

## Use

1. Copy MP3 files into `/Apps/Data/Importar` on the iPod (disk mode, or any method that writes to the data partition).
   Up to 20 files per run.
2. Open **NanoSync** on the iPod. The list shows the files and their tags. Battery must be at least 40%.
3. **The first run is a dry run**: it reads and checks everything and writes nothing (it creates `dryrun.txt`). Delete
   `/Apps/Data/Importar/dryrun.txt` and run again to import for real.
4. Tap **Import**. When it says "Import complete", restart the iPod; the songs appear in Music.

Other buttons: **Reload** rescans the folder, **Free RAM** releases the memory block (it is also released after 5 minutes
idle). To import without cover art create an empty `/Apps/Data/Importar/noart.txt`.

## Files it writes

- `iPod_Control/Music/Fxx/ABCD.mp3`: the copied songs (random folder `F00`..`F49`, random 4 letter name).
- `iPod_Control/iTunes/iTunesCDB` and `iPod_Control/iTunes/iTunes Library.itlp/{Library,Locations,Dynamic}.itdb`,
  `Locations.itdb.cbk`.
- `iPod_Control/Artwork/ArtworkDB`, `F1010_1.ithmb` (240x240), `F1013_1.ithmb` (50x50), `F1015_1.ithmb` (58x58),
  `F1016_1.ithmb` (57x57), RGB565.
- Backups next to them while an import runs: `iTunesCDB.bak`, `Library.itdb.bak`, ... in `iPod_Control/iTunes`, and
  `ArtworkDB.bak`, `F10xx_1.ithmb.bak` in `iPod_Control/Artwork`. They are overwritten by the next import, so they always
  hold the state before the last import.
- In `/Apps/Data/Importar`: `log.txt` (what happened, with error codes), `journal.txt` and `done.txt` (only during an
  import), `dryrun.txt`, `ns_init.txt`, `noart.txt` (markers).

`Extras.itdb`, `Genius.itdb`, `Play Counts` and the existing songs are not modified.

## Undo / rollback

- Failed or interrupted import: automatic. At the next start the app finds `journal.txt` and restores every file from
  its `.bak`, verifying hashes; the MP3 files you queued are still in the folder. `log.txt` says what happened.
- Undo a finished import: after the app says "Import complete", the `.bak` files still hold the previous library. Copy
  them back over the originals (disk mode, partition unmounted; the `.bak` files are in `iPod_Control/iTunes`): `iTunesCDB.bak` -> `iTunesCDB`,
  `Library.itdb.bak`/`Locations.itdb.bak`/`Dynamic.itdb.bak`/`Locations.itdb.cbk.bak` -> the files without `.bak` in
  `iTunes Library.itlp`, the `Artwork` `.bak` files likewise, and delete the new songs from `iPod_Control/Music`.
  Do this before importing again, because the next import replaces the `.bak` files.
- The strongest undo is your own backup of the iPod's data partition made before the first import.

## Known limits

- **MP3 only** (files ending in `.mp3`). No AAC, FLAC, podcasts, audiobooks or video.
- **Cover art: JPEG only.** Baseline JPEG is supported; progressive JPEG is supported when the picture is at
  least about 800 px. A PNG or damaged cover is skipped with a warning and the song is imported without a cover.
  Up to 3 to 8 distinct covers per import (memory), the same cover in one batch is stored once.
- **ID3 tags up to 640 KB**; a file with a larger tag (for example a huge embedded picture) is refused.
- **Library size**: the whole library is regenerated in RAM, with a working block of at most 2 MiB. In the PC
  simulator with a 1.9 MiB block, a library grew to 61 tracks and the next 20 were refused ("Not enough memory"); expect a
  limit of roughly 70 to 80 tracks in total. The app refuses cleanly and changes nothing.
- 20 files per run. Lyrics (`Extras.itdb`) and the compilation flag are ignored with a warning in `log.txt`;
  sort-order, podcast and TV tags are ignored too.
- Titles and artists are shown ASCII-only in NanoSync's own list (the app font); the library itself keeps the full UTF-8 text.
- Duplicates are not detected: importing the same file twice adds the song twice.

## Risks

- This writes the iPod's library database. The database signature (HashAB) is checked before and after, and a
  library whose signature does not verify is never modified, but the stock firmware's behavior with databases it did not
  write itself cannot be guaranteed. **Back up the iPod before use**; never connect it to iTunes or restore it with
  Apple software while it carries modifications unless you know what you are doing.
- Do not remove power or the cable during an import. The recovery above handles it, but it is a last resort.
- The PC test suite exercises the same engine code against a simulated file system, with injected failures and
  crashes; it is not a substitute for testing on a device, and it was not run against firmware other than the author's.
- No warranty (see `LICENSE`).

## Tests

```sh
tests/run_tests.sh          # engine, library, rollback, crash recovery (needs gcc, python3)
tests/test_scripts.sh       # installer / updater / remover on a FAT32 image (needs mtools, dosfstools)
```

`tests/README.md` lists what is covered.

## Repository layout

```
src/        the app: ns_engine.c (import engine, runs on the device and in the simulator), ns_art.c (covers),
            ns_hb.c (device platform layer), nanosync.c (LVGL screen), Makefile, Info.plist
core/       portable code: HashAB, CDB writer, SQLite writer, tag reader, deflate, TJpgDec
tools/      build.sh, install/update/remove scripts, ARM ELF sanity check
tests/      PC simulator host, fixtures generator, test scripts
docs/       library format notes
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

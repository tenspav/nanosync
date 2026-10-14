# iPod nano 7G library: what NanoSync reads and writes

A short summary of the on-device layout, as far as NanoSync needs it. Offsets are little endian unless noted. This is a
description of observed file formats, not an official specification.

## Files

| Path (on the data partition) | Content |
|---|---|
| `iPod_Control/Music/Fxx/ABCD.mp3` | song files, `xx` = 00..49, 4 upper case letters |
| `iPod_Control/iTunes/iTunesCDB` | the track database: an `mhbd` header followed by a zlib stream of the body (`mhsd` sections: tracks, playlists, albums, artists ...) |
| `iPod_Control/iTunes/iTunes Library.itlp/Library.itdb` | SQLite: items, albums, artists, containers, genres |
| `iPod_Control/iTunes/iTunes Library.itlp/Locations.itdb` | SQLite: file locations of the items |
| `iPod_Control/iTunes/iTunes Library.itlp/Dynamic.itdb` | SQLite: play counts and dates |
| `iPod_Control/iTunes/iTunes Library.itlp/Locations.itdb.cbk` | signature of `Locations.itdb` |
| `iPod_Control/Artwork/ArtworkDB` | cover index (`mhfd` / `mhsd` / `mhli` / `mhii` / `mhod` / `mhni`) |
| `iPod_Control/Artwork/F1010_1.ithmb` ... | raw RGB565 images, one file per format, images appended |

NanoSync regenerates the three `.itdb` files completely from the new CDB (`core/mdb_sqlgen.c`) and writes them with its own
minimal SQLite file writer (`core/mdb_sqlite.c`, 4096-byte pages, UTF-8, B-trees with interior pages when needed).

## Signatures (HashAB)

The firmware refuses a library whose signature does not verify.

- `iTunesCDB`: `mhbd` header length at `+4`. The signature is 57 bytes at `0xAB`. Input: SHA-1 of the physical file with
  these fields zeroed: `0x18` (8 bytes, db id), `0x32` (20), `0x58` (20), `0x72` (46), `0xAB` (57), and the 2-byte scheme at
  `0x30` set to 4. The signer then stores the scheme 3. The HashAB function takes that SHA-1 and the device's 8-byte GUID
  (the USB serial number `0123456789ABCDEF` as 8 bytes). It is computed over the file as it is on disk: if a file is padded
  with zeros, it must be re-signed.
- `Locations.itdb.cbk`: 57 bytes of HashAB, then the 20 byte SHA-1 of the block hashes, then one 20 byte SHA-1 per 1024
  byte block of `Locations.itdb`; the HashAB input is the SHA-1 over the block hashes. Total size `57 + 20 + 20 * blocks`.
  Computed over the physical file.

## CDB compression

Header field `0x0C` is 2 and byte `0xA8` is 1 when the body is compressed. The body is a zlib stream. NanoSync reads
stored, fixed and dynamic Huffman blocks (`core/mdb_zlib.c`) and writes a single fixed-Huffman block with a short LZ77 search
(`core/mdb_deflate.c`).

## Track record

A new track adds an `mhit` (624 byte template, `core/mdb_tmpl.h`) with the string records (`mhod`, UTF-16LE: title, artist,
album, genre, composer, location `:iPod_Control:Music:Fxx:ABCD.mp3`, file type ...), entries in the master playlist,
and the album/artist lists and sort indexes (`core/mdb_cdb.c`). Cover art sets `mhit` fields `0x7C` (image count),
`0x80` (size of the original, width*height*3), `0xA4` (has artwork) and `0x160` (image id).

## Artwork

| Format id | Size | File |
|---|---|---|
| 1010 | 240x240 | `F1010_1.ithmb` |
| 1013 | 50x50 | `F1013_1.ithmb` |
| 1015 | 58x58 | `F1015_1.ithmb` |
| 1016 | 57x57 | `F1016_1.ithmb` |

Pixels are RGB565 little endian, rows padded to the stride of the format (480, 100, 116, 116 bytes). Image ids start at
100. New images are appended to the `.ithmb` files at multiples of the frame size and referenced from `ArtworkDB`. The
`ArtworkDB` has no hash or signature.

## What the engine guarantees

- Order of operations: backup (`.bak`, hash checked) -> journal (hash) -> copy songs (SHA-1 verified) -> write database files,
  `iTunesCDB` last -> read back and compare hashes -> delete the journal (commit) -> delete the sources.
- The OS on the device cannot rename, truncate-in-place or append: files are rewritten whole, and a database file is never
  shrunk (a shorter new file is padded with zero pages that SQLite ignores; CDB and cbk are re-signed over the padded file).
- On start, a journal that is present means an interrupted run: every file is restored from its `.bak` and verified.

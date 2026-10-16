#!/usr/bin/env bash
# NanoSync v0.0.0 - developed by tenspav
set -u
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(dirname "$HERE")
QUICK=${1:-}
GUID=${NS_SIM_GUID:-0123456789ABCDEF}
B=$REPO/build
"$HERE/build_sim.sh" "$B" || exit 2
SIM=$B/ns_sim; FIX=$B/nsfix; PY=python3
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
export NS_DET=1
PASS=0; FAILN=0
ok()    { PASS=$((PASS+1)); echo "[PASS] $*"; }
bad()   { FAILN=$((FAILN+1)); echo "[FAIL] $*"; }
check() { if eval "$2"; then ok "$1"; else bad "$1   (condition: $2)"; fi; }
D=$W/data; mkdir -p "$D"
M() { "$PY" "$HERE/mkmp3.py" "$@"; }
M "$D/a_baseline.mp3"    --title "Song One"   --artist "Artist A" --album "Album X" --genre "Rock" --year 2001 --track 1 --cover baseline
M "$D/b_progressive.mp3" --title "Song Two"   --artist "Artist B" --album "Album Y" --genre "Pop"  --year 2002 --track 2 --cover progressive
M "$D/c_nocover.mp3"     --title "Song Three" --artist "Artist A" --album "Album X" --track 3 --frames 800
M "$D/d_png.mp3"         --title "Song Four"  --artist "Artist C" --cover png
M "$D/e_bigtag.mp3"      --title "Big Tag"    --artist "Artist D" --junk-tag-kb 700
head -c 5000 /dev/urandom > "$D/f_garbage.mp3"
echo "notes" > "$D/readme.txt"
mk() {
  local r=$1; shift
  rm -rf "$r"; mkdir -p "$r/Apps/Data/Importar" "$r/iPod_Control/iTunes" "$r/iPod_Control/Music"
  local i; for i in $(seq -w 0 49); do mkdir "$r/iPod_Control/Music/F$i"; done
  "$FIX" make "$GUID" "$r/iPod_Control/iTunes" >/dev/null
  M "$r/iPod_Control/Music/F18/WTUZ.mp3" --title Fixture --artist "Test Artist" --frames 100
  echo 1 > "$r/Apps/Data/Importar/ns_init.txt"
  local f; for f in "$@"; do cp "$f" "$r/Apps/Data/Importar/"; done
}
thash() { (cd "$1" && { find . -type f ! -name log.txt ! -name '*.bak' -print0 | LC_ALL=C sort -z | xargs -0 sha256sum; find . -type d | LC_ALL=C sort; }) | sha256sum | cut -c1-16; }
LOG() { echo "$1/Apps/Data/Importar/log.txt"; }
IT() { echo "$1/iPod_Control/iTunes"; }
run() { local r=$1; shift; "$SIM" "$r" --auto-import "$@"; }
state_ok() { "$FIX" verify "$GUID" "$(IT "$1")" > "$W/v.out" 2>&1 && grep -q "^TRACKS $2\$" "$W/v.out" && "$PY" "$HERE/sqlcheck.py" "$(IT "$1")" "$2" > "$W/s.out" 2>&1; }
state_either() { state_ok "$1" 1 || state_ok "$1" "$2"; }
echo "### T1 happy path: 2 covers + 1 without cover; a .txt, a garbage MP3 and a PNG cover in the folder"
mk "$W/t1" "$D/a_baseline.mp3" "$D/b_progressive.mp3" "$D/c_nocover.mp3" "$D/d_png.mp3" "$D/f_garbage.mp3" "$D/readme.txt"
cp "$(IT "$W/t1")/iTunesCDB" "$W/t1.cdb.orig"
run "$W/t1" > "$W/t1.out" 2>&1
check "T1 import finishes: RESULT 1 (ok)" "grep -q '^RESULT 1' $W/t1.out"
check "T1 library has 1 + 4 tracks; HashAB, cbk and the three SQLite databases are valid" "state_ok $W/t1 5"
copies_match() { local f; for f in "$@"; do sha256sum "$W"/t1/iPod_Control/Music/F*/????.mp3 | grep -q "$(sha256sum < "$f" | cut -c1-64)" || return 1; done; }
check "T1 4 MP3 copies on the iPod, byte-identical to the sources" "[ \$(ls $W/t1/iPod_Control/Music/F*/????.mp3 | grep -vc WTUZ) = 4 ] && copies_match $D/a_baseline.mp3 $D/b_progressive.mp3 $D/c_nocover.mp3 $D/d_png.mp3"
check "T1 imported sources removed; the .txt and the garbage MP3 stay" "[ ! -e $W/t1/Apps/Data/Importar/a_baseline.mp3 ] && [ -e $W/t1/Apps/Data/Importar/readme.txt ] && [ -e $W/t1/Apps/Data/Importar/f_garbage.mp3 ]"
check "T1 two distinct covers decoded; ArtworkDB and 4 .ithmb files written (2 images each)" "[ -s $W/t1/iPod_Control/Artwork/ArtworkDB ] && [ \$(stat -c%s $W/t1/iPod_Control/Artwork/F1010_1.ithmb) = 230400 ] && [ \$(grep -c 'COVER decoded' $(LOG $W/t1)) = 2 ]"
check "T1 PNG cover: warning in the log, track still imported" "grep -q 'unsupported cover' $(LOG $W/t1)"
check "T1 backup .bak of the library files equals the original" "cmp -s $W/t1.cdb.orig $(IT $W/t1)/iTunesCDB.bak"
check "T1 journal removed after commit" "[ ! -e $W/t1/Apps/Data/Importar/journal.txt ]"
check "T1 the new CDB differs from the original" "! cmp -s $W/t1.cdb.orig $(IT $W/t1)/iTunesCDB"
echo "### T2 dry run (dryrun.txt present): reads everything, writes nothing"
mk "$W/t2" "$D/a_baseline.mp3" "$D/c_nocover.mp3"; echo x > "$W/t2/Apps/Data/Importar/dryrun.txt"
H0=$(thash "$W/t2"); run "$W/t2" > "$W/t2.out" 2>&1
check "T2 RESULT 2 (dry run ok)" "grep -q '^RESULT 2' $W/t2.out"
check "T2 nothing changed on the iPod" "[ $H0 = \$(thash $W/t2) ] && [ ! -e $(IT $W/t2)/iTunesCDB.bak ]"
echo "### T3 first run: dry run is switched on by default"
mk "$W/t3" "$D/c_nocover.mp3"; rm "$W/t3/Apps/Data/Importar/ns_init.txt"
run "$W/t3" > "$W/t3.out" 2>&1
check "T3 RESULT 2 and dryrun.txt created; library untouched" "grep -q '^RESULT 2' $W/t3.out && [ -e $W/t3/Apps/Data/Importar/dryrun.txt ] && state_ok $W/t3 1"
echo "### T4 noart.txt: covers are not written"
mk "$W/t4" "$D/a_baseline.mp3"; touch "$W/t4/Apps/Data/Importar/noart.txt"
run "$W/t4" > "$W/t4.out" 2>&1
check "T4 RESULT 1, 2 tracks, no Artwork folder" "grep -q '^RESULT 1' $W/t4.out && state_ok $W/t4 2 && [ ! -e $W/t4/iPod_Control/Artwork ]"
echo "### T5 invalid library signature: import blocked, nothing written"
mk "$W/t5" "$D/c_nocover.mp3"; printf '\377' | dd of="$(IT "$W/t5")/iTunesCDB" bs=1 seek=$((0xB0)) conv=notrunc 2>/dev/null
H0=$(thash "$W/t5"); run "$W/t5" > "$W/t5.out" 2>&1
check "T5 blocked, tree unchanged, source file still there" "grep -qi 'HASHAB' $(LOG $W/t5) && [ $H0 = \$(thash $W/t5) ] && [ -e $W/t5/Apps/Data/Importar/c_nocover.mp3 ]"
echo "### T6 low battery"
mk "$W/t6" "$D/c_nocover.mp3"; H0=$(thash "$W/t6"); run "$W/t6" --battery 10 > "$W/t6.out" 2>&1
check "T6 RESULT 4 (refused), tree unchanged" "grep -q '^RESULT 4' $W/t6.out && [ $H0 = \$(thash $W/t6) ]"
echo "### T7 oversized ID3 tag (700 KB) is refused per file, the others import"
mk "$W/t7" "$D/e_bigtag.mp3" "$D/c_nocover.mp3"; run "$W/t7" > "$W/t7.out" 2>&1
check "T7 big-tag file refused (E_TAG*), the other imported (2 tracks)" "grep -q 'E_TAG' $W/t7.out && state_ok $W/t7 2 && [ -e $W/t7/Apps/Data/Importar/e_bigtag.mp3 ]"
echo "### T8 device semantics (NS_SDK=1): sequential reads, 4 KB per call, EOF returns an error code, delete fails on open database files, no truncation"
mk "$W/t8" "$D/a_baseline.mp3" "$D/b_progressive.mp3" "$D/c_nocover.mp3"
NS_SDK=1 NS_SDK_EOFRC=1 NS_SDK_MAXGOT=4096 NS_SDK_NOTRUNC=1 NS_SDK_RMFAIL=1 run "$W/t8" > "$W/t8.out" 2>&1
check "T8 RESULT 1, library valid with 4 tracks" "grep -q '^RESULT 1' $W/t8.out && state_ok $W/t8 4"
echo "### T9 second import on the same tree (library grows again, covers appended)"
M "$W/more.mp3" --title "Song Five" --artist "Artist E" --cover baseline --frames 300
cp "$W/more.mp3" "$W/t8/Apps/Data/Importar/"; run "$W/t8" > "$W/t9.out" 2>&1
check "T9 RESULT 1, 5 tracks, valid; .ithmb files hold 3 images" "grep -q '^RESULT 1' $W/t9.out && state_ok $W/t8 5 && [ \$(stat -c%s $W/t8/iPod_Control/Artwork/F1010_1.ithmb) = 345600 ]"
if [ "$QUICK" != quick ]; then
echo "### T10 injected write errors at many points: always a valid library; on failure the original is restored"
mk "$W/t10" "$D/a_baseline.mp3" "$D/c_nocover.mp3"; run "$W/t10" --stats > "$W/t10.out" 2>&1; NOPS=$(awk '/^OPS/{print $2}' "$W/t10.out")
nbad=0; nrest=0; nok=0; n=0
for nw in $(seq 1 7 160); do
  mk "$W/t10" "$D/a_baseline.mp3" "$D/c_nocover.mp3"; H0=$(thash "$W/t10"); run "$W/t10" --failwrite $nw > "$W/t10.out" 2>&1; n=$((n+1))
  if grep -q '^RESULT 1' "$W/t10.out"; then nok=$((nok+1)); state_ok "$W/t10" 3 || nbad=$((nbad+1))
  else
    nrest=$((nrest+1))
    if ! { state_ok "$W/t10" 1 && [ ! -e "$W/t10/Apps/Data/Importar/journal.txt" ] && [ -e "$W/t10/Apps/Data/Importar/a_baseline.mp3" ] && [ $(ls "$W/t10"/iPod_Control/Music/F*/????.mp3 | grep -vc WTUZ) = 0 ]; }; then nbad=$((nbad+1)); echo "   write error $nw: BAD STATE ($(grep -e ^RESULT -e ^MSG "$W/t10.out" | tr '\n' ' '))"; fi
  fi
done
check "T10 $n injected write errors: $nrest restored to the original, $nok completed, $nbad bad states" "[ $nbad = 0 ] && [ $nrest -gt 3 ]"
echo "### T11 simulated power loss (process killed) at sampled operations, then the app is started again and recovers"
mk "$W/t11" "$D/a_baseline.mp3" "$D/c_nocover.mp3"; run "$W/t11" --stats > "$W/t11.out" 2>&1; N=$(awk '/^OPS/{print $2}' "$W/t11.out")
nbad=0; ncr=0
for k in $(seq 3 $(( N / 40 + 1 )) "$N"); do for mode in 0 1; do
  mk "$W/t11" "$D/a_baseline.mp3" "$D/c_nocover.mp3"; run "$W/t11" --crash "$k" --mode $mode > "$W/t11a.out" 2>&1; [ $? = 42 ] || continue
  "$SIM" "$W/t11" --no-import > "$W/t11b.out" 2>&1; ncr=$((ncr+1))
  if ! { state_either "$W/t11" 3 && [ ! -e "$W/t11/Apps/Data/Importar/journal.txt" ]; }; then nbad=$((nbad+1)); echo "   crash at op $k mode $mode: BAD STATE"; fi
done; done
check "T11 $ncr crash points (of $N operations x 2 modes): after recovery the library is always valid ($nbad bad)" "[ $nbad = 0 ] && [ $ncr -gt 10 ]"
fi
echo "SUMMARY: $PASS PASS, $FAILN FAIL"
exit $FAILN

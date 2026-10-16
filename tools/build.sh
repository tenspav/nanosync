#!/usr/bin/env bash
# NanoSync v0.0.0 - developed by tenspav
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(dirname "$HERE")
SDK= GUID= DEVICE= OUT=$PWD/build
usage() { echo "usage: $0 --sdk NANOAPPS_DIR (--guid HEX16 | --device DEV) [--out DIR]" >&2; exit 2; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
while [[ $# -gt 0 ]]; do
  case $1 in
    --sdk) SDK=${2:-}; shift 2;;
    --guid) GUID=${2:-}; shift 2;;
    --device) DEVICE=${2:-}; shift 2;;
    --out) OUT=${2:-}; shift 2;;
    *) usage;;
  esac
done
[[ -n $SDK ]] || usage
[[ -f $SDK/sdk/hb_app.mk ]] || die "$SDK does not look like a NanoApps checkout (sdk/hb_app.mk missing)"
if [[ -z $GUID && -n $DEVICE ]]; then
  command -v udevadm >/dev/null || die "udevadm is needed for --device; pass --guid instead"
  GUID=$(udevadm info --query=property --name "$(readlink -f -- "$DEVICE")" | sed -n 's/^ID_SERIAL_SHORT=//p' | head -n1)
fi
[[ $GUID =~ ^[0-9A-Fa-f]{16}$ ]] || die "the GUID must be 16 hex digits (got '$GUID')"
command -v arm-none-eabi-gcc >/dev/null || die "arm-none-eabi-gcc not found"
APP=$SDK/apps/nanosync
rm -rf "$APP"
mkdir -p "$APP"
cp -a "$REPO"/src/. "$APP"/
cp -a "$REPO"/core "$APP"/core
make -C "$APP" NS_GUID="$GUID" 2>&1 | tee "$APP/build.log"
mkdir -p "$OUT"
cp "$APP/build/nanosync.hbapp" "$OUT/"
[[ -f $APP/build/nanosync.reloc.elf ]] && cp "$APP/build/nanosync.reloc.elf" "$OUT/"
echo "built: $OUT/nanosync.hbapp"
if [[ -f $OUT/nanosync.reloc.elf ]]; then "$HERE/check_arm.sh" "$OUT/nanosync.reloc.elf"; fi

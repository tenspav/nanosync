#!/usr/bin/env bash
# NanoSync v0.0.0 - developed by tenspav
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
source "$HERE/nanosync-common.sh"
PACK= PACK_SHA=
usage() {
  cat >&2 <<EOF
usage: sudo $0 --pack PACK --device DEV [--usb-serial ID] [--expected-size BYTES] [--pack-sha256 HASH] [--slot auto|A|B] [--backup-dir DIR]
       $0 --pack PACK --image IMG [--pack-sha256 HASH] [--slot auto|A|B] [--backup-dir DIR]
EOF
  exit 2
}
while [[ $# -gt 0 ]]; do
  case $1 in
    --pack) PACK=${2:-}; shift 2;;
    --pack-sha256) PACK_SHA=${2:-}; shift 2;;
    --device) DEVICE=${2:-}; shift 2;;
    --image) IMAGE=${2:-}; shift 2;;
    --usb-serial) USB_SERIAL=${2:-}; shift 2;;
    --expected-size) EXPECTED_SIZE=${2:-}; shift 2;;
    --slot) SLOT=${2:-}; shift 2;;
    --backup-dir) BACKUP_DIR=${2:-}; shift 2;;
    -h|--help) usage;;
    *) usage;;
  esac
done
[[ -n $PACK && -f $PACK ]] || die "--pack must be an existing pack file that does not contain NanoSync"
[[ -z $PACK_SHA || $(hash_of "$PACK") == "$PACK_SHA" ]] || die "pack SHA-256 does not match --pack-sha256"
ns_need_tools date
ns_open_target
stamp=$(date -u +%Y%m%dT%H%M%SZ)
ns_snap "$PACK_DST"
ns_snap "/Apps/Executables/$EXE_NAME"
ns_snap "/Apps/Icons/$ICON_NAME"
if [[ -n ${HAD[$PACK_DST]} ]]; then
  cp "$tmp/old$PACK_DST" "$BACKUP_DIR/$(basename "$PACK_DST").with-nanosync.$stamp"
  echo "backup : $BACKUP_DIR/$(basename "$PACK_DST").with-nanosync.$stamp (${HAD[$PACK_DST]})"
fi
run() {
  local f
  ns_write_verify "$PACK" "$PACK_DST" || return 1
  for f in "/Apps/Executables/$EXE_NAME" "/Apps/Icons/$ICON_NAME"; do
    if ns_exists "$f"; then
      WROTE+=("$f"); mdel -i "$PART" "::$f" || return 1; echo "removed $f"
    else echo "absent  $f"; fi
  done
  for f in "/Apps/Executables/$EXE_NAME" "/Apps/Icons/$ICON_NAME"; do
    if ns_exists "$f"; then echo "still present: $f" >&2; return 1; fi
  done
}
if ! run; then ns_rollback; exit 1; fi
ns_flush
echo "REMOVE_OK: NanoSync removed. /Apps/Data/Importar and the library were kept. Restart the iPod."

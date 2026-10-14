#!/usr/bin/env bash
# Updates an already installed NanoSync: replaces only /Apps/Executables/Nanosync.hbapp with the one from the apps directory.
# The icon and the pack are rewritten only if they differ. Files that are already identical are not rewritten. The current
# executable is saved to the backup directory first; after writing, the file is read back and compared by SHA-256; any failure
# restores the previous executable. It never touches other executables, /Apps/Data (log.txt, dryrun.txt ...), the firmware or
# /iPod_Control. The data partition must be unmounted (do not eject the iPod). Restart the iPod afterwards.
#
#   sudo ./update-nanosync.sh --apps-dir DIR --device /dev/disk/by-id/usb-Apple_iPod_<SERIAL>-0:0 [--usb-serial ID] [--expected-size BYTES]
#   ./update-nanosync.sh --apps-dir DIR --image rehearsal.img          (FAT32 image, no root, no device)
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=nanosync-common.sh
source "$HERE/nanosync-common.sh"

APPS_DIR=
usage() {
  cat >&2 <<EOF
usage: sudo $0 --apps-dir DIR --device DEV [--usb-serial ID] [--expected-size BYTES] [--slot auto|A|B] [--backup-dir DIR]
       $0 --apps-dir DIR --image IMG [--slot auto|A|B] [--backup-dir DIR]
EOF
  exit 2
}
while [[ $# -gt 0 ]]; do
  case $1 in
    --apps-dir) APPS_DIR=${2:-}; shift 2;;
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
[[ -n $APPS_DIR ]] || usage
ns_need_tools date
ns_check_apps_dir "$APPS_DIR"
ns_open_target
ns_exists "/Apps/Executables/$EXE_NAME" || die "$EXE_NAME is not installed: use install-nanosync.sh (this script only UPDATES)"

stamp=$(date -u +%Y%m%dT%H%M%SZ)
ns_snap "/Apps/Executables/$EXE_NAME"
ns_snap "/Apps/Icons/$ICON_NAME"
ns_snap "$PACK_DST"
cp "$tmp/old/Apps/Executables/$EXE_NAME" "$BACKUP_DIR/$EXE_NAME.before-update.$stamp"
echo "backup : $BACKUP_DIR/$EXE_NAME.before-update.$stamp (${HAD[/Apps/Executables/$EXE_NAME]})"
if [[ -n ${HAD[$PACK_DST]} ]]; then cp "$tmp/old$PACK_DST" "$BACKUP_DIR/$(basename "$PACK_DST").before-update.$stamp"; fi

run() {
  ns_write_verify "$APPS_DIR/Executables/$EXE_NAME" "/Apps/Executables/$EXE_NAME" skip || return 1
  ns_write_verify "$APPS_DIR/Icons/$ICON_NAME" "/Apps/Icons/$ICON_NAME" skip || return 1
  ns_write_verify "$APPS_DIR/AllApps.pack" "$PACK_DST" skip || return 1     # last
}
if ! run; then ns_rollback; exit 1; fi
ns_flush
echo "UPDATE_OK: NanoSync updated. Restart the iPod. /Apps/Data/Importar (log.txt, dryrun.txt) was not touched."

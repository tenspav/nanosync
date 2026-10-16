#!/usr/bin/env bash
# NanoSync v0.0.0 - developed by tenspav
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
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
  ns_write_verify "$APPS_DIR/AllApps.pack" "$PACK_DST" skip || return 1
}
if ! run; then ns_rollback; exit 1; fi
ns_flush
echo "UPDATE_OK: NanoSync updated. Restart the iPod. /Apps/Data/Importar (log.txt, dryrun.txt) was not touched."

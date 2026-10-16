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
ns_need_tools find sort date
ns_check_apps_dir "$APPS_DIR"
ns_open_target
stamp=$(date -u +%Y%m%dT%H%M%SZ)
mapfile -t EXES < <(find "$APPS_DIR/Executables" -maxdepth 1 -type f -name '*.hbapp' -printf '%f\n' | LC_ALL=C sort)
mapfile -t ICONS < <(find "$APPS_DIR/Icons" -maxdepth 1 -type f -name '*.bin' -printf '%f\n' | LC_ALL=C sort)
echo "apps   : ${#EXES[@]} executable(s), ${#ICONS[@]} icon(s), pack $(hash_of "$APPS_DIR/AllApps.pack")"
ns_snap "$PACK_DST"
if [[ -n ${HAD[$PACK_DST]} ]]; then
  cp "$tmp/old$PACK_DST" "$BACKUP_DIR/$(basename "$PACK_DST").before-nanosync.$stamp"
  echo "backup : $BACKUP_DIR/$(basename "$PACK_DST").before-nanosync.$stamp (${HAD[$PACK_DST]})"
fi
for f in "${EXES[@]}"; do ns_snap "/Apps/Executables/$f"; done
for f in "${ICONS[@]}"; do ns_snap "/Apps/Icons/$f"; done
for f in "${EXES[@]}"; do
  if [[ -n ${HAD["/Apps/Executables/$f"]} ]]; then cp "$tmp/old/Apps/Executables/$f" "$BACKUP_DIR/$f.before-nanosync.$stamp"; fi
done
run() {
  local f
  echo "== executables =="
  for f in "${EXES[@]}"; do ns_write_verify "$APPS_DIR/Executables/$f" "/Apps/Executables/$f" || return 1; done
  echo "== icons =="
  for f in "${ICONS[@]}"; do ns_write_verify "$APPS_DIR/Icons/$f" "/Apps/Icons/$f" || return 1; done
  echo "== pack (last) =="
  ns_write_verify "$APPS_DIR/AllApps.pack" "$PACK_DST" || return 1
}
if ! run; then ns_rollback; exit 1; fi
ns_flush
echo "INSTALL_OK: ${#EXES[@]} app(s) installed in slot $SLOT."
echo "Restart the iPod and re-run the firmware project's resident installer so the new app shows up. Do not eject before unmounting."
echo "To undo: sudo $HERE/remove-nanosync.sh --pack PACK_WITHOUT_NANOSYNC ... (the previous pack is in $BACKUP_DIR)"

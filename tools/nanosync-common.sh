#!/usr/bin/env bash
# NanoSync v0.0.0 - developed by tenspav
set -euo pipefail
umask 077
MARKER=/Apps/Startup/resident-B.active
EXE_NAME="Nanosync.hbapp"
ICON_NAME="Nanosync.bin"
BACKUP_DIR=${BACKUP_DIR:-./nanosync-backup}
LOCK_DIR=${NANOSYNC_LOCK_DIR:-/var/lock}
DEVICE= IMAGE= USB_SERIAL= EXPECTED_SIZE= SLOT=auto
die() { printf 'REFUSED: %s\n' "$*" >&2; exit 1; }
hash_of() { sha256sum -- "$1" | awk '{print $1}'; }
ns_need_tools() {
  local tool
  for tool in sha256sum awk mdir mcopy mdel readlink mktemp "$@"; do
    command -v "$tool" >/dev/null 2>&1 || die "missing tool: $tool"
  done
}
ns_open_target() {
  [[ -n $DEVICE || -n $IMAGE ]] || die "give --device DEV or --image FILE"
  [[ -z $DEVICE || -z $IMAGE ]] || die "use --device OR --image, not both"
  case $SLOT in auto|A|B) ;; *) die "--slot must be auto, A or B";; esac
  if [[ -n $DEVICE ]]; then
    local dev props model serial size node
    [[ $EUID == 0 ]] || die "root required for --device (use sudo)"
    ns_need_tools blockdev udevadm lsblk findmnt flock
    dev=$(readlink -f -- "$DEVICE")
    [[ -b $dev ]] || die "$DEVICE is not a block device"
    props=$(udevadm info --query=property --name "$dev")
    model=$(sed -n 's/^ID_MODEL=//p' <<<"$props" | head -n1)
    serial=$(sed -n 's/^ID_SERIAL_SHORT=//p' <<<"$props" | head -n1)
    size=$(blockdev --getsize64 "$dev")
    [[ $model == iPod ]] || die "$dev is not an iPod (ID_MODEL='$model')"
    [[ -z $USB_SERIAL || $serial == "$USB_SERIAL" ]] || die "USB serial mismatch: device reports '$serial', expected '$USB_SERIAL'"
    [[ -z $EXPECTED_SIZE || $size == "$EXPECTED_SIZE" ]] || die "disk size mismatch: device is $size bytes, expected $EXPECTED_SIZE"
    echo "device : $dev  model=$model  usb-serial=$serial  size=$size"
    PART="${dev}1"
    [[ -b $PART ]] || die "data partition $PART not found"
    while read -r node; do
      [[ -z $(findmnt -rn -S "$node" -o TARGET 2>/dev/null || true) ]] || die "$node is mounted: unmount it first (do not eject the iPod)"
    done < <(lsblk -nrpo NAME "$dev")
    mkdir -p "$LOCK_DIR"
    exec 9>"$LOCK_DIR/nanosync-maintenance.lock"
    flock -n 9 || die "another maintenance operation holds the lock"
  else
    [[ -f $IMAGE ]] || die "image file not found: $IMAGE"
    PART=$IMAGE
    export MTOOLS_SKIP_CHECK=1
  fi
  mkdir -p "$BACKUP_DIR"
  tmp=$(mktemp -d)
  trap 'rm -rf -- "$tmp"' EXIT
  ns_exists "/Apps" || die "no /Apps directory on the target: not a nano 7 with the homebrew firmware?"
  case $SLOT in
    B) ns_exists "$MARKER" || die "slot B requested but $MARKER is missing"; PACK_DST=/Apps/AllApps-B.pack;;
    A) PACK_DST=/Apps/AllApps.pack;;
    auto) if ns_exists "$MARKER"; then SLOT=B; PACK_DST=/Apps/AllApps-B.pack; else SLOT=A; PACK_DST=/Apps/AllApps.pack; fi;;
  esac
  echo "slot   : $SLOT  (app pack: $PACK_DST)"
}
ns_exists() { mdir -i "$PART" "::$1" >/dev/null 2>&1; }
ns_read_hash() { rm -f "$tmp/read"; mcopy -n -i "$PART" "::$1" "$tmp/read" 2>/dev/null || return 1; hash_of "$tmp/read"; }
ns_check_apps_dir() {
  local d=$1
  [[ -n $d && -d $d ]] || die "--apps-dir must be the directory that contains AllApps.pack, Executables/ and Icons/"
  [[ -f $d/AllApps.pack ]] || die "$d/AllApps.pack missing"
  [[ -f $d/Executables/$EXE_NAME ]] || die "$d/Executables/$EXE_NAME missing (was nanosync included when the pack was built?)"
  [[ -f $d/Icons/$ICON_NAME ]] || die "$d/Icons/$ICON_NAME missing"
}
declare -A HAD=()
WROTE=()
ns_snap() {
  local dst=$1 f="$tmp/old$1"
  mkdir -p "$(dirname "$f")"
  if ns_exists "$dst"; then mcopy -n -i "$PART" "::$dst" "$f"; HAD[$dst]=$(hash_of "$f"); else HAD[$dst]=; fi
}
ns_write_verify() {
  local src=$1 dst=$2 skip=${3:-} want
  want=$(hash_of "$src")
  if [[ -n $skip && ${HAD[$dst]:-} == "$want" ]]; then echo "same  $dst  $want (not rewritten)"; return 0; fi
  WROTE+=("$dst")
  mcopy -o -i "$PART" "$src" "::$dst" || return 1
  [[ $(ns_read_hash "$dst") == "$want" ]] || { echo "readback mismatch for $dst" >&2; return 1; }
  echo "ok    $dst  $want"
}
ns_rollback() {
  echo "ERROR: rolling back..." >&2
  local d
  for d in ${WROTE[@]+"${WROTE[@]}"}; do
    if [[ -n ${HAD[$d]:-} ]]; then mcopy -o -i "$PART" "$tmp/old$d" "::$d" || echo "  failed to restore $d" >&2
    else mdel -i "$PART" "::$d" 2>/dev/null || true; fi
  done
  echo "ROLLBACK_OK: previous files restored (copies of overwritten files are also in $BACKUP_DIR)." >&2
}
ns_flush() { sync; if [[ -n $DEVICE ]]; then blockdev --flushbufs "$PART" || true; fi; }

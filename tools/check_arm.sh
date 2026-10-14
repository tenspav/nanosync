#!/usr/bin/env bash
# Sanity checks on the built ARM ELF (usage: check_arm.sh path/to/nanosync.reloc.elf).
#  - p_r_read must really CALL FOBJ_READ (blx). A compiler once removed the call when it was reached through a constant function
#    pointer parameter, so reads "returned" 0 bytes without calling the OS; the PC simulator cannot see that regression.
#  - fobj_append (the write path) must contain calls too.
#  - no VFP/NEON instructions anywhere (the app is integer-only).
set -euo pipefail
E=${1:?usage: check_arm.sh ELF}
n=$(arm-none-eabi-objdump -d --disassemble=p_r_read "$E" | grep -c "blx" || true)
w=$(arm-none-eabi-objdump -d --disassemble=fobj_append "$E" | grep -c "blx" || true)
v=$(arm-none-eabi-objdump -d "$E" | grep -ciE "\bv(ld|st|add|sub|mul|mov|cvt|cmp)[a-z0-9.]*\s" || true)
echo "p_r_read: $n blx; fobj_append (write): $w blx; VFP/NEON instructions: $v"
if [ "$n" -ge 1 ] && [ "$w" -ge 1 ] && [ "$v" = 0 ]; then echo CHECK_ARM_OK; else echo CHECK_ARM_FAILED; exit 1; fi

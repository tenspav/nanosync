#!/usr/bin/env bash
# NanoSync v0.0.0 - developed by tenspav
set -euo pipefail
E=${1:?usage: check_arm.sh ELF}
n=$(arm-none-eabi-objdump -d --disassemble=p_r_read "$E" | grep -c "blx" || true)
w=$(arm-none-eabi-objdump -d --disassemble=fobj_append "$E" | grep -c "blx" || true)
v=$(arm-none-eabi-objdump -d "$E" | grep -ciE "\bv(ld|st|add|sub|mul|mov|cvt|cmp)[a-z0-9.]*\s" || true)
echo "p_r_read: $n blx; fobj_append (write): $w blx; VFP/NEON instructions: $v"
if [ "$n" -ge 1 ] && [ "$w" -ge 1 ] && [ "$v" = 0 ]; then echo CHECK_ARM_OK; else echo CHECK_ARM_FAILED; exit 1; fi

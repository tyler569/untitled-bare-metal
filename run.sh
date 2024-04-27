#!/usr/bin/env bash

iso="untitled_bare_metal.iso"
mem="128M"
smp="4"
debugopt="-debugcon stdio"
gdbserver=""
tee="|& tee last_output"

while getopts "dmst" opt; do
  case $opt in
    d)
      debugopt="-d int,cpu_reset"
      ;;
    m)
      debugopt="-monitor stdio"
      ;;
    s)
      gdbserver="-S"
      ;;
    t)
      tee=""
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

exec qemu-system-x86_64 -s -vga std \
  -no-reboot \
  -m $mem \
  -smp $smp \
  -cdrom $iso \
  -display none \
  $debugopt \
  $gdbserver |& tee last_output

#!/usr/bin/env bash

iso="untitled_bare_metal.iso"
gdbserver=""

debugcon="-debugcon stdio"
debugint="-d int,cpu_reset"

debugopt="$debugcon"

while getopts "ds" opt; do
  case $opt in
    d)
      debugopt="$debugint"
      ;;
    s)
      gdbserver="-S"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

exec qemu-system-x86_64 -s -vga std -no-reboot -m 128M -cdrom $iso -display none $debugopt $gdbserver |& tee last_output

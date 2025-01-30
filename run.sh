#!/usr/bin/env bash

iso="untitled_bare_metal.iso"
mem="128M"
smp=2
debugopt="-debugcon stdio"
serialopt="-serial unix:/tmp/vm_uart.sock,server,nowait"
gdbserver=""
video="-display none"
tee="|& tee last_output"

while getopts "dmustv" opt; do
  case $opt in
    d)
      debugopt="-d int,cpu_reset"
      ;;
    m)
      debugopt="-monitor stdio"
      ;;
    u)
      debugopt="-serial stdio"
      serialopt=""
      ;;
    s)
      gdbserver="-S"
      ;;
    t)
      tee=""
      ;;
    v)
      video=""
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
  -M smm=off \
  $video \
  $debugopt \
  $serialopt \
  -cpu max \
  $gdbserver |& tee last_output

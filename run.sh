#!/usr/bin/env bash

iso="untitled_bare_metal.iso"
mem="128M"
smp=2
debugopt="-debugcon stdio"
serialopt="-serial unix:/tmp/vm_uart.sock,server,nowait"
netopt="-net nic,model=e1000 -net user"
gdbserver=""
video="-display none -vga virtio"
tee="|& tee last_output"

while getopts "dmustv" opt; do
  case $opt in
    d)
      debugopt="-d int,cpu_reset"
      ;;
    m)
      debugopt="-monitor stdio"
      tee=""
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
      video="-vga virtio"
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
  $netopt \
  -cpu max \
  $gdbserver |& tee last_output

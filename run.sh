#!/usr/bin/env bash

iso="untitled_bare_metal.iso"
mem="128M"
smp=2
debugopt="-chardev stdio,id=dbgio,logfile=last_output -device isa-debugcon,chardev=dbgio,iobase=0xe9"
serialopt="-serial unix:/tmp/vm_uart.sock,server,nowait"
netopt="-net nic,model=e1000e -net user"
gdbserver=""
video="-display none -vga virtio"

while getopts "dmustv" opt; do
  case $opt in
    d)
      debugopt="$debugopt -d int,cpu_reset"
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
      debugopt="-debugcon stdio"
      ;;
    v)
      video="-vga virtio"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

exec qemu-system-x86_64 -s \
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
  $gdbserver

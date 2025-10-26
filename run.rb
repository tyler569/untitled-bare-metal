#!/usr/bin/env ruby

require 'optparse'

# Default configuration
options = {
  iso: 'untitled_bare_metal.iso',
  mem: '128M',
  smp: 2,
  debug: false,
  monitor: false,
  uart: false,
  gdb_server: false,
  debugcon: false,
  video: false
}

parser = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [options]"
  opts.separator ""
  opts.separator "Launch QEMU with untitled_bare_metal.iso"
  opts.separator ""
  opts.separator "Configuration Options:"

  opts.on("--iso PATH", "ISO image to boot (default: #{options[:iso]})") do |v|
    options[:iso] = v
  end

  opts.on("--mem SIZE", "Memory size (default: #{options[:mem]})") do |v|
    options[:mem] = v
  end

  opts.on("--smp NUM", Integer, "Number of CPUs (default: #{options[:smp]})") do |v|
    options[:smp] = v
  end

  opts.separator ""
  opts.separator "Behavioral Options:"

  opts.on("-d", "--debug", "Enable debug logging (int,cpu_reset)") do
    options[:debug] = true
  end

  opts.on("-m", "--monitor", "Use QEMU monitor on stdio") do
    options[:monitor] = true
  end

  opts.on("-u", "--uart", "Use UART on stdio (disables unix socket)") do
    options[:uart] = true
  end

  opts.on("-s", "--gdb-server", "Start paused, waiting for GDB connection (-S)") do
    options[:gdb_server] = true
  end

  opts.on("-t", "--debugcon", "Use debugcon on stdio") do
    options[:debugcon] = true
  end

  opts.on("-v", "--video", "Enable video output (virtio VGA)") do
    options[:video] = true
  end

  opts.separator ""
  opts.separator "Examples:"
  opts.separator "  #{$0}                           # Launch with default settings"
  opts.separator "  #{$0} -v                        # Launch with video enabled"
  opts.separator "  #{$0} -s                        # Launch paused, waiting for GDB"
  opts.separator "  #{$0} --mem 256M --smp 4        # Launch with 256MB RAM and 4 CPUs"
  opts.separator "  #{$0} --iso mykernel.iso -u -v  # Launch custom ISO with UART and video"
  opts.separator ""

  opts.on("-h", "--help", "Show this help message") do
    puts opts
    exit
  end
end

parser.parse!

# Default options
debugopt = [
  "-chardev", "stdio,id=dbgio,logfile=last_output",
  "-device", "isa-debugcon,chardev=dbgio,iobase=0xe9"
]
serialopt = ["-serial", "unix:/tmp/vm_uart.sock,server,nowait"]
netopt = ["-net", "nic,model=e1000e", "-net", "user"]
video = ["-display", "none", "-vga", "virtio"]
gdbserver = []

# Apply options (order matters - later options can override earlier ones)
if options[:debug]
  debugopt += ["-d", "int,cpu_reset"]
end

if options[:monitor]
  debugopt = ["-monitor", "stdio"]
end

if options[:uart]
  debugopt = ["-serial", "stdio"]
  serialopt = []
end

if options[:gdb_server]
  gdbserver = ["-S"]
end

if options[:debugcon]
  debugopt = ["-debugcon", "stdio"]
end

if options[:video]
  video = ["-vga", "virtio"]
end

# Build QEMU command
cmd = [
  "qemu-system-x86_64",
  "-s",
  "-no-reboot",
  "-m", options[:mem],
  "-smp", options[:smp].to_s,
  "-cdrom", options[:iso],
  "-M", "smm=off",
  *video,
  *debugopt,
  *serialopt,
  *netopt,
  "-cpu", "max",
  *gdbserver
]

# Execute QEMU
exec(*cmd)

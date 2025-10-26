#!/usr/bin/env ruby
# Disassembly viewer - shows disassembly with source interleaved

require 'optparse'

# Default configuration
options = {
  file: ARGV[0] || 'build/kern/untitled_bare_metal',
  objdump: 'llvm-objdump',
  format: 'intel'
}

parser = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [file] [options]"
  opts.separator ""
  opts.separator "Disassemble kernel binary with source interleaving"
  opts.separator ""

  opts.on("-f", "--format FORMAT", [:intel, :att],
          "Assembly format (intel, att; default: intel)") do |f|
    options[:format] = f
  end

  opts.on("--objdump PATH", "Path to objdump binary (default: llvm-objdump)") do |v|
    options[:objdump] = v
  end

  opts.on("-h", "--help", "Show this help message") do
    puts opts
    exit
  end
end

parser.parse!

# Update file from remaining args if provided
options[:file] = ARGV[0] if ARGV[0]

# Check if binary exists
unless File.file?(options[:file])
  warn "Error: #{options[:file]} does not exist"
  warn "Usage: #{$0} [file]"
  exit 1
end

# Build objdump command
cmd = [
  options[:objdump],
  '-d',           # Disassemble
  '-S',           # Interleave source
]

# Add Intel syntax option if requested
cmd << '-Mintel' if options[:format] == :intel

# Specify sections to disassemble
cmd += [
  '-j.text',
  '-j.text.low',
  '-j.init',
  '-j.fini',
  options[:file]
]

# Pipe to less for pagination
pipe_cmd = "#{cmd.map { |arg| arg.include?(' ') ? "'#{arg}'" : arg }.join(' ')} | less"
exec(pipe_cmd)

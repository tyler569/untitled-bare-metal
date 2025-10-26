#!/usr/bin/env ruby
# Backtrace tool - parses debug output and resolves addresses to source locations

require 'optparse'

# Default configuration
file = ARGV[0] || 'build/kern/untitled_bare_metal'
addr2line_binary = 'llvm-addr2line'
log_file = 'last_output'

# Check if binary exists
unless File.file?(file)
  warn "File not found: #{file}"
  warn "Usage: #{$0} [file]"
  exit 1
end

# Check if log file exists
unless File.file?(log_file)
  warn "Log file not found: #{log_file}"
  exit 1
end

# Read the log file
output = File.read(log_file)

# Parse different error formats and extract addresses
addresses = []

if output =~ /\(0x.*\) <.*>/
  # Format: (0xADDRESS) <symbol>
  addresses = output.scan(/\(0x([0-9a-fA-F]+)\)/).flatten

elsif output =~ /^\s+[0-9]+:.*IP=/
  # Format: CPU dump with IP= field
  addresses = output.scan(/^\s+\d+:.*IP=0x([0-9a-fA-F]+)/).flatten.uniq

elsif output =~ /frame ip:/
  # Format: frame ip: ADDRESS
  addresses = output.scan(/frame ip:\s*(?:0x)?([0-9a-fA-F]+)/).flatten

elsif output =~ /Fault occurred at/
  # Format: Fault occurred at 0xADDRESS
  addresses = output.scan(/Fault occurred at\s+0x([0-9a-fA-F]+)/).flatten
end

# Exit if no addresses found
if addresses.empty?
  warn "No recognizable error format found in #{log_file}"
  exit 1
end

# Resolve addresses using addr2line
addr2line_cmd = [addr2line_binary, '-fips', '-e', file] + addresses
exec(*addr2line_cmd)

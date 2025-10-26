#!/usr/bin/env ruby
# Code formatter - runs clang-format on all C/header files

require 'optparse'
require 'find'

# Default configuration
options = {
  dirs: %w[arch include kern lib user],
  clang_format: 'clang-format',
  check: false,
  verbose: false
}

parser = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [options]"
  opts.separator ""
  opts.separator "Format all C and header files using clang-format"
  opts.separator ""

  opts.on("-c", "--check", "Check formatting without modifying files") do
    options[:check] = true
  end

  opts.on("-v", "--verbose", "Show files being formatted") do
    options[:verbose] = true
  end

  opts.on("--clang-format PATH", "Path to clang-format binary") do |v|
    options[:clang_format] = v
  end

  opts.on("-h", "--help", "Show this help message") do
    puts opts
    exit
  end
end

parser.parse!

# Find all C and header files
files = []
options[:dirs].each do |dir|
  next unless Dir.exist?(dir)

  Find.find(dir) do |path|
    next unless File.file?(path)
    next unless path.end_with?('.c', '.h')
    files << path
  end
end

if files.empty?
  warn "No C or header files found in: #{options[:dirs].join(', ')}"
  exit 1
end

puts "Formatting #{files.size} file(s)..." if options[:verbose]

# Build clang-format command
cmd = [options[:clang_format]]

if options[:check]
  # Check mode: dry-run to verify formatting
  cmd << '--dry-run'
  cmd << '-Werror'
end

cmd << '-i'  # In-place formatting
cmd += files

# Show files being formatted in verbose mode
if options[:verbose]
  files.each { |f| puts "  #{f}" }
end

# Execute clang-format
system(*cmd) or exit($?.exitstatus)

puts "Done!" if options[:verbose] && !options[:check]
puts "All files are properly formatted." if options[:check] && $?.success?

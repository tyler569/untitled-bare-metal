#!/usr/bin/env ruby
# Configure build directories with CMake

require 'optparse'
require 'fileutils'

# Default configuration
options = {
  toolchain: 'cmake/toolchains/x86_64.cmake',
  generator: 'Ninja',
  build_types: ['Debug', 'Release'],
  verbose: false
}

parser = OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [options]"
  opts.separator ""
  opts.separator "Configure CMake build directories"
  opts.separator ""

  opts.on("-t", "--toolchain PATH",
          "Toolchain file (default: #{options[:toolchain]})") do |v|
    options[:toolchain] = v
  end

  opts.on("-g", "--generator GEN",
          "CMake generator (default: #{options[:generator]})") do |v|
    options[:generator] = v
  end

  opts.on("-b", "--build-type TYPE",
          "Single build type (default: both Debug and Release)") do |v|
    options[:build_types] = [v]
  end

  opts.on("-v", "--verbose", "Show CMake output") do
    options[:verbose] = true
  end

  opts.on("--clean", "Remove build directories before configuring") do
    options[:clean] = true
  end

  opts.on("-h", "--help", "Show this help message") do
    puts opts
    exit
  end
end

parser.parse!

# Verify toolchain exists
unless File.file?(options[:toolchain])
  warn "Error: Toolchain file not found: #{options[:toolchain]}"
  exit 1
end

# Build directory mapping
build_dirs = {
  'Debug' => 'build',
  'Release' => 'build-release'
}

# Clean build directories if requested
if options[:clean]
  options[:build_types].each do |build_type|
    dir = build_dirs[build_type] || build_type.downcase
    if Dir.exist?(dir)
      puts "Removing #{dir}/"
      FileUtils.rm_rf(dir)
    end
  end
end

# Configure each build type
success = true

options[:build_types].each do |build_type|
  build_dir = build_dirs[build_type] || build_type.downcase

  puts "Configuring #{build_type} build in #{build_dir}/"

  cmd = [
    'cmake',
    '-B', build_dir,
    '-DCMAKE_BUILD_TYPE=' + build_type,
    '-DCMAKE_TOOLCHAIN_FILE=' + options[:toolchain],
    '-G', options[:generator]
  ]

  if options[:verbose]
    puts "  Running: #{cmd.join(' ')}"
    result = system(*cmd)
  else
    result = system(*cmd, out: File::NULL, err: File::NULL)
  end

  unless result
    warn "Error: Failed to configure #{build_type} build"
    success = false
  else
    puts "  ✓ #{build_type} build configured"
  end

  puts
end

exit(success ? 0 : 1)

const std = @import("std");

pub fn build(b: *std.Build) void {
    // Use freestanding x86_64 target for kernel and userland
    // Explicitly disable SSE/MMX to prevent use in kernel
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86_64,
        .cpu_model = .{ .explicit = .generic(.x86_64) },
        .os_tag = .freestanding,
        .abi = .none,
        .cpu_features_sub = std.Target.x86.featureSet(&.{
            .@"3dnow",
            .mmx,
            .sse,
            .sse2,
            .sse3,
        }),
    });

    const optimize = b.standardOptimizeOption(.{});

    const iso_file = b.pathJoin(&.{ b.install_prefix, "untitled_bare_metal.iso" });

    // Step 1: Download and build Limine bootloader
    const limine_step = setupLimine(b);

    // Step 2: Build userland programs
    const userland_programs = [_][]const u8{
        "userland",
        "calculator_server",
        "serial_driver",
    };

    const userland_sources = [_][]const u8{
        "user/cptr_alloc.c",
        "user/exec.c",
        "user/lib.c",
        "user/pci.c",
        "lib/num.c",
        "lib/print.c",
        "lib/sort.c",
        "lib/string.c",
        "lib/tar.c",
    };

    var userland_exes = std.ArrayList(*std.Build.Step.Compile).initCapacity(b.allocator, userland_programs.len) catch @panic("OOM");
    defer userland_exes.deinit(b.allocator);

    for (userland_programs) |program| {
        const exe = buildUserland(b, target, optimize, program, &userland_sources);
        userland_exes.append(b.allocator, exe) catch @panic("OOM");
    }

    // Step 3: Create initrd tarball from userland programs
    const initrd_tar = createInitrdTarball(b, userland_exes.items);

    // Step 4: Build kernel
    const kernel = buildKernel(b, target, optimize);

    // Step 5: Create ISO image
    const iso_step = createIso(b, limine_step, kernel, initrd_tar, iso_file);

    // Default step builds the ISO
    b.getInstallStep().dependOn(iso_step);

    // Add a "run" step to run the ISO in QEMU (matches run.sh defaults)
    const run_step = b.step("run", "Run the OS in QEMU");
    const qemu_cmd = b.addSystemCommand(&.{
        "qemu-system-x86_64", "-s", "-vga", "std", "-no-reboot",
        "-m", "128M", "-smp", "2",
        "-cdrom", iso_file,
        "-M", "smm=off",
        "-display", "none",
        "-vga", "virtio",
        "-debugcon", "stdio",
        "-serial", "unix:/tmp/vm_uart.sock,server,nowait",
        "-net", "nic,model=e1000e", "-net", "user",
        "-cpu", "max",
        // "-d", "int,cpu_reset",
    });
    qemu_cmd.step.dependOn(iso_step);
    run_step.dependOn(&qemu_cmd.step);
}

fn setupLimine(b: *std.Build) *std.Build.Step.Compile {
    const limine_dir = "limine";
    const limine_repo = "https://github.com/limine-bootloader/limine.git";
    const limine_branch = "v7.x-binary";

    // Check if limine directory exists, if not clone it
    const check_limine = b.addSystemCommand(&.{
        "sh",
        "-c",
        "test -d " ++ limine_dir ++ " || git clone --depth 1 --branch " ++ limine_branch ++ " " ++ limine_repo ++ " " ++ limine_dir,
    });

    // Build the limine binary using Zig's build system (for the host machine)
    const host_target = b.resolveTargetQuery(.{});

    const limine_exe = b.addExecutable(.{
        .name = "limine",
        .root_module = b.createModule(.{
            .target = host_target,
            .optimize = .ReleaseSafe,
        }),
    });

    limine_exe.addCSourceFile(.{
        .file = b.path("limine/limine.c"),
        .flags = &.{ "-std=c99", "-Wall", "-Wextra" },
    });
    limine_exe.linkLibC();
    limine_exe.step.dependOn(&check_limine.step);

    return limine_exe;
}

fn buildUserland(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    name: []const u8,
    common_sources: []const []const u8,
) *std.Build.Step.Compile {
    const root_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .red_zone = false,
        .sanitize_c = .off,
        .error_tracing = false,
        .no_builtin = true,
    });

    const exe = b.addExecutable(.{
        .name = name,
        .root_module = root_module,
    });
    exe.bundle_compiler_rt = false;

    // Add program-specific source
    const program_source = b.fmt("user/{s}.c", .{name});
    exe.addCSourceFile(.{
        .file = b.path(program_source),
        .flags = &userland_cflags,
    });

    // Add common sources
    for (common_sources) |src| {
        exe.addCSourceFile(.{
            .file = b.path(src),
            .flags = &userland_cflags,
        });
    }

    exe.addIncludePath(b.path("include"));
    exe.addIncludePath(b.path("limine"));
    root_module.addCMacro("__shrike__", "1");

    return exe;
}

const userland_cflags = [_][]const u8{
    "-std=c23",
    "-fasm",
};

fn createInitrdTarball(
    b: *std.Build,
    userland_exes: []*std.Build.Step.Compile,
) std.Build.LazyPath {
    // Create tarball directly from the emitted binaries (no intermediate install)
    const tar_cmd = b.addSystemCommand(&.{ "gtar", "-cf" });
    const tar_output = tar_cmd.addOutputFileArg("initrd.tar");
    tar_cmd.addArg("--transform=s|.*/||");  // Strip directory paths in archive

    // Add each executable binary as an input - this tracks dependencies properly
    for (userland_exes) |exe| {
        tar_cmd.addFileArg(exe.getEmittedBin());
    }

    return tar_output;
}

fn buildKernel(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) *std.Build.Step.Compile {
    const root_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .code_model = .kernel,
        .red_zone = false,
        .sanitize_c = .off,
        .error_tracing = false,
        .no_builtin = true,
    });

    const kernel = b.addExecutable(.{
        .name = "untitled_bare_metal",
        .root_module = root_module,
    });
    kernel.bundle_compiler_rt = false;

    // Kernel sources
    const kernel_sources = [_][]const u8{
        "kern/main.c",
        "kern/alloc.c",
        "kern/boot.c",
        "kern/debug.c",
        "kern/elf.c",
        "kern/ipc.c",
        "kern/syscall.c",
        "kern/obj/cnode.c",
        "kern/obj/endpoint.c",
        "kern/obj/irq.c",
        "kern/obj/notification.c",
        "kern/obj/tcb.c",
        "kern/obj/untyped.c",
    };

    // Library sources
    const lib_sources = [_][]const u8{
        "lib/hexdump.c",
        "lib/num.c",
        "lib/print.c",
        "lib/sort.c",
        "lib/spin_lock.c",
        "lib/string.c",
        "lib/tar.c",
    };

    // x86_64 architecture sources
    const arch_sources = [_][]const u8{
        "arch/x86_64/debug.c",
        "arch/x86_64/entry.c",
        "arch/x86_64/frame.c",
        "arch/x86_64/gdt.c",
        "arch/x86_64/idt.c",
        "arch/x86_64/initrd.c",
        "arch/x86_64/intrin.c",
        "arch/x86_64/ints.c",
        "arch/x86_64/ioapic.c",
        "arch/x86_64/lapic.c",
        "arch/x86_64/pci.c",
        "arch/x86_64/phy_mem.c",
        "arch/x86_64/pic.c",
        "arch/x86_64/mmu.c",
        "arch/x86_64/smp.c",
        "arch/x86_64/syscall.c",
        "arch/x86_64/obj/io_port.c",
        "arch/x86_64/obj/page.c",
    };

    const kernel_cflags = [_][]const u8{
        "-std=c23",
        "-fasm",
    };

    for (kernel_sources) |src| {
        kernel.addCSourceFile(.{
            .file = b.path(src),
            .flags = &kernel_cflags,
        });
    }

    for (lib_sources) |src| {
        kernel.addCSourceFile(.{
            .file = b.path(src),
            .flags = &kernel_cflags,
        });
    }

    for (arch_sources) |src| {
        kernel.addCSourceFile(.{
            .file = b.path(src),
            .flags = &kernel_cflags,
        });
    }

    // Assembly sources
    kernel.addAssemblyFile(b.path("arch/x86_64/isrs.S"));
    kernel.addAssemblyFile(b.path("arch/x86_64/syscall.S"));

    kernel.addIncludePath(b.path("include"));
    kernel.addIncludePath(b.path("limine"));
    root_module.addCMacro("__KERNEL__", "1");
    root_module.addCMacro("__shrike__", "1");
    kernel.setLinkerScript(b.path("arch/x86_64/link.ld"));

    return kernel;
}

fn createIso(
    b: *std.Build,
    limine_exe: *std.Build.Step.Compile,
    kernel: *std.Build.Step.Compile,
    initrd_tar: std.Build.LazyPath,
    _: []const u8,
) *std.Build.Step {
    const iso_dir = b.pathJoin(&.{ b.install_prefix, "isodir" });
    const iso_boot_dir = b.pathJoin(&.{ b.install_prefix, "isodir", "boot" });
    const iso_boot_limine_dir = b.pathJoin(&.{ b.install_prefix, "isodir", "boot", "limine" });

    // Create ISO directory structure
    const mkdir_cmd = b.addSystemCommand(&.{
        "mkdir",
        "-p",
        iso_boot_limine_dir,
    });

    // Copy limine.cfg
    const copy_cfg = b.addSystemCommand(&.{
        "cp",
        "limine.cfg",
        iso_boot_limine_dir,
    });
    copy_cfg.step.dependOn(&mkdir_cmd.step);

    // Copy Limine files (these are pre-built binaries from the git repo)
    const copy_limine_files = b.addSystemCommand(&.{
        "cp",
        "limine/limine-bios.sys",
        "limine/limine-bios-cd.bin",
        "limine/limine-uefi-cd.bin",
        iso_boot_limine_dir,
    });
    copy_limine_files.step.dependOn(&limine_exe.step);
    copy_limine_files.step.dependOn(&mkdir_cmd.step);

    // Install kernel to ISO directory
    const install_kernel = b.addInstallArtifact(kernel, .{
        .dest_dir = .{ .override = .{ .custom = "isodir/boot" } },
    });
    install_kernel.step.dependOn(&mkdir_cmd.step);

    // Track the kernel binary as an input dependency
    const kernel_bin = kernel.getEmittedBin();

    // Copy initrd.tar to ISO
    const copy_initrd = b.addSystemCommand(&.{"cp"});
    copy_initrd.addFileArg(initrd_tar);
    copy_initrd.addArg(iso_boot_dir);
    copy_initrd.step.dependOn(&mkdir_cmd.step);

    // Create ISO using xorriso
    const xorriso_cmd = b.addSystemCommand(&.{
        "xorriso",
        "-as",
        "mkisofs",
        "-b",
        "boot/limine/limine-bios-cd.bin",
        "-no-emul-boot",
        "-boot-load-size",
        "4",
        "--boot-info-table",
        "--efi-boot",
        "boot/limine/limine-uefi-cd.bin",
        "-efi-boot-part",
        "--efi-boot-image",
        "--protective-msdos-label",
        iso_dir,
        "-o",
    });
    const iso_output = xorriso_cmd.addOutputFileArg("untitled_bare_metal.iso");

    // Add input file dependencies so Zig knows to rebuild when they change
    xorriso_cmd.addFileArg(kernel_bin);
    xorriso_cmd.addFileArg(initrd_tar);

    xorriso_cmd.step.dependOn(&copy_cfg.step);
    xorriso_cmd.step.dependOn(&copy_limine_files.step);
    xorriso_cmd.step.dependOn(&install_kernel.step);
    xorriso_cmd.step.dependOn(&copy_initrd.step);

    // Install Limine to the ISO using the compiled limine executable
    const limine_install = b.addRunArtifact(limine_exe);
    limine_install.addArg("bios-install");
    limine_install.addFileArg(iso_output);
    limine_install.step.dependOn(&xorriso_cmd.step);

    // Install the ISO to the output directory
    const install_iso = b.addInstallFile(iso_output, "untitled_bare_metal.iso");
    install_iso.step.dependOn(&limine_install.step);

    return &install_iso.step;
}

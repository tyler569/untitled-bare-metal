const std = @import("std");

// Constants
const iso_name = "untitled_bare_metal.iso";
const iso_dir_name = "isodir";
const limine_dir = "limine";
const limine_repo = "https://github.com/limine-bootloader/limine.git";
const limine_branch = "v7.x-binary";

const userland_cflags = [_][]const u8{
    "-std=c23",
    "-fasm",
};

const kernel_cflags = [_][]const u8{
    "-std=c23",
    "-fasm",
};

pub fn build(b: *std.Build) void {
    // Use freestanding x86_64 target for kernel and userland
    // Explicitly disable SSE/MMX to prevent use in kernel
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86_64,
        .cpu_model = .{ .explicit = .generic(.x86_64) },
        .os_tag = .freestanding,
        .abi = .none,
        .cpu_features_add = std.Target.x86.featureSet(&.{ .soft_float }),
        .cpu_features_sub = std.Target.x86.featureSet(&.{
            .x87,
            .@"3dnow",
            .mmx,
            .sse,
            .sse2,
            .sse3,
        }),
    });

    const optimize = b.standardOptimizeOption(.{});

    const iso_file = b.pathJoin(&.{ b.install_prefix, iso_name });

    const limine_step = setupLimine(b);

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

    var userland_exes: [userland_programs.len]*std.Build.Step.Compile = undefined;
    for (userland_programs, 0..) |program, i| {
        userland_exes[i] = buildUserland(b, target, optimize, program, &userland_sources);
    }

    const initrd_tar = createInitrdTarball(b, &userland_exes);

    const kernel = buildKernel(b, target, optimize);

    const iso_step = createIso(b, limine_step, kernel, initrd_tar);

    b.getInstallStep().dependOn(iso_step);

    const run_step = b.step("run", "Run the OS in QEMU");
    const qemu_cmd = b.addSystemCommand(&.{
        "qemu-system-x86_64", "-s",
        "-vga",               "std",
        "-m",                 "128M",
        "-smp",               "2",
        "-cdrom",             iso_file,
        "-M",                 "smm=off",
        "-display",           "none",
        "-vga",               "virtio",
        "-chardev",           "stdio,id=dbgio,logfile=last_output",
        "-device",            "isa-debugcon,chardev=dbgio,iobase=0xe9",
        "-serial",            "unix:/tmp/vm_uart.sock,server,nowait",
        "-net",               "nic,model=e1000e",
        "-net",               "user",
        "-cpu",               "max",
        "-no-reboot",
        // "-d", "int,cpu_reset",
    });
    qemu_cmd.step.dependOn(iso_step);
    run_step.dependOn(&qemu_cmd.step);
}

fn setupLimine(b: *std.Build) *std.Build.Step.Compile {
    // Check if limine directory exists, if not clone it
    const check_limine = b.addSystemCommand(&.{
        "sh", "-c",
        "test -d " ++ limine_dir ++
            " || git clone --depth 1 --branch " ++
            limine_branch ++ " " ++ limine_repo ++ " " ++ limine_dir,
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
    });

    const exe = b.addExecutable(.{
        .name = name,
        .root_module = root_module,
    });

    // Add program-specific source
    const program_source = b.fmt("user/{s}.c", .{name});
    exe.addCSourceFile(.{
        .file = b.path(program_source),
        .flags = &userland_cflags,
    });

    exe.addCSourceFiles(.{
        .files = common_sources,
        .flags = &userland_cflags,
    });

    exe.addIncludePath(b.path("include"));
    exe.addIncludePath(b.path("limine"));
    root_module.addCMacro("__shrike__", "1");

    return exe;
}

fn createInitrdTarball(
    b: *std.Build,
    userland_exes: []*std.Build.Step.Compile,
) std.Build.LazyPath {
    // Create tarball directly from the emitted binaries (no intermediate install)
    const tar_cmd = b.addSystemCommand(&.{ "gtar", "-cf" });
    const tar_output = tar_cmd.addOutputFileArg("initrd.tar");
    tar_cmd.addArg("--transform=s|.*/||"); // Strip directory paths in archive

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
    });

    const kernel = b.addExecutable(.{
        .name = "untitled_bare_metal",
        .root_module = root_module,
    });

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

    kernel.addCSourceFiles(.{
        .files = &(kernel_sources ++ lib_sources ++ arch_sources),
        .flags = &kernel_cflags,
    });

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
) *std.Build.Step {
    const iso_dir = b.pathJoin(&.{ b.install_prefix, iso_dir_name });
    const iso_boot_dir = b.pathJoin(&.{ b.install_prefix, iso_dir_name, "boot" });
    const iso_boot_limine_dir = b.pathJoin(&.{ b.install_prefix, iso_dir_name, "boot", "limine" });

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
    const iso_output = xorriso_cmd.addOutputFileArg(iso_name);

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
    const install_iso = b.addInstallFile(iso_output, iso_name);
    install_iso.step.dependOn(&limine_install.step);

    return &install_iso.step;
}

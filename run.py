#!/usr/bin/env python3
import argparse
import os
import shlex
import signal
import subprocess
import sys

def parse_args():
    parser = argparse.ArgumentParser(
        description="Run qemu-system-x86_64 with various debugging options."
    )
    parser.add_argument(
        "-d",
        action="store_true",
        help="Use debug options: -d int,cpu_reset"
    )
    parser.add_argument(
        "-m",
        action="store_true",
        help="Use monitor stdio (disables tee)"
    )
    parser.add_argument(
        "-u",
        action="store_true",
        help="Use serial stdio (disables serial unix socket)"
    )
    parser.add_argument(
        "-s",
        action="store_true",
        help="Enable GDB server (-S)"
    )
    parser.add_argument(
        "-t",
        action="store_true",
        help="Do not pipe output to tee"
    )
    parser.add_argument(
        "-v",
        action="store_true",
        help="Set video option to '-vga virtio'"
    )
    return parser.parse_args()

def build_command(args):
    # Defaults
    iso = "untitled_bare_metal.iso"
    mem = "128M"
    smp = "2"  # number of CPUs
    debugopt = "-debugcon stdio"
    serialopt = "-serial unix:/tmp/vm_uart.sock,server,nowait"
    netopt = "-net nic,model=e1000 -net user"
    gdbserver = ""
    video = "-display none -vga virtio"
    use_tee = True  # by default, pipe output to tee

    # Process options
    if args.d:
        debugopt = "-d int,cpu_reset"
    if args.m:
        debugopt = "-monitor stdio"
        use_tee = False
    if args.u:
        debugopt = "-serial stdio"
        serialopt = ""
    if args.s:
        gdbserver = "-S"
    if args.t:
        use_tee = False
    if args.v:
        video = "-vga virtio"  # remove "-display none"

    # Build command list.
    # Use shlex.split() for options that contain multiple tokens.
    cmd = [
        "qemu-system-x86_64",
        "-s",
        "-vga", "std",
        "-no-reboot",
        "-m", mem,
        "-smp", smp,
        "-cdrom", iso,
        "-M", "smm=off"
    ]
    cmd.extend(shlex.split(video))
    cmd.extend(shlex.split(debugopt))
    if serialopt:
        cmd.extend(shlex.split(serialopt))
    cmd.extend(shlex.split(netopt))
    cmd.extend(["-cpu", "max"])
    if gdbserver:
        cmd.extend(shlex.split(gdbserver))

    return cmd, use_tee

def restore_terminal():
    """Attempt to restore terminal settings."""
    try:
        subprocess.run(["stty", "sane"], check=True)
    except Exception:
        pass

def run_with_tee(cmd):
    """
    Emulate "|& tee last_output" by capturing combined stdout/stderr,
    writing to both sys.stdout and the file "last_output". Also, ensure that
    if interrupted (C-C), we kill the child process group and restore the terminal.
    """
    logfile = open("last_output", "w")
    # Launch the process in its own process group so we can send signals to the group.
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        preexec_fn=os.setsid
    )

    try:
        for line in process.stdout:
            sys.stdout.write(line)
            logfile.write(line)
        process.stdout.close()
        return_code = process.wait()
        logfile.close()
        if return_code:
            sys.exit(return_code)
    except KeyboardInterrupt:
        # Send SIGTERM to the process group
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except Exception:
            pass
        logfile.close()
        restore_terminal()
        sys.exit(1)
    finally:
        restore_terminal()

def main():
    args = parse_args()
    cmd, use_tee = build_command(args)
    # Uncomment the following line to see the command being executed:
    # print("Executing:", " ".join(shlex.quote(arg) for arg in cmd))

    if use_tee:
        run_with_tee(cmd)
    else:
        # When not using tee, run the command with Popen in its own process group,
        # then forward signals (like KeyboardInterrupt) and restore terminal afterward.
        try:
            process = subprocess.Popen(cmd, preexec_fn=os.setsid)
            process.wait()
        except KeyboardInterrupt:
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except Exception:
                pass
            restore_terminal()
            sys.exit(1)
        finally:
            restore_terminal()

if __name__ == "__main__":
    main()

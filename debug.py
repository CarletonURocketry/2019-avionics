#! /usr/bin/env python3

#
# Script to run OpenOCD and GDB for CU InSpace avionics software makefile.
#
# Samuel Dewan
# 2019-01-20
#

import os
import sys
import subprocess
import signal
import argparse
import io
import socket
import shutil
import time

# IO class to read lines from socket
class SocketIO(io.RawIOBase):
    def __init__(self, sock):
        self.sock = sock
    def read(self, size=4096):
        return self.sock.recv(size)
    def seekable(self):
        return False

# Send a command to a port. Returns true if successful. If command is none,
# check if the port is open but do not send anything.
def send_tcp_command(host, port, command = None, wait_for = None):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(0.2)
    result = False
    try:
        sock.connect((host, port))
        if command is not None:
            sock.settimeout(None)
            position = 0
            while position < len(command):
                position += sock.send(command[position:])
            reader = SocketIO(sock)
            if wait_for is not None:
                for line in reader:
                    if wait_for in line:
                        break
        result = True
    except ConnectionRefusedError as e:
        pass
    sock.close()
    return result

def debug_print(message, verbose):
    if verbose:
        print(message)
    

# Parse arguments
parser = argparse.ArgumentParser(description='Run OpenOCD and GDB.')
parser.add_argument('binary', nargs='?', default=None, help='Binary to debug.')
parser.add_argument('--gdb', help='Path to GDB executable.')
parser.add_argument('--openocd', help='Path to OpenOCD executable.')
parser.add_argument('--interface', default='cmsis', help='Programming interface.')
parser.add_argument('--host', default='localhost', help='GDB server host.')
parser.add_argument('--gdb-port', default=2331, type=int,
                    help='GDB server port.')
parser.add_argument('--openocd-port', default=4444, type=int,
                    help='OpenOCD command port.')
parser.add_argument('--gdb-file', default=None,
                    help='File (pipe or serial interface) to connect to with GDB')
parser.add_argument('--upload', default=False, action="store_true",
                    help='Upload binary to target.')
parser.add_argument('--reset', nargs='?', default=None, const="run",
                    help='Reset target.')
parser.add_argument('--no-debug', default=False, action="store_true",
                    help='Do not launch interactive GDB.')
parser.add_argument('--verbose', default=False, action="store_true",
                    help='Print debugging output.')

args = parser.parse_args()

# Disallow use of gdb-file with reset or upload
if args.gdb_file is not None and (args.upload or args.reset):
    sys.exit("Cannot use --gdb_file with --upload or --reset (OpenOCD server "
             "required)")
    
# Check if binary exists if it is required
if args.binary is not None and not os.path.isfile(args.binary):
    sys.exit("Binary \"{0}\" does not exist.".format(args.binary))
elif args.binary is None and (not args.no_debug or args.upload):
    sys.exit("No binary specifed.")

# Check if gdb-file exists if it was specified
if args.gdb_file is not None and not os.path.exists(args.gdb_file):
    sys.exit("File \"{0}\" does not exist.".format(args.gdb_file))

# Find OpenOCD if GDB server is not already accessable
openocd_path = None

if args.gdb_file is None and not send_tcp_command(args.host, args.gdb_port):
    # Need to launch our own GDB server
    openocd_path = args.openocd if args.openocd is not None else shutil.which("openocd")
    if openocd_path is None:
        sys.exit("Could not find OpenOCD")
    elif not os.path.isfile(openocd_path):
        sys.exit("OpenOCD at \"{0}\" does not exist.".format(openocd_path))
    elif not os.access(openocd_path, os.X_OK):
        sys.exit("OpenOCD at \"{0}\" is not executable.".format(openocd_path))

debug_print("OpenOCD at: {0}".format(openocd_path), args.verbose)

# Find OpenOCD config
openocd_cfg = None

if openocd_path is not None:
    # Need to find a config file for OpenOCD
    if args.interface.casefold() == "cmsis":
        openocd_cfg = "openocd.cfg"
    else:
        openocd_cfg = "openocd-{0}.cfg".format(args.interface.casefold())
    
    if openocd_cfg is not None and not os.path.isfile(openocd_cfg):
        sys.exit("Could not find config file \"{0}\".".format(openocd_cfg))

debug_print("OpenOCD config: {0}".format(openocd_cfg), args.verbose)

# Find GDB
gdb_path = args.gdb
gdb_arch_cmd = None

if gdb_path is None:
    gdb_path = shutil.which("arm-none-eabi-gdb")
if gdb_path is None:
    gdb_path = shutil.which("gdb-multiarch")
    gdb_arch_cmd = "set architecture armv6-m"
if gdb_path is None:
    gdb_path = shutil.which("gdb")
    gdb_arch_cmd = "set architecture armv6-m"
if gdb_path is None:
    sys.exit("Could not find GDB.")

if not os.path.isfile(gdb_path):
    sys.exit("GDB at \"{0}\" does not exist.".format(gdb_path))
elif not os.access(gdb_path, os.X_OK):
    sys.exit("GDB at \"{0}\" is not executable.".format(gdb_path))

debug_print("GDB at: {0}".format(gdb_path), args.verbose)

# Launch OpenOCD
openocd_proc = None

if openocd_path is not None:
    debug_print("Starting OpenOCD", args.verbose)
    openocd_proc = subprocess.Popen([openocd_path, "-f", openocd_cfg, "-l",
                                     "/dev/null"], preexec_fn = os.setsid,
                                     stderr = subprocess.DEVNULL)
    # TODO: Find a way to detect when OpenOCD is ready to accept connections
    # instead of sleeping
    if args.upload or args.reset:
        # We will need to connect right away, sleep to let OpenOCD start up first
        time.sleep(0.5)

# Upload firmware if requested
if args.upload:
    print("Uploading...", end="", flush=True);
    result = send_tcp_command(args.host, args.openocd_port,
                              command = b"reset halt\nflash write_image erase %b\n" %
                              args.binary.encode('utf-8'), wait_for = b"wrote")
    if not result:
        print()
        if openocd_proc is not None:
            openocd_proc.kill()
        sys.exit("GDB is available but OpenOCD does not appear to be running.")
    print(" done.")

# Reset target if requested
if args.reset:
    print("Resetting...", end="", flush=True);
    result = send_tcp_command(args.host, args.openocd_port,
                              command = b"reset %b\n" %
                              args.reset.encode('utf-8'))
    # TODO: Find a way to detect when OpenOCD is finished rather than sleeping
    time.sleep(0.5)
    if not result:
        print()
        if openocd_proc is not None:
            openocd_proc.kill()
        sys.exit("GDB is available but OpenOCD does not appear to be running.")
    print(" done.")

# If interactive debugging was disabled we are done now
if args.no_debug:
    # Kill OpenOCD if we launched it
    if openocd_proc is not None:
        openocd_proc.kill()
    sys.exit(0)
   
# Marshal GDB arguments
gdb_remote = args.gdb_file
if gdb_remote is None:
    gdb_remote = "{0}:{1}".format(args.host, args.gdb_port)

gdb_args = [gdb_path]
if gdb_arch_cmd is not None:
    gdb_args.extend(["-ex", gdb_arch_cmd])
gdb_args.extend(["-ex", "target extended-remote {0}".format(gdb_remote),
                args.binary])

# Ignore SIGINT so that Python will not exit when interrupt chararacter is typed
signal.signal(signal.SIGINT, signal.SIG_IGN)

# Launch GDB
gdb_proc = subprocess.Popen(gdb_args);

# Wait for GDB to complete
gdb_proc.wait()

# If we started OpenOCD, kill it
if openocd_proc is not None:
    openocd_proc.kill()

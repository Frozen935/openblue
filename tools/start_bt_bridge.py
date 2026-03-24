#!/usr/bin/env python3
"""
NuttX Bluetooth HCI bridge helper for Ubuntu and macOS.

Ubuntu usage:
  1) Plug in a USB Bluetooth dongle and confirm the HCI index (e.g. hci0)
  2) Start the bridge (prepares dependencies and stops system Bluetooth)
     ./start_bt_bridge.py --hci-index 0
  3) Configure the QEMU Bluetooth node, example:
     -device virtio-serial-device,bus=virtio-mmio-bus.10 \
     -chardev socket,path=/tmp/hci_bridge.sock,id=qemubt \
     -device virtconsole,chardev=qemubt

macOS usage:
  1) Plug in a USB Bluetooth dongle and find VID/PID:
     system_profiler SPUSBDataType
  2) Start the bridge:
     ./start_bt_bridge.py --vid 0x0a12 --pid 0x0001
  3) Configure the QEMU Bluetooth node, same as above

Successful startup logs (example):
  [INFO] Preparing Linux Bluetooth adapter (hci0)...
  [SUCCESS] Adapter hci0 is ready (DOWN state).
  [SUCCESS] Starting Bumble Bridge on /tmp/hci_bridge.sock...
  [INFO] Press Ctrl+C to stop and restore system Bluetooth.
  >>> connecting to HCI...
  >>> connected
  >>> connecting to HCI...
  >>> connected

Node access logs (example):
  20:09:56.513 I bumble.bridge: [CONTROLLER->HOST] HCI_RESET_COMMAND
  20:09:56.732 I bumble.bridge: [HOST->CONTROLLER] HCI_COMMAND_COMPLETE_EVENT:
    num_hci_command_packets: 1
    command_opcode:          HCI_RESET_COMMAND
    return_parameters:
      status: SUCCESS
"""
import os
import sys
import subprocess
import platform
import shutil
import time
import argparse
import signal

# --- Core Configuration ---
# Update this to your desired socket path
SOCKET_PATH = "/tmp/hci_bridge.sock"
SOCKET_MODE = 0o666

# Virtual environment configuration
VENV_DIR = os.path.join(os.getcwd(), "bumble_env")
PYTHON_BIN = os.path.join(VENV_DIR, "bin", "python3")
PIP_BIN = os.path.join(VENV_DIR, "bin", "pip")

# --- Helper: colored logs ---


class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'


def log(msg, level="info"):
    if level == "info":
        print(f"{Colors.OKBLUE}[INFO] {msg}{Colors.ENDC}")
    elif level == "success":
        print(f"{Colors.OKGREEN}[SUCCESS] {msg}{Colors.ENDC}")
    elif level == "warn":
        print(f"{Colors.WARNING}[WARN] {msg}{Colors.ENDC}")
    elif level == "error":
        print(f"{Colors.FAIL}[ERROR] {msg}{Colors.ENDC}")


def run_cmd(cmd, shell=False, sudo=False, check=True):
    """Run a shell command with optional sudo"""
    if sudo and os.geteuid() != 0:
        if isinstance(cmd, list):
            cmd = ["sudo"] + cmd
        else:
            cmd = "sudo " + cmd

    cmd_str = cmd if isinstance(cmd, str) else " ".join(cmd)
    try:
        subprocess.run(cmd, shell=shell, check=check)
    except subprocess.CalledProcessError:
        log(f"Command failed: {cmd_str}", "error")
        sys.exit(1)


def is_bumble_ready():
    if not os.path.exists(PYTHON_BIN) or not os.path.exists(PIP_BIN):
        return False
    try:
        result = subprocess.run(
            [PYTHON_BIN, "-c", "import bumble"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        return result.returncode == 0
    except Exception:
        return False


def ensure_runtime_ready():
    if not os.path.exists(PYTHON_BIN):
        log(f"Python binary not found: {PYTHON_BIN}", "error")
        log("Run without --skip-install to set up the environment.", "warn")
        sys.exit(1)
    try:
        result = subprocess.run(
            [PYTHON_BIN, "-c", "import bumble"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
    except Exception:
        result = None
    if result is None or result.returncode != 0:
        log("Bumble is not available in the virtual environment.", "error")
        log("Run without --skip-install to install Bumble.", "warn")
        sys.exit(1)


def check_linux_tools():
    missing = []
    for tool in ["sudo", "rfkill", "hciconfig", "systemctl", "pkill"]:
        if shutil.which(tool) is None:
            missing.append(tool)
    if missing:
        log(f"Missing required tools: {', '.join(missing)}", "error")
        log("Run without --skip-install to install dependencies.", "warn")
        sys.exit(1)


def ensure_sudo_available():
    if os.geteuid() == 0:
        return
    if shutil.which("sudo") is None:
        log("sudo is not available on this system.", "error")
        sys.exit(1)
    result = subprocess.run(
        ["sudo", "-n", "true"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )
    if result.returncode != 0:
        log("sudo permission is required to control Bluetooth.", "error")
        log("Run with sudo or configure passwordless sudo for required commands.", "warn")
        sys.exit(1)


def set_socket_path(path_value):
    global SOCKET_PATH
    SOCKET_PATH = path_value


def apply_socket_permissions():
    deadline = time.time() + 3.0
    while time.time() < deadline:
        if os.path.exists(SOCKET_PATH):
            try:
                os.chmod(SOCKET_PATH, SOCKET_MODE)
                log(f"Socket permissions set to {oct(SOCKET_MODE)}", "success")
                return
            except PermissionError:
                if os.geteuid() == 0:
                    break
                run_cmd(
                    f"chmod {oct(SOCKET_MODE)[2:]} {SOCKET_PATH}", shell=True, sudo=True, check=False)
                return
        time.sleep(0.1)
    if os.path.exists(SOCKET_PATH):
        log(f"Unable to change permissions for {SOCKET_PATH}", "warn")


def check_hci_available(hci_index):
    result = subprocess.run(
        f"hciconfig hci{hci_index}",
        shell=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )
    if result.returncode != 0:
        log(f"Bluetooth adapter hci{hci_index} not found.", "error")
        log("Check that your adapter is present and recognized.", "warn")
        sys.exit(1)

# --- 1. Environment checks and dependency install ---


def install_dependencies():
    if is_bumble_ready():
        return
    os_type = platform.system()
    log(f"Detected OS: {os_type}")

    if os_type == "Linux":
        log("Checking Linux system dependencies...")
        ensure_sudo_available()
        run_cmd(["sudo", "apt", "update", "-y"], check=False)
        run_cmd(["sudo", "apt", "install", "-y", "python3-venv",
                "python3-dev", "libusb-1.0-0-dev", "bluez-tools", "net-tools"])

    elif os_type == "Darwin":
        log("Checking macOS dependencies...")
        if shutil.which("brew") is None:
            log("Homebrew is missing. Please install Homebrew first.", "error")
            sys.exit(1)
        run_cmd(["brew", "install", "libusb"])

    else:
        log(f"Unsupported OS: {os_type}", "error")
        sys.exit(1)

    # Create virtual environment
    if not os.path.exists(VENV_DIR):
        log(f"Creating virtual environment in {VENV_DIR}...")
        run_cmd([sys.executable, "-m", "venv", VENV_DIR])

    # Install Bumble
    log("Installing/updating Bumble framework...")
    run_cmd([PIP_BIN, "install", "--upgrade", "pip"])
    run_cmd([PIP_BIN, "install", "bumble[usb,ui,pcap]"])

# --- 2. Hardware preparation and cleanup ---


def clean_socket():
    """Remove stale socket file to avoid address conflicts"""
    if os.path.exists(SOCKET_PATH):
        log(f"Removing old socket: {SOCKET_PATH}")
        try:
            os.remove(SOCKET_PATH)
        except PermissionError:
            run_cmd(f"rm {SOCKET_PATH}", shell=True, sudo=True)


def stop_bluetooth_services():
    run_cmd("systemctl stop bluetooth.service bluetooth.socket >/dev/null 2>&1",
            sudo=True, shell=True, check=False)
    run_cmd("systemctl stop bluetooth >/dev/null 2>&1",
            sudo=True, shell=True, check=False)
    run_cmd("pkill -f bluetoothd >/dev/null 2>&1",
            sudo=True, shell=True, check=False)


def prepare_linux_hci(hci_index):
    """Linux: prepare HCI adapter for Bumble"""
    log(f"Preparing Linux Bluetooth adapter (hci{hci_index})...")
    run_cmd("rfkill unblock bluetooth", sudo=True, shell=True, check=False)
    stop_bluetooth_services()
    time.sleep(1)
    run_cmd(f"hciconfig hci{hci_index} down",
            sudo=True, shell=True, check=False)
    run_cmd(f"hciconfig hci{hci_index} reset",
            sudo=True, shell=True, check=False)
    run_cmd(f"hciconfig hci{hci_index} down",
            sudo=True, shell=True, check=False)
    log(f"Adapter hci{hci_index} is ready (DOWN state).", "success")


def restore_linux_hci(hci_index):
    """Linux: restore system Bluetooth state"""
    log("Restoring system Bluetooth settings...")
    run_cmd(f"hciconfig hci{hci_index} up", sudo=True, shell=True, check=False)
    run_cmd("systemctl start bluetooth.service bluetooth.socket >/dev/null 2>&1",
            sudo=True, shell=True, check=False)
    run_cmd("systemctl start bluetooth >/dev/null 2>&1",
            sudo=True, shell=True, check=False)
    log("System Bluetooth restored.", "success")

# --- 3. Core runtime ---


def run_bridge_linux(hci_index):
    check_linux_tools()
    ensure_sudo_available()
    check_hci_available(hci_index)
    ensure_runtime_ready()
    prepare_linux_hci(hci_index)
    clean_socket()

    bridge_cmd = [
        "sudo", PYTHON_BIN, "-m", "bumble.apps.hci_bridge",
        f"hci-socket:{hci_index}",
        f"unix-server:{SOCKET_PATH}"
    ]

    log(f"Starting Bumble Bridge on {SOCKET_PATH}...", "success")
    log("Press Ctrl+C to stop and restore system Bluetooth.")

    try:
        bridge_proc = subprocess.Popen(bridge_cmd, start_new_session=True)
        apply_socket_permissions()
        bridge_proc.wait()
    except KeyboardInterrupt:
        log("Stopping...", "warn")
        bridge_proc.terminate()
        try:
            bridge_proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            bridge_proc.kill()
            bridge_proc.wait()
    finally:
        restore_linux_hci(hci_index)
        clean_socket()


def run_bridge_macos(vid, pid):
    if not vid or not pid:
        log("For macOS, you must provide --vid and --pid.", "error")
        log("Run 'system_profiler SPUSBDataType' to find your USB Bluetooth dongle IDs.", "warn")
        sys.exit(1)
    ensure_runtime_ready()

    clean_socket()

    usb_device = f"usb:{vid}:{pid}"

    module_check = subprocess.run(
        [
            PYTHON_BIN,
            "-c",
            "import importlib.util, sys; sys.exit(0 if importlib.util.find_spec('bumble.apps.usb_hci_bridge') else 1)"
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )
    if module_check.returncode == 0:
        bridge_cmd = [
            PYTHON_BIN, "-m", "bumble.apps.usb_hci_bridge",
            usb_device,
            f"unix-server:{SOCKET_PATH}"
        ]
    else:
        log("bumble.apps.usb_hci_bridge not found, falling back to bumble.apps.hci_bridge.", "warn")
        bridge_cmd = [
            PYTHON_BIN, "-m", "bumble.apps.hci_bridge",
            f"unix-server:{SOCKET_PATH}",
            usb_device
        ]

    log(f"Starting Bumble Bridge for device {usb_device}...", "success")
    log(f"Listening on: {SOCKET_PATH}", "success")
    log("If connection fails, try unplugging/replugging the dongle.", "warn")

    try:
        bridge_proc = subprocess.Popen(bridge_cmd, start_new_session=True)
        apply_socket_permissions()
        bridge_proc.wait()
    except KeyboardInterrupt:
        log("Stopping...", "warn")
        bridge_proc.terminate()
        try:
            bridge_proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            bridge_proc.kill()
            bridge_proc.wait()
    finally:
        clean_socket()


# --- Main entrypoint ---


def main():
    parser = argparse.ArgumentParser(
        description="Bumble Bluetooth Bridge Setup")
    parser.add_argument(
        "--vid", help="USB Vendor ID (Required for macOS, e.g., 0x0a12)")
    parser.add_argument(
        "--pid", help="USB Product ID (Required for macOS, e.g., 0x0001)")
    parser.add_argument("--skip-install", action="store_true",
                        help="Skip dependency installation")
    parser.add_argument("--socket-path", default=SOCKET_PATH,
                        help="Unix socket path for the bridge")
    parser.add_argument("--hci-index", type=int, default=0,
                        help="Linux HCI index, e.g., 0 for hci0")
    args = parser.parse_args()

    set_socket_path(args.socket_path)

    if not args.skip_install:
        install_dependencies()

    os_type = platform.system()
    if os_type == "Linux":
        run_bridge_linux(args.hci_index)
    elif os_type == "Darwin":
        run_bridge_macos(args.vid, args.pid)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass

#!/usr/bin/env python3
"""
Laptop Telemetry Sender for ESP32 OLED Monitor (USB Serial Version)

Sends system stats directly over USB serial to the ESP32.

Requirements:
    pip install psutil pyserial GPUtil

Usage:
    python telemetry_serial.py

The script will auto-detect the ESP32 COM port.
"""

import json
import time
import sys
import serial
import serial.tools.list_ports

# Try to import dependencies
try:
    import psutil
except ImportError:
    print("ERROR: psutil not installed. Run: pip install psutil")
    sys.exit(1)

try:
    import GPUtil
    GPU_AVAILABLE = True
except ImportError:
    print("WARNING: GPUtil not installed. GPU stats will not be available.")
    print("Install with: pip install GPUtil")
    GPU_AVAILABLE = False

# ============================================
# SERIAL PORT DETECTION
# ============================================

def find_esp32_port():
    """Find the ESP32-S3 serial port."""
    ports = serial.tools.list_ports.comports()

    for port in ports:
        # ESP32-S3 USB CDC typically shows as these
        if "USB" in port.description or "CP210" in port.description or "CH340" in port.description:
            return port.device
        if "ESP" in port.description.upper():
            return port.device
        # Windows COM ports
        if "COM" in port.device:
            # Check for USB Serial device
            if port.vid is not None:
                return port.device

    # If nothing specific found, list all and let user choose
    if ports:
        print("\nAvailable ports:")
        for i, port in enumerate(ports):
            print(f"  [{i}] {port.device} - {port.description}")

        try:
            choice = int(input("\nSelect port number: "))
            return ports[choice].device
        except (ValueError, IndexError):
            pass

    return None


# ============================================
# NETWORK TRACKING
# ============================================

class NetworkTracker:
    def __init__(self):
        self.last_bytes_sent = 0
        self.last_bytes_recv = 0
        self.last_time = time.time()
        self.upload_speed = 0.0
        self.download_speed = 0.0
        net = psutil.net_io_counters()
        self.last_bytes_sent = net.bytes_sent
        self.last_bytes_recv = net.bytes_recv

    def update(self):
        current_time = time.time()
        elapsed = current_time - self.last_time
        if elapsed < 0.1:
            return (self.upload_speed, self.download_speed)

        net = psutil.net_io_counters()
        bytes_sent_delta = net.bytes_sent - self.last_bytes_sent
        bytes_recv_delta = net.bytes_recv - self.last_bytes_recv

        self.upload_speed = (bytes_sent_delta / elapsed) / (1024 * 1024)
        self.download_speed = (bytes_recv_delta / elapsed) / (1024 * 1024)

        self.last_bytes_sent = net.bytes_sent
        self.last_bytes_recv = net.bytes_recv
        self.last_time = current_time

        return (self.upload_speed, self.download_speed)


class DiskTracker:
    def __init__(self):
        self.last_read_bytes = 0
        self.last_write_bytes = 0
        self.last_time = time.time()
        self.read_speed = 0.0
        self.write_speed = 0.0
        disk = psutil.disk_io_counters()
        if disk:
            self.last_read_bytes = disk.read_bytes
            self.last_write_bytes = disk.write_bytes

    def update(self):
        current_time = time.time()
        elapsed = current_time - self.last_time
        if elapsed < 0.1:
            return (self.read_speed, self.write_speed)

        disk = psutil.disk_io_counters()
        if not disk:
            return (0.0, 0.0)

        read_delta = disk.read_bytes - self.last_read_bytes
        write_delta = disk.write_bytes - self.last_write_bytes

        self.read_speed = (read_delta / elapsed) / (1024 * 1024)
        self.write_speed = (write_delta / elapsed) / (1024 * 1024)

        self.last_read_bytes = disk.read_bytes
        self.last_write_bytes = disk.write_bytes
        self.last_time = current_time

        return (self.read_speed, self.write_speed)


# ============================================
# TELEMETRY COLLECTION
# ============================================

network_tracker = NetworkTracker()
disk_tracker = DiskTracker()


def get_cpu_info():
    cpu_percent = psutil.cpu_percent(interval=None)
    cpu_freq = psutil.cpu_freq()

    temp = 0.0
    try:
        temps = psutil.sensors_temperatures()
        if temps:
            for name in ['coretemp', 'cpu_thermal', 'k10temp', 'zenpower']:
                if name in temps and temps[name]:
                    temp = temps[name][0].current
                    break
            if temp == 0.0:
                for sensor_list in temps.values():
                    if sensor_list:
                        temp = sensor_list[0].current
                        break
    except Exception:
        pass

    return {
        "usage": round(cpu_percent, 1),
        "temp": round(temp, 1),
        "freq": round(cpu_freq.current / 1000, 2) if cpu_freq else 0.0,
        "cores": psutil.cpu_count(logical=True)
    }


def get_ram_info():
    mem = psutil.virtual_memory()
    swap = psutil.swap_memory()

    return {
        "usage": round(mem.percent, 1),
        "used": round(mem.used / (1024**3), 2),
        "total": round(mem.total / (1024**3), 2),
        "swap_used": round(swap.used / (1024**3), 2),
        "swap_total": round(swap.total / (1024**3), 2)
    }


def get_gpu_info():
    if not GPU_AVAILABLE:
        return {
            "usage": 0.0,
            "temp": 0.0,
            "vram_used": 0.0,
            "vram_total": 0.0,
            "name": "N/A"
        }

    try:
        gpus = GPUtil.getGPUs()
        if gpus:
            gpu = gpus[0]
            return {
                "usage": round(gpu.load * 100, 1),
                "temp": round(gpu.temperature, 1),
                "vram_used": round(gpu.memoryUsed / 1024, 2),
                "vram_total": round(gpu.memoryTotal / 1024, 2),
                "name": gpu.name[:31]
            }
    except Exception:
        pass

    return {
        "usage": 0.0,
        "temp": 0.0,
        "vram_used": 0.0,
        "vram_total": 0.0,
        "name": "Unknown"
    }


def get_network_info():
    upload, download = network_tracker.update()
    net = psutil.net_io_counters()

    return {
        "upload": round(upload, 2),
        "download": round(download, 2),
        "total_upload": round(net.bytes_sent / (1024**3), 2),
        "total_download": round(net.bytes_recv / (1024**3), 2)
    }


def get_disk_info():
    read_speed, write_speed = disk_tracker.update()
    disk_usage = psutil.disk_usage('/')

    return {
        "read": round(read_speed, 1),
        "write": round(write_speed, 1),
        "usage": round(disk_usage.percent, 1),
        "used": round(disk_usage.used / (1024**3), 1),
        "total": round(disk_usage.total / (1024**3), 1)
    }


def get_all_telemetry():
    return {
        "cpu": get_cpu_info(),
        "ram": get_ram_info(),
        "gpu": get_gpu_info(),
        "network": get_network_info(),
        "disk": get_disk_info()
    }


# ============================================
# MAIN
# ============================================

def main():
    print("\n" + "=" * 50)
    print("  LAPTOP TELEMETRY SENDER (USB Serial)")
    print("=" * 50)

    # Find ESP32 port
    port = find_esp32_port()
    if not port:
        print("\nERROR: Could not find ESP32 serial port!")
        print("Make sure the ESP32 is connected via USB.")
        sys.exit(1)

    print(f"\n  Connecting to {port}...")

    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for ESP32 to reset
        print(f"  Connected!")
        print("\n  Sending telemetry data...")
        print("  Press Ctrl+C to stop")
        print("=" * 50 + "\n")

        # Initialize CPU percent tracker
        psutil.cpu_percent(interval=None)

        while True:
            try:
                # Collect telemetry
                data = get_all_telemetry()

                # Send as JSON line
                json_line = json.dumps(data) + "\n"
                ser.write(json_line.encode())

                # Read response
                if ser.in_waiting:
                    response = ser.readline().decode().strip()
                    if response == "OK":
                        print(f"CPU: {data['cpu']['usage']:5.1f}%  "
                              f"RAM: {data['ram']['usage']:5.1f}%  "
                              f"GPU: {data['gpu']['usage']:5.1f}%  "
                              f"NET: {data['network']['download']:5.1f} MB/s", end='\r')

                time.sleep(1)

            except serial.SerialException as e:
                print(f"\nSerial error: {e}")
                print("Reconnecting...")
                time.sleep(2)
                try:
                    ser.close()
                    ser = serial.Serial(port, 115200, timeout=1)
                    time.sleep(2)
                except Exception:
                    pass

    except KeyboardInterrupt:
        print("\n\nStopped by user.")
    except serial.SerialException as e:
        print(f"\nERROR: Could not open {port}")
        print(f"  {e}")
        print("\nMake sure:")
        print("  - ESP32 is connected via USB")
        print("  - No other program is using the port (close Serial Monitor)")
    finally:
        try:
            ser.close()
        except Exception:
            pass


if __name__ == "__main__":
    main()

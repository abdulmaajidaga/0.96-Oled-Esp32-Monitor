#!/usr/bin/env python3
"""
Laptop Telemetry Server for ESP32 OLED Monitor
Run this script on your laptop to serve system stats over HTTP.

Requirements:
    pip install psutil flask GPUtil

Usage:
    python telemetry_server.py

The server will start on port 5000 by default.
Update the ESP32 config.h with your laptop's IP address.
"""

import json
import time
import socket
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from typing import Dict, Any

# Try to import optional dependencies
try:
    import psutil
except ImportError:
    print("ERROR: psutil not installed. Run: pip install psutil")
    exit(1)

try:
    import GPUtil
    GPU_AVAILABLE = True
except ImportError:
    print("WARNING: GPUtil not installed. GPU stats will not be available.")
    print("Install with: pip install GPUtil")
    GPU_AVAILABLE = False

# ============================================
# CONFIGURATION
# ============================================

HOST = "0.0.0.0"  # Listen on all interfaces
PORT = 5000

# ============================================
# NETWORK TRACKING
# ============================================

class NetworkTracker:
    """Track network speeds by calculating deltas."""

    def __init__(self):
        self.last_bytes_sent = 0
        self.last_bytes_recv = 0
        self.last_time = time.time()
        self.upload_speed = 0.0
        self.download_speed = 0.0

        # Initialize with current values
        net = psutil.net_io_counters()
        self.last_bytes_sent = net.bytes_sent
        self.last_bytes_recv = net.bytes_recv

    def update(self) -> tuple:
        """Update and return (upload_MBps, download_MBps)."""
        current_time = time.time()
        elapsed = current_time - self.last_time

        if elapsed < 0.1:  # Avoid division by very small numbers
            return (self.upload_speed, self.download_speed)

        net = psutil.net_io_counters()

        bytes_sent_delta = net.bytes_sent - self.last_bytes_sent
        bytes_recv_delta = net.bytes_recv - self.last_bytes_recv

        # Convert to MB/s
        self.upload_speed = (bytes_sent_delta / elapsed) / (1024 * 1024)
        self.download_speed = (bytes_recv_delta / elapsed) / (1024 * 1024)

        # Update tracking
        self.last_bytes_sent = net.bytes_sent
        self.last_bytes_recv = net.bytes_recv
        self.last_time = current_time

        return (self.upload_speed, self.download_speed)


class DiskTracker:
    """Track disk I/O speeds by calculating deltas."""

    def __init__(self):
        self.last_read_bytes = 0
        self.last_write_bytes = 0
        self.last_time = time.time()
        self.read_speed = 0.0
        self.write_speed = 0.0

        # Initialize with current values
        disk = psutil.disk_io_counters()
        if disk:
            self.last_read_bytes = disk.read_bytes
            self.last_write_bytes = disk.write_bytes

    def update(self) -> tuple:
        """Update and return (read_MBps, write_MBps)."""
        current_time = time.time()
        elapsed = current_time - self.last_time

        if elapsed < 0.1:
            return (self.read_speed, self.write_speed)

        disk = psutil.disk_io_counters()
        if not disk:
            return (0.0, 0.0)

        read_delta = disk.read_bytes - self.last_read_bytes
        write_delta = disk.write_bytes - self.last_write_bytes

        # Convert to MB/s
        self.read_speed = (read_delta / elapsed) / (1024 * 1024)
        self.write_speed = (write_delta / elapsed) / (1024 * 1024)

        # Update tracking
        self.last_read_bytes = disk.read_bytes
        self.last_write_bytes = disk.write_bytes
        self.last_time = current_time

        return (self.read_speed, self.write_speed)


# ============================================
# TELEMETRY COLLECTION
# ============================================

network_tracker = NetworkTracker()
disk_tracker = DiskTracker()


def get_cpu_info() -> Dict[str, Any]:
    """Get CPU statistics."""
    cpu_percent = psutil.cpu_percent(interval=None)

    # Get current frequency - just use what psutil reports
    freq_ghz = 0.0
    try:
        cpu_freq = psutil.cpu_freq()
        if cpu_freq and cpu_freq.current:
            freq_ghz = cpu_freq.current / 1000
    except:
        pass

    return {
        "usage": round(cpu_percent, 1),
        "freq": round(freq_ghz, 2),
        "cores": psutil.cpu_count(logical=True)
    }


def get_ram_info() -> Dict[str, Any]:
    """Get RAM statistics."""
    mem = psutil.virtual_memory()
    swap = psutil.swap_memory()

    return {
        "usage": round(mem.percent, 1),
        "used": round(mem.used / (1024**3), 2),
        "total": round(mem.total / (1024**3), 2),
        "swap_used": round(swap.used / (1024**3), 2),
        "swap_total": round(swap.total / (1024**3), 2)
    }


def get_gpu_info() -> Dict[str, Any]:
    """Get GPU statistics (requires GPUtil)."""
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
            gpu = gpus[0]  # Use first GPU
            return {
                "usage": round(gpu.load * 100, 1),
                "temp": round(gpu.temperature, 1),
                "vram_used": round(gpu.memoryUsed / 1024, 2),
                "vram_total": round(gpu.memoryTotal / 1024, 2),
                "name": gpu.name[:31]  # Truncate to fit ESP32 buffer
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


def get_network_info() -> Dict[str, Any]:
    """Get network statistics."""
    upload, download = network_tracker.update()
    net = psutil.net_io_counters()

    return {
        "upload": round(upload, 2),
        "download": round(download, 2),
        "total_upload": round(net.bytes_sent / (1024**3), 2),
        "total_download": round(net.bytes_recv / (1024**3), 2)
    }


def get_disk_info() -> Dict[str, Any]:
    """Get disk statistics like Task Manager shows."""
    read_speed, write_speed = disk_tracker.update()

    # Get disk usage of main partition
    disk_usage = psutil.disk_usage('/')

    # Calculate disk active time (like Task Manager)
    # This is based on busy_time from disk_io_counters
    try:
        io = psutil.disk_io_counters()
        # Active time approximation based on read/write activity
        # If there's any read/write happening, show activity %
        total_speed = read_speed + write_speed
        # Scale: 0 MB/s = 0%, 100 MB/s = 100%
        active_percent = min(100, total_speed * 2)  # 50 MB/s = 100%
    except:
        active_percent = 0.0

    return {
        "read": round(read_speed, 1),
        "write": round(write_speed, 1),
        "active": round(active_percent, 1),  # Active time like Task Manager
        "usage": round(disk_usage.percent, 1),
        "used": round(disk_usage.used / (1024**3), 1),
        "total": round(disk_usage.total / (1024**3), 1)
    }


def get_all_telemetry() -> Dict[str, Any]:
    """Collect all telemetry data."""
    return {
        "cpu": get_cpu_info(),
        "ram": get_ram_info(),
        "gpu": get_gpu_info(),
        "network": get_network_info(),
        "disk": get_disk_info(),
        "timestamp": int(time.time())
    }


# ============================================
# HTTP SERVER
# ============================================

class TelemetryHandler(BaseHTTPRequestHandler):
    """HTTP request handler for telemetry endpoint."""

    def log_message(self, format, *args):
        """Suppress default logging."""
        pass

    def do_GET(self):
        """Handle GET requests."""
        if self.path == '/telemetry' or self.path == '/':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()

            data = get_all_telemetry()
            self.wfile.write(json.dumps(data).encode())

        elif self.path == '/health':
            self.send_response(200)
            self.send_header('Content-Type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'OK')

        else:
            self.send_response(404)
            self.end_headers()


def get_local_ip() -> str:
    """Get the local IP address of this machine."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"


def run_server():
    """Start the HTTP server."""
    server = HTTPServer((HOST, PORT), TelemetryHandler)

    local_ip = get_local_ip()

    print("\n" + "=" * 50)
    print("  LAPTOP TELEMETRY SERVER")
    print("=" * 50)
    print(f"\n  Server running on port {PORT}")
    print(f"\n  Update ESP32 config.h with:")
    print(f"    #define TELEMETRY_HOST \"{local_ip}\"")
    print(f"\n  Endpoints:")
    print(f"    http://{local_ip}:{PORT}/telemetry")
    print(f"    http://{local_ip}:{PORT}/health")
    print("\n  Press Ctrl+C to stop")
    print("=" * 50 + "\n")

    # Initialize CPU percent tracker (first call is always 0)
    psutil.cpu_percent(interval=None)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.shutdown()


if __name__ == "__main__":
    run_server()

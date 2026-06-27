# meta-telemetry: Yocto Layer for Raspberry Pi 5 WebSocket Telemetry

A custom Yocto Project meta-layer deploying a Qt6 C++ daemon and a lightweight web presentation layer to surface real-time operating system and application runtime metrics over persistent WebSockets.

## ⚙️ Systems Architecture & Data Plane

This layer establishes a strictly decoupled, event-driven telemetry pipeline optimized for low-latency embedded monitoring:

`Browser Client (dashboard.html) <--> WebSockets (Port 8080) <--> Qt6 C++ Daemon <--> Physical Layer (GPIO 14)`

* **Event-Driven IPC:** Replaces heavy polling loops with an asynchronous WebSocket server running on port 8080. Telemetry payloads are encoded into JSON frames and pushed natively to connected clients.
* **Decoupled Presentation Layer:** A Lighttpd web server operates on port 80 to deliver a static, modern, responsive CSS Grid dashboard. The client application utilizes dynamic host resolution (`window.location.hostname`) to bind to the socket bridge automatically under changing DHCP conditions.
* **Hardware Interactivity:** Implements bidirectional communication, allowing clients to send control frames down the socket to safely trigger physical hardware state changes on GPIO 14.

## 🛠️ Tech Stack

* **Build System:** Yocto Project, BitBake (Scaffolded via `core-image-minimal`)
* **Hardware Target:** Raspberry Pi 5 (`aarch64`)
* **Core Backend:** C++17, Qt6 Base Core/Network
* **Web Serving:** Lighttpd (Minimal footprint, CGI modules stripped)
* **Frontend UI:** HTML5, CSS3 (Modern dark-mode grid), native JavaScript ES6 WebSockets
* **Process Management:** systemd (Automated lifecycle handling)

## 🚀 Build and Deployment Instructions

**1. Clone the Layer**
Navigate to your Yocto `sources/` directory and pull this repository:
```bash
cd ~/rpi-yocto/sources/
git clone [https://github.com/](https://github.com/)<YOUR_GITHUB_USERNAME>/meta-telemetry.git
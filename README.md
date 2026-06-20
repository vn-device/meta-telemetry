# meta-telemetry: Yocto Layer for Raspberry Pi 5 SNMPv3 Telemetry

A custom Yocto Project meta-layer deploying a C-based AgentX IPC subagent to surface operating system and application runtime metrics to a Net-SNMP master daemon via the IPv4 loopback interface.

## ⚙️ Systems Architecture & Data Plane

This Board Support Package (BSP) extension establishes a strictly decoupled telemetry pipeline optimized for embedded Linux environments:

`Net-SNMP Master Daemon <--> Unix Domain Socket (/var/agentx/master) <--> C Subagent <--> Kernel Loopback (127.0.0.1)`

* **Data Plane Isolation:** Polling operates entirely at the OS kernel level via `127.0.0.1`, guaranteeing zero physical network latency and air-gapped IPC reliability even if the physical NIC (`192.168.1.x`) drops its DHCP lease.
* **AgentX IPC Bridge:** Bypasses legacy MIB compilation by utilizing the AgentX protocol. The master daemon delegates the `.1.3.6.1.4.1.99999` enterprise tree to the custom C binary, which autonomously registers and broadcasts `ASN_INTEGER` payloads.

## 🛠️ Tech Stack

* **Build System:** Yocto Project, BitBake
* **Hardware Target:** Raspberry Pi 5 (`aarch64`)
* **Systems Programming:** C, standard POSIX APIs
* **Networking & IPC:** Net-SNMP (AgentX Protocol), IPv4 Loopback routing
* **Process Management:** systemd (Scaffolded)

## 🚀 Build Instructions

This layer is designed to be appended to a standard `core-image-minimal` target.

**1. Clone the Layer**
Navigate to your Yocto `sources/` directory and pull this repository:
```bash
cd sources/
git clone [https://github.com/](https://github.com/)<YOUR_GITHUB_USERNAME>/meta-telemetry.git
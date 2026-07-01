# System Architecture

XCAM is the camera edge node inside the MKME X stack. It does not replace XTOC, XCOM, XNODE, an NVR, or a hardened commercial camera system. It gives the local network small camera endpoints that XTOC can view, label, and operationalize.

## Logical layers

| Layer | Products | Responsibility |
| --- | --- | --- |
| Command and shared picture | XTOC | Video ISR, packet archive, map overlays, Sentinel ingest, shared operational context, and bridge workflows. |
| Camera edge | XCAM | ESP32 camera capture, RTSP, MJPEG, snapshot, browser setup, local status, and optional Sentinel motion alerts. |
| Field operator software | XCOM | Offline field packet, map, radio, and import/export workflows. |
| Wearable/handheld endpoint | XNODE | Watch/T-Deck alerts, maps, GPS, mesh, SOS, CheckIn, and synced operator context. |
| Passive intel and analysis | XINTEL, XCORE | Local radio-intel monitoring and offline analysis around the same operational data. |

## Video data flow

The common XCAM video flow is:

```text
Camera sensor
  -> ESP32 camera driver
  -> XCAM firmware
  -> RTSP, MJPEG, or snapshot URL
  -> trusted LAN or field LAN
  -> XTOC Video ISR tile or compatible local viewer
```

The operator-facing URLs are:

```text
rtsp://<thing-name>.local:554/mjpeg/1
http://<thing-name>.local/stream
http://<thing-name>.local/snapshot
```

The status page also exposes IPv4 versions of each URL for networks where `.local` name resolution is unavailable.

## Motion-alert data flow

When enabled, motion alerts follow this flow:

```text
Camera frame
  -> 8x8 visual digest
  -> changed-percent score
  -> threshold and consecutive-hit check
  -> XTOC T=11 SENTINEL packet text
  -> HTTP POST to local MANET bridge /send endpoint
  -> XTOC Sentinel ingest
```

The bridge URL is configured in the browser setup page. A typical local target is:

```text
http://<xtoc-laptop-lan-ip>:8095/send
```

XCAM normalizes the configured bridge URL by trimming trailing slashes and appending `/send` when needed.

## Device identity model

Each device starts with a unique default thing name:

```text
xcam-<six-hex-character-suffix>
```

Operators should rename cameras to the physical location or role the TOC will use, for example:

```text
xcam-front-gate
xcam-yard-north
xcam-drone-bench
xcam-radio-room
```

That name drives the local hostname and the URL labels operators copy into XTOC.

## Runtime model

The firmware is built on:

- PlatformIO with Arduino ESP32.
- IotWebConf for provisioning, configuration storage, captive portal behavior, and config forms.
- ESP32 camera driver through the `OV2640` camera wrapper.
- Micro-RTSP for RTSP serving.
- `WebServer` routes for status, config, stream, snapshot, restart, and optional flash LED control.
- mDNS for friendly local discovery.
- Embedded status HTML from `html/index.min.html`.

## Current implementation anchors

- PlatformIO environments: `platformio.ini`
- Board definitions: `boards/*.json`
- Main firmware loop and routes: `src/main.cpp`
- Status page template: `html/index.html`
- Motion-alert packet helpers: `include/motion_alert.h`
- Host regression test: `test/motion_alert_host_test.cpp`
- Host test runner: `scripts/run-host-tests.py`

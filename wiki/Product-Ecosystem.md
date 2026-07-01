# Product Ecosystem

The MKME X stack is a local-first operational toolkit. XTOC, XCOM, XNODE, XCAM, XINTEL, and XCORE each solve a different part of the same problem: moving observations, reports, alerts, maps, camera feeds, and analysis through controlled local workflows when normal infrastructure is unreliable or unavailable.

## XTOC

XTOC is the Tactical Operations Center software. In the XCAM workflow, it is the operator-facing Video ISR board and the operational record around the camera view.

XCAM supports XTOC by providing:

- Fixed or portable LAN camera sources.
- Stable RTSP, MJPEG, and snapshot URLs.
- Human-readable camera names such as `xcam-front-gate` or `xcam-yard-north`.
- IPv4 fallback URLs for networks where `.local` discovery is unreliable.
- Optional camera-frame motion alerts that post `T=11 SENTINEL` packet text to the local MANET bridge.

## XCAM

XCAM is the firmware in this repository. It runs on ESP32-CAM class hardware and exposes a small camera node that is easy to provision, name, test, and hand to XTOC.

XCAM handles:

- Temporary setup access point with a unique default identity.
- Browser-based configuration for WiFi, thing name, board type, camera tuning, and motion alerts.
- Local status page with diagnostics, camera settings, motion-alert status, and copyable URLs.
- RTSP camera stream on port `554`.
- HTTP MJPEG stream at `/stream`.
- HTTP JPEG snapshot at `/snapshot`.
- Optional flash LED control on boards that define `FLASH_LED_GPIO`.
- Optional XTOC Sentinel motion-alert POSTs to a local bridge URL.

## XCOM

XCOM is the field-side offline radio and mapping suite. XCAM does not require XCOM to operate, but XCAM feed labels and Sentinel alerts should use the same naming discipline as XCOM/XTOC packet workflows: clear Unit IDs, physical locations, and operationally meaningful labels.

## XNODE

XNODE is the wearable or handheld edge node for maps, GPS, mesh, alerts, SOS, and check-in flows. XCAM complements XNODE because a team can carry the high-value map and alert subset on the operator while fixed XCAM nodes keep local video on the TOC board.

## XINTEL

XINTEL is the local radio intelligence monitor. It complements XCAM by turning legally receivable audio into structured events while XCAM turns camera views and visual change into local video and optional Sentinel events.

## XCORE

XCORE is the local tactical AI analyst. XCAM does not depend on XCORE, but the camera-derived operational picture can sit beside the packet, alert, map, and radio-intel data that XCORE may summarize or reason over.

## Why the stack works together

The design is not centered on one perfect network. The common pattern is structured, locally controlled operational data. XCAM adds the video edge: small camera nodes that can be placed, named, verified, and viewed without a vendor camera cloud. XTOC remains the command surface; XCAM supplies camera sources and optional visual-change alerts.

## Sources

- XCAM GitHub repo: https://github.com/MKme/xcam
- XTOC Store page: https://store.mkme.org/product/xtoc-tactical-operations-center-software-suite/
- XCAM README: `README.md`
- XCAM firmware entrypoint: `src/main.cpp`

# XCAM Wiki

XCAM is the MKME LAN-ready ESP32 camera node for XTOC Video ISR. It turns ESP32-CAM class hardware into a local RTSP, MJPEG, and snapshot endpoint with browser setup, location-friendly naming, status-page URLs, and optional camera-frame motion alerts for XTOC Sentinel ingest.

At the system level:

- XTOC is the offline command center and operator-facing Video ISR surface.
- XCAM is the camera edge node that puts fixed, portable, or helmet-mounted camera views on the local network.
- XCOM is the offline field radio and mapping companion for packet and operational workflows.
- XNODE is the wearable or handheld edge device for alerts, maps, GPS, mesh, SOS, and check-in flows.
- XINTEL and XCORE add local radio intelligence monitoring and offline analysis around the same operational picture.

This directory is the source for the repository wiki. The Markdown files are written in GitHub Wiki style so they can be published directly to the GitHub wiki when the special `MKme/xcam.wiki.git` repo is initialized.

## Pages

- [Product Ecosystem](Product-Ecosystem): where XCAM fits in the MKME X stack.
- [System Architecture](System-Architecture): how camera, LAN, XTOC, and Sentinel data move.
- [XCAM Firmware](XCAM-Firmware): firmware features, routes, configuration, and status behavior.
- [Hardware Targets](Hardware-Targets): supported PlatformIO environments and board families.
- [LAN Video and XTOC ISR](LAN-Video-and-XTOC-ISR): RTSP, MJPEG, snapshot, naming, and XTOC setup workflow.
- [Motion Alerts and Sentinel Bridge](Motion-Alerts-and-Sentinel-Bridge): camera-frame motion detection and XTOC `T=11 SENTINEL` packets.
- [Build, Flash, Test, and CI](Build-Flash-Test-and-CI): repeatable build, flash, and host-test workflow.
- [Operations and Troubleshooting](Operations-and-Troubleshooting): field checks and common failure modes.
- [Publishing the GitHub Wiki](Publishing-the-GitHub-Wiki): how to publish these files to GitHub Wiki.

## Current firmware targets

| Target family | PlatformIO examples | Role |
| --- | --- | --- |
| AI-Thinker ESP32-CAM | `esp32cam_ai_thinker` | Default low-cost XCAM bring-up target with PSRAM and OV2640 camera support. |
| ESP32-S3 camera boards | `esp32cam_s3_wroom_n16r8`, `esp32cam_espressif_esp32s3_eye`, `esp32cam_espressif_esp32s3_cam_lcd` | Higher-resource S3 camera targets with PSRAM-oriented builds. |
| M5Stack camera devices | `esp32cam_m5stack_unitcam`, `esp32cam_m5stack_unitcams3`, `esp32cam_m5stack_m5poecam_w`, `m5stack-timer-cam` | Packaged camera modules for small fixed or PoE-style LAN video nodes. |
| Seeed and TTGO camera boards | `esp32cam_seeed_xiao_esp32s3_sense`, `esp32cam_ttgo_t_camera`, `esp32cam_ttgo_t_journal` | Compact ESP32 camera boards for lab, field kit, and prototype deployments. |

## Primary source references

- XCAM GitHub repo: https://github.com/MKme/xcam
- XTOC Store page: https://store.mkme.org/product/xtoc-tactical-operations-center-software-suite/
- In-repo sales page source: `site/xcam-sales-page-linked.html`
- In-repo WooCommerce copy: `site/xcam/woocommerce-product-xcam.txt`

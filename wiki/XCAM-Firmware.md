# XCAM Firmware

The XCAM firmware turns ESP32-CAM class boards into compact local camera nodes for XTOC Video ISR and compatible LAN viewers.

## Main surfaces

Current user-facing and integration surfaces include:

- Temporary setup WiFi access point named from the device's unique `xcam-<suffix>` identity.
- Browser status page at `http://<thing-name>.local/` or the device IPv4 address.
- Browser configuration page at `/config`.
- RTSP stream at `rtsp://<thing-name>.local:554/mjpeg/1`.
- HTTP Motion JPEG stream at `/stream`.
- HTTP JPEG snapshot at `/snapshot`.
- Restart route at `/restart`.
- Optional flash LED route at `/flash?v=<0-255>` on boards that define `FLASH_LED_GPIO`.
- Motion-alert status and configuration for XTOC Sentinel bridge posting.

## Provisioning behavior

On first boot or when WiFi is not configured, XCAM starts an access point using the device thing name. The setup flow is:

1. Connect to the temporary `xcam-<suffix>` WiFi network.
2. Open `http://192.168.4.1` if the captive portal does not open automatically.
3. Set WiFi, access password, board type, camera settings, and thing name.
4. Reboot the camera onto the permanent LAN.
5. Open the status page and copy the RTSP, MJPEG, or snapshot URL into XTOC.

The access password is used for protected configuration routes through basic authentication with user `admin`.

## Status page

The status page renders the embedded `html/index.min.html` template and exposes:

- App title and firmware version.
- Board type, SDK version, CPU, RAM, PSRAM, flash, and heap diagnostics.
- Uptime and RTSP session count.
- Hostname, mDNS name, MAC address, WiFi mode, SSID, signal strength, IPv4, and IPv6.
- Current camera frame size, frame duration, frame frequency, JPEG quality, and sensor tuning values.
- Camera initialization result and error text when initialization fails.
- Motion-alert configuration and runtime counters.
- Copyable RTSP, MJPEG, snapshot, and restart URLs with `.local` and IPv4 fallback forms.

## Camera controls

The browser configuration page exposes the current camera controls from `src/main.cpp`:

- Frame duration.
- Frame size from QQVGA through UXGA.
- JPEG quality.
- Brightness, contrast, and saturation.
- Special effect.
- White balance, AWB gain, and white-balance mode.
- Exposure control, DSP auto exposure, exposure level, and manual exposure value.
- Gain control, manual AGC gain, and gain ceiling.
- Black pixel correction, white pixel correction, gamma correction, and lens correction.
- Horizontal mirror, vertical flip, downsize enable, and color bar.

Default frame size is `VGA (640x480)`. Default JPEG quality is selected at runtime from PSRAM availability.

## Network services

XCAM advertises HTTP and RTSP locally:

- HTTP status/config/stream/snapshot service on port `80`.
- RTSP service on port `554` when the camera initializes successfully.
- mDNS name based on the configured thing name.

The device is intended for trusted LANs, field LANs, lab networks, MANET segments, or VPN-protected networks. The stream URLs should not be exposed directly to the public internet.

## Motion alerts

Motion alerts are disabled by default. When enabled and configured with a usable local bridge URL, the firmware samples camera frames, computes a compact visual digest, tracks change percent, and posts an XTOC `T=11 SENTINEL` packet when the configured thresholds are met.

See [Motion Alerts and Sentinel Bridge](Motion-Alerts-and-Sentinel-Bridge) for the packet and tuning details.

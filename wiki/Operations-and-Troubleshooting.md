# Operations and Troubleshooting

This page captures the checks that matter during field use, product setup, and firmware bring-up.

## First boot does not show a setup network

Expected setup SSID:

```text
xcam-<suffix>
```

If it does not appear:

- Confirm the board is powered from stable 5 V.
- Confirm the correct PlatformIO environment was flashed.
- Open serial monitor at `115200`.
- Check for boot loops, camera init failures, PSRAM errors, or brownout-like resets.
- Try a known-good USB cable and power source.

## Cannot open the setup page

Try:

```text
http://192.168.4.1
```

If captive portal behavior does not open automatically, connect to the XCAM setup SSID manually and browse to that IP.

## `.local` URL does not resolve

Use the IPv4 fallback URLs shown on the status page.

Common causes:

- Client device does not support mDNS.
- Router or VLAN does not forward multicast discovery.
- Camera and viewer are on different subnets.
- Hostname was changed but the client cached the old name.

For operational use, consider DHCP reservations for known XCAM units.

## Camera fails to initialize

Check:

- Correct PlatformIO environment for the physical board.
- Camera ribbon orientation and seating.
- Board-specific pin mapping in `boards/*.json`.
- PSRAM availability and whether the selected target expects PSRAM.
- Power stability during WiFi and camera startup.
- Frame size and JPEG quality settings.

If a board advertises PSRAM but behaves unreliably, use lower resolution and safer camera settings before assuming the firmware is broken.

## Stream is slow or unstable

Check:

- WiFi signal strength on the status page.
- Power supply quality.
- Frame size.
- Frame duration.
- JPEG quality.
- Number of active RTSP/MJPEG viewers.
- Whether the board has PSRAM and enough free heap.

ESP32-CAM class hardware is small. High resolution, high quality, multiple viewers, poor WiFi, and weak power can combine into dropped frames or resets.

## RTSP does not connect

Verify:

- Camera initialized successfully.
- RTSP URL is exactly `rtsp://<thing-name>.local:554/mjpeg/1` or the IPv4 equivalent.
- Client is on the same reachable network.
- Port `554` is not blocked between client and camera.
- `.local` resolution works, or use the IPv4 URL.

The RTSP server starts only after a successful camera initialization.

## MJPEG or snapshot works but XTOC does not show video

Use the URL type XTOC expects for that tile. If RTSP is failing but MJPEG works, test the RTSP URL with another local viewer and confirm the camera, XTOC browser, and XTOC host can all reach the same network.

## Motion alerts do not send

Check the status page:

- Motion alerts enabled.
- Bridge URL is configured and starts with `http://`.
- Normalized bridge URL ends in `/send`.
- Camera initialized.
- Network is online.
- Last send status and last error.
- Last motion score.
- Consecutive hits versus target.

If no alerts occur, lower the area threshold only after confirming samples are increasing and the camera view actually changes.

## Too many motion alerts

Increase one or more:

- Motion pixel delta.
- Area threshold.
- Consecutive hits.
- Cooldown.

Also check lighting, shadows, auto exposure shifts, moving branches, reflective surfaces, and vibration.

## Lost configuration password

The configuration user is always:

```text
admin
```

The password is the configured access point password. If it is lost, erase the ESP32 flash and reflash the firmware. This discards stored configuration.

## Field operating notes

- Name cameras by physical role before handing URLs to operators.
- Verify snapshot, MJPEG, and RTSP before leaving the staging area.
- Record IPv4 fallback URLs when mDNS is unreliable.
- Keep XCAM on trusted networks or VPN-protected segments.
- Do not treat motion alerts as a life-safety or certified intrusion alarm.
- Keep XTOC as the operator-facing record; XCAM is the camera edge.

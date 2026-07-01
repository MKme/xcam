# LAN Video and XTOC ISR

XCAM is built around a simple field workflow: place the camera, name it for the site, verify the feed locally, and add the right URL to XTOC Video ISR or another trusted LAN viewer.

## Camera URLs

Each configured XCAM exposes:

```text
rtsp://<thing-name>.local:554/mjpeg/1
http://<thing-name>.local/stream
http://<thing-name>.local/snapshot
```

The status page also shows IPv4 fallback URLs:

```text
rtsp://<ipv4-address>:554/mjpeg/1
http://<ipv4-address>/stream
http://<ipv4-address>/snapshot
```

Use IPv4 URLs when `.local` name resolution is not reliable on the network.

## Which URL to use

| URL type | Use when |
| --- | --- |
| RTSP | XTOC, an NVR, VLC, ffmpeg, or another video tool expects an RTSP camera source. |
| MJPEG stream | You want quick browser validation or a simple HTTP camera view. |
| Snapshot | A still image is enough for status, documentation, or a lightweight visual check. |

RTSP is the normal XTOC Video ISR path when a live tile is needed.

## Multi-camera naming

Use names that match the physical role the TOC will recognize:

```text
xcam-front-gate
xcam-yard-north
xcam-motorpool-2
xcam-drone-bench
xcam-radio-room
xcam-helmet-1
```

Avoid generic names such as `camera1` or `esp32cam` when more than one camera may be online. The thing name becomes the `.local` hostname and the label operators see while copying URLs.

## XTOC setup workflow

1. Flash the correct XCAM PlatformIO environment.
2. Power the camera with stable 5 V.
3. Connect to the temporary `xcam-<suffix>` setup access point.
4. Open the setup page and enter WiFi credentials.
5. Set the thing name to a physical location or role.
6. Select the board type and tune camera settings if needed.
7. Reboot onto the permanent LAN.
8. Open the status page.
9. Confirm that the snapshot or MJPEG stream works in a browser.
10. Copy the RTSP, MJPEG, or snapshot URL into XTOC Video ISR.

## Browser validation

Before adding a camera to XTOC, test:

- `http://<thing-name>.local/` opens the status page.
- `http://<thing-name>.local/snapshot` returns a fresh JPEG.
- `http://<thing-name>.local/stream` opens a live MJPEG stream.
- `rtsp://<thing-name>.local:554/mjpeg/1` opens in an RTSP-capable viewer if RTSP is the planned XTOC source.

If `.local` fails but IPv4 works, the network likely lacks reliable mDNS forwarding or local name resolution.

## LAN placement notes

XCAM is a local node, not a cloud camera. For field use:

- Keep cameras and XTOC on the same trusted LAN or routed field network.
- Use DHCP reservations or documented static leases when camera identity matters.
- Segment sensitive camera networks from guest or public networks.
- Avoid exposing XCAM directly to the public internet.
- Verify power, WiFi signal, and stream stability before relying on a camera in an operation.

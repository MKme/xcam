# Motion Alerts and Sentinel Bridge

XCAM can turn camera-frame change into an XTOC Sentinel alert. This is lightweight operational awareness, not a certified alarm system.

## What motion alerts do

When enabled, the firmware:

1. Samples camera frames at the configured interval.
2. Computes an 8x8 luma digest from the current JPEG frame.
3. Compares the digest to the previous baseline.
4. Calculates a changed-percent score.
5. Requires the configured area threshold and consecutive-hit count.
6. Enforces a cooldown period.
7. Builds an XTOC `T=11 SENTINEL` packet.
8. Posts that packet to the configured local MANET bridge URL.

Motion alerts are disabled by default and are automatically disabled if the stored bridge URL is invalid or empty.

## Bridge URL

The motion bridge must be an HTTP URL. HTTPS is not accepted by the current firmware check.

Typical local configuration:

```text
http://<xtoc-laptop-lan-ip>:8095/send
```

The firmware normalizes the URL:

- Leading and trailing whitespace is removed.
- Trailing slashes are removed.
- `/send` is appended when the configured URL does not already end with `/send`.

## Configurable settings

| Setting | Firmware parameter | Default | Range |
| --- | --- | --- | --- |
| Enable camera motion alerts | `me` | disabled | checkbox |
| XTOC MANET bridge URL | `mbu` | empty | text |
| Sentinel label | `ml` | empty | text |
| Sentinel node ID | `mnid` | empty | decimal, `0x` hex, or `!` hex |
| Sentinel latitude | `mlat` | `0.0` | `-90` to `90` |
| Sentinel longitude | `mlon` | `0.0` | `-180` to `180` |
| Sentinel motion sensor type | `mst` | `1` | `1` to `2` |
| Motion sample interval | `msi` | `1000` ms | `250` to `10000` ms |
| Motion pixel delta | `mdt` | `25` | `1` to `128` |
| Motion area threshold | `mat` | `8` percent | `1` to `100` percent |
| Motion consecutive hits | `mch` | `2` | `1` to `10` |
| Motion alert cooldown | `mcd` | `60000` ms | `1000` to `3600000` ms |
| Motion bridge timeout | `mto` | `2000` ms | `250` to `10000` ms |

## Labels and node IDs

If the Sentinel label is blank, XCAM uses the configured thing name. Labels are trimmed and capped at 32 bytes in the encoded payload.

If the Sentinel node ID is blank, XCAM uses a MAC-derived fallback. Node IDs can be entered as:

```text
3735928559
0xDEADBEEF
!deadbeef
```

Invalid node IDs prevent packet generation and appear on the status page as the last motion error.

## Packet body

XCAM posts JSON to the bridge:

```json
{
  "client": {
    "id": "xcam-<mac-derived-id>",
    "app": "xcom",
    "role": "client",
    "label": "<motion-label>"
  },
  "kind": "packet",
  "text": "<T=11 SENTINEL packet text>"
}
```

The packet text contains a generated XTOC packet ID and a base64url encoded Sentinel payload.

## Tuning guidance

Start conservative:

- Keep sample interval near `1000` ms.
- Keep pixel delta near `25`.
- Keep area threshold near `8` percent.
- Require at least `2` consecutive hits.
- Keep cooldown around `60000` ms until false positives are understood.

Increase area threshold or consecutive hits if lighting changes cause alerts. Decrease them only when the camera must catch smaller changes and the view is stable.

## Status fields

The status page reports:

- Enabled or disabled state.
- Normalized bridge URL.
- Label, configured node ID, and effective node ID.
- Location, sensor type, thresholds, cooldown, and timeout.
- Sample count and alert count.
- Last motion score.
- Last sample age.
- Last alert age.
- Last send result.
- Last error.

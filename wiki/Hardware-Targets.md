# Hardware Targets

XCAM builds many ESP32-CAM class targets from the same firmware tree. Each target is selected by a PlatformIO environment in `platformio.ini` and a matching board definition under `boards/`.

## Common build command

Build one target:

```powershell
pio run -e esp32cam_ai_thinker
```

Upload one target:

```powershell
pio run -e esp32cam_ai_thinker -t upload --upload-port COM10
```

Use the actual COM port Windows assigned to the attached board.

## Supported PlatformIO environments

| Environment | Board definition |
| --- | --- |
| `esp32cam_ai_thinker` | `boards/esp32cam_ai_thinker.json` |
| `esp32cam_espressif_esp_eye` | `boards/esp32cam_espressif_esp_eye.json` |
| `esp32cam_espressif_esp32s2_cam_board` | `boards/esp32cam_espressif_esp32s2_cam_board.json` |
| `esp32cam_espressif_esp32s2_cam_header` | `boards/esp32cam_espressif_esp32s2_cam_header.json` |
| `esp32cam_espressif_esp32s3_cam_lcd` | `boards/esp32cam_espressif_esp32s3_cam_lcd.json` |
| `esp32cam_espressif_esp32s3_eye` | `boards/esp32cam_espressif_esp32s3_eye.json` |
| `esp32cam_freenove_wrover_kit` | `boards/esp32cam_freenove_wrover_kit.json` |
| `esp32cam_m5stack_atoms3r` | `boards/esp32cam_m5stack_atoms3r.json` |
| `esp32cam_m5stack_camera_psram` | `boards/esp32cam_m5stack_camera_psram.json` |
| `esp32cam_m5stack_camera` | `boards/esp32cam_m5stack_camera.json` |
| `esp32cam_m5stack_esp32cam` | `boards/esp32cam_m5stack_esp32cam.json` |
| `esp32cam_m5stack_unitcam` | `boards/esp32cam_m5stack_unitcam.json` |
| `esp32cam_m5stack_unitcams3` | `boards/esp32cam_m5stack_unitcams3.json` |
| `esp32cam_m5stack_wide` | `boards/esp32cam_m5stack_wide.json` |
| `esp32cam_m5stack_m5poecam_w` | `boards/esp32cam_m5stack_m5poecam_w.json` |
| `esp32cam_seeed_xiao_esp32s3_sense` | `boards/esp32cam_seeed_xiao_esp32s3_sense.json` |
| `esp32cam_ttgo_t_camera` | `boards/esp32cam_ttgo_t_camera.json` |
| `esp32cam_ttgo_t_journal` | `boards/esp32cam_ttgo_t_journal.json` |
| `m5stack-timer-cam` | `boards/m5stack-timer-cam.json` |
| `esp32cam_s3_wroom_n16r8` | `boards/esp32cam_s3_wroom_n16r8.json` |

The `boards/` directory also contains `esp32cam_freenove_s3_wroom_n8r8.json`. It is a board definition present in the repo, but it is not currently wired to a named PlatformIO environment in `platformio.ini`.

## Practical starting targets

For a first XCAM bring-up, start with:

- `esp32cam_ai_thinker` for common ESP32-CAM modules with PSRAM.
- `esp32cam_s3_wroom_n16r8` for ESP32-S3 WROOM N16R8 camera hardware.
- `esp32cam_seeed_xiao_esp32s3_sense` for compact Seeed XIAO ESP32S3 Sense builds.
- `esp32cam_m5stack_unitcams3` or `esp32cam_m5stack_m5poecam_w` for M5Stack packaged camera devices.

## Hardware traits that matter

XCAM behavior is heavily affected by:

- Camera sensor wiring and board pin definitions.
- PSRAM availability and reliability.
- Flash size and partition layout.
- Power quality during WiFi transmit and camera startup.
- Antenna placement and local RF conditions.
- Whether the selected PlatformIO environment matches the physical board.

## PSRAM and frame buffers

Targets with PSRAM typically use a PSRAM frame buffer location and two frame buffers. Boards without reliable PSRAM may need lower frame sizes, fewer buffers, or PSRAM disabled in the configuration to avoid camera initialization failures.

Useful starting values:

- No PSRAM: SVGA or lower, one frame buffer, JPEG quality around `12` to `14`.
- With PSRAM: larger frame sizes are possible, but reliability still depends on board quality, power, and heat.

## Flash and status LEDs

Some board definitions include:

- `FLASH_LED_GPIO` for the optional `/flash?v=<0-255>` route.
- `USER_LED_GPIO` for status LED behavior.

If the flash route is missing, the selected board target probably does not define a flash LED GPIO.

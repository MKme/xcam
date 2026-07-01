# Build, Flash, Test, and CI

This repo is set up so all supported XCAM firmware targets can be built from one PlatformIO tree.

## Install prerequisites

The project uses:

- PlatformIO for firmware builds and uploads.
- Python for the host motion-alert tests.
- A C++ compiler such as `c++`, `g++`, or `clang++` for host tests.
- Arduino ESP32 through the PlatformIO `espressif32@6.9.0` platform.

The firmware libraries are declared in `platformio.ini`:

- `prampec/IotWebConf@^3.2.1`
- `geeksville/Micro-RTSP@^0.1.6`
- `rzeldent/micro-moustache@^1.0.1`

## Host tests

Run:

```powershell
python scripts/run-host-tests.py
```

This compiles and runs:

```text
test/motion_alert_host_test.cpp
```

The host test covers:

- base64url encoding.
- Sentinel payload encoding.
- decimal and hex node ID parsing.
- motion changed-percent scoring.
- digest segmentation.

## Build all configured targets

Run:

```powershell
pio run
```

This builds every active environment in `platformio.ini`.

## Build one target

Examples:

```powershell
pio run -e esp32cam_ai_thinker
pio run -e esp32cam_s3_wroom_n16r8
pio run -e esp32cam_seeed_xiao_esp32s3_sense
pio run -e esp32cam_m5stack_unitcams3
```

## Flash

AI-Thinker example:

```powershell
pio run -e esp32cam_ai_thinker -t upload --upload-port COM10
```

ESP32-S3 WROOM example:

```powershell
pio run -e esp32cam_s3_wroom_n16r8 -t upload --upload-port COM20
```

Use the real COM port assigned to the attached board. Many ESP32-CAM modules must be put into download mode before upload.

## Monitor serial output

Run:

```powershell
pio device monitor
```

The default monitor speed is `115200`. Serial output is useful for camera initialization failures, PSRAM failures, WiFi connection state, RTSP startup, and motion-alert errors.

## HTML status page generation

The status page source is:

```text
html/index.html
```

The embedded minified output is:

```text
html/index.min.html
```

Regenerate after editing the status page:

```powershell
python minify.py html/index.html html/index.min.html
```

The repository includes `generate_html.ps1` and `generate_html.sh`, but those scripts install or update Python packages. Use the direct `minify.py` command when the existing local environment already has what it needs.

## CI notes

The README badge currently points at the upstream `esp32cam-rtsp` workflow, while the XCAM repo itself should be checked locally with:

```powershell
python scripts/run-host-tests.py
pio run
```

If a GitHub Actions workflow is added later, it should run the host tests and build the active PlatformIO environments that are expected to remain supported.

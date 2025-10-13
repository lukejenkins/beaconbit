# Multi-target build instructions

This project supports building for multiple Espressif MCUs. Below are mappings from common board names to the IDF target names and instructions to build locally.

Board -> IDF target mapping

- ESP32-D0WD-V3 -> esp32
- ESP32-C3-DevKitM-1 -> esp32c3
- ESP32-S3-DevKitC-1 -> esp32s3
- ESP32-C6-DevKitM-1 -> esp32c6
- ESP32-C61-DevKitC-1 -> esp32c6 (C61 is part of the C6 family; use `esp32c6` target)

Local build steps

1. Source the IDF environment (adjust IDF path if needed):

```sh
source $HOME/esp/v5.5.1/esp-idf/export.sh
```

1. Set the target and build:

```sh
# Example: build for ESP32-S3
export IDF_TARGET=esp32s3
idf.py fullclean
idf.py build
```

1. Flash and monitor (specify your serial port):

```sh
idf.py -p /dev/tty.usbmodem141201 flash monitor
```

CI

We added a GitHub Actions workflow `.github/workflows/build-matrix.yml` that builds and runs the audit script across the target matrix listed above.

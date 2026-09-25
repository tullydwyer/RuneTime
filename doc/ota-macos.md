# Build and install RuneTime from a Mac

This procedure was used successfully on 25 September 2026 with a PineTime running InfiniTime. It uses the repo's Docker build and macOS Bluetooth LE through [Bleak](https://bleak.readthedocs.io/). The application DFU package is transferred over the watch's legacy Nordic DFU service.

## 1. Build a fresh application DFU ZIP

From the repository root, with Docker running:

```sh
docker run --rm -v "$PWD":/sources --user "$(id -u):$(id -g)" infinitime/infinitime-build
```

Confirm the build exits successfully and produces `build/output/pinetime-mcuboot-app-dfu-<version>.zip`. Use the ZIP from this build; the script checks its manifest, application size, and CRC before connecting. Running `cmake --build build` directly on macOS fails when the existing cache was generated inside Docker at `/sources`.

Do not use the recovery-loader ZIP, `bootloader.bin`, or a raw image for this application update.

## 2. Prepare the watch and Python

Keep the watch near the Mac, enable its Bluetooth, and disconnect it from phone companion apps. If needed, enable updates on the watch under **Settings → Firmware & files → Till reboot** (or **Enabled**). A phone connection can prevent the watch from advertising to the laptop.

Use Python 3.12 with an isolated environment. Bleak 3.0.2 was the version used for the successful transfer:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install 'bleak==3.0.2'
.venv/bin/python scripts/pinetime_ble_dfu.py --check
```

The read-only check must find one `InfiniTime` advertisement with DFU service `00001530-1212-efde-1523-785feabcd123`, then connect and read model `PineTime` and battery level. If it finds no watch, check the phone connection and watch Bluetooth, then retry. Do not guess a nearby BLE device's identity.

## 3. Transfer the application image

Use the **fresh** ZIP path printed by the build, for example:

```sh
.venv/bin/python scripts/pinetime_ble_dfu.py build/output/pinetime-mcuboot-app-dfu-1.16.0.zip
```

The script verifies the ZIP's image CRC and size, checks watch identity and battery, then sends 20-byte packets with receipt checks every 10 packets. A successful run reports that the watch received the full image, validated the CRC, and accepted the activation command. The watch normally disconnects while rebooting.

Wait for the watch to return to its face. With the phone still disconnected, run `--check` again to confirm it is advertising and reconnects as `PineTime`/`InfiniTime`. It may take several tens of seconds to advertise after reboot. If the phone reconnects first, use the watch's screen to confirm that it booted instead.

## 4. Validate on the watch

From the watch face, swipe right, open **Settings** with the cogwheel, open **Firmware**, then choose **Validate**. InfiniTime can revert an unvalidated update after a later reset. This final action is on the watch; the laptop transfer does not perform it.

The successful 25 September 2026 transfer used a 393,888-byte application image with manifest CRC 54601. The watch reported 99% battery, acknowledged every receipt count, validated the full image CRC, rebooted, and later reconnected as `PineTime`/`InfiniTime`. These values describe that build and are not fixed values for future releases.

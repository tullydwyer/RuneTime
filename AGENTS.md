# RuneTime agent notes

This is a personal PineTime/InfiniTime firmware project. For build and watch-install requests, follow [the macOS Bluetooth update guide](doc/ota-macos.md).

- Build firmware inside the `infinitime/infinitime-build` Docker image. The local `build/` CMake cache is configured for `/sources` inside that container and cannot be used directly from macOS.
- After code changes, build a fresh `pinetime-mcuboot-app-dfu-*.zip` and check the build result. Do not install an older ZIP already in `build/output/`.
- Use the application DFU ZIP for routine updates. Bootloader, recovery loader, raw `.bin`, and `.hex` files serve different purposes.
- On this Mac, the watch advertises as `InfiniTime` with DFU service `00001530-1212-efde-1523-785feabcd123` after its phone companion disconnects. Verify the GATT model is `PineTime` and battery is sufficient before sending firmware.
- `scripts/pinetime_ble_dfu.py --check` is read-only. Give the script the freshly built ZIP only when the user has asked to install firmware.
- After transfer, verify the watch rebooted and is running InfiniTime. The user must validate the new firmware on the watch through **Settings → Firmware → Validate**, or it may revert on a later reset.

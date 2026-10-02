# Day 34 kept build

This is a snapshot of the Pi Pet candidate that passed review, kept so it can be restored without regenerating it.
The lesson's source is still the prompt, spec, tests, and acceptance criteria.
New candidates go in the ignored `.build/` folder.

- **Built:** September 25, 2026, alongside commit `687e62a4` ("centre the pet, fix tilt axes").
- **Verified on device:** on October 2, 2026, a flash read-back from Stopwatch `28:84:85:43:aa:a0` matched these files byte for byte: the app in `app0`, the bootloader, and the partition table.
- **Toolchain:** Arduino ESP32 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26.
- **Board settings:** `esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi`.

## Contents

| Path | What it is |
| --- | --- |
| `device/pi_pet/` | Sketch source. `grok_assets.h` is generated; it is kept so the sketch builds as is. |
| `device/assets/` | `gen_assets.py eyes.json grok_assets.h` regenerates the header. Needs numpy, shapely, and svgpathtools. |
| `device/bin/` | The built images, `flash_args`, `partitions.csv`, and `SHA256SUMS`. |
| `host/pi-pet/` | The pi extension and hub that relay agent status over USB or Wi-Fi. |

The images contain no Wi-Fi credentials or pairing token.
The device stores those in NVS at runtime.

## Flash

Stop the hub first (or pause it), since it holds the serial port.

App update. Keeps NVS: touch calibration, Wi-Fi, and pairing.

```bash
esptool --port /dev/cu.usbmodem101 write-flash 0x10000 device/bin/pi_pet.ino.bin
```

Fresh install. Rewrites the bootloader, partition table, and OTA data.
On a board that already has this partition table, NVS survives.
On a board with a different layout, calibrate and pair again.

```bash
esptool --port /dev/cu.usbmodem101 write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 device/bin/pi_pet.ino.bootloader.bin 0x8000 device/bin/pi_pet.ino.partitions.bin 0xe000 device/bin/boot_app0.bin 0x10000 device/bin/pi_pet.ino.bin
```

## Install the host

```bash
cp -R host/pi-pet ~/.pi/agent/extensions/ && npm --prefix ~/.pi/agent/extensions/pi-pet install
```

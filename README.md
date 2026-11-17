# Meshtastic node with Raspberry Pi Pico + DX-LR20 (LLCC68)

A very low-cost, fully DIY [Meshtastic](https://meshtastic.org) node built from a **Raspberry Pi Pico (RP2040)** and a **DX-LR20 LoRa module (Semtech LLCC68)**. The goal of this project is to make it possible to assemble nodes with exactly this configuration: a wiring diagram, a small set of changes to the official firmware, and the build and flashing steps.

This is both a hardware and a firmware project:

- **Hardware:** an SPI wiring between the Pico and the module, including the module's external RF switch control (RXEN/TXEN).
- **Firmware:** the official Meshtastic firmware plus a custom `variant.h`, one build flag and one small source patch.

![Wiring diagram](pico_dx-lr20_wiring.jpg)

> **Firmware base:** Meshtastic `2.8.0`, upstream commit [`fcfe329`](https://github.com/meshtastic/firmware/commit/fcfe329), PlatformIO environment `pico`.

## Contents

1. [Why the LLCC68](#1-why-the-llcc68)
2. [LLCC68 limitations](#2-llcc68-limitations)
3. [Hardware](#3-hardware)
4. [Firmware](#4-firmware)
5. [Design notes](#5-design-notes)
6. [Using the node from a host over USB](#6-using-the-node-from-a-host-over-usb)
7. [Troubleshooting](#7-troubleshooting)
8. [Known limitations of this build](#8-known-limitations-of-this-build)
9. [License and credits](#9-license-and-credits)

---

## 1. Why the LLCC68

The LLCC68 is described in the Zephyr device-tree bindings as [a cost-cut version of the SX126x](https://docs.zephyrproject.org/latest/build/dts/api/bindings/lora/semtech%2Cllcc68.html), and Semtech lists it as [pin-to-pin compatible with the SX1262](https://www.semtech.com/products/wireless-rf/lora-connect/llcc68). In practice that gives this build a few clear advantages:

- **Very low cost.** A Raspberry Pi Pico and an LLCC68 module are among the cheapest parts you can use to build a Meshtastic node.
- **Same radio interface as the SX1262.** It uses the same SPI command set, so Meshtastic drives it through its existing SX126x code (`USE_LLCC68`).
- **Same output power class.** Up to +22 dBm, like the SX1262.
- **Simple, solderable build.** Eleven wires between two boards, no custom PCB required.

It is a good fit for cheap fixed nodes, test nodes, and local meshes where airtime and low cost matter more than maximum range.

## 2. LLCC68 limitations

The LLCC68 is cheaper because Semtech removed part of the LoRa modulation range. **LLCC68 nodes cannot operate in every Meshtastic mode.** This matters before you build.

### What the chip does not support

According to the list of unsupported combinations posted on [Semtech's LoRa developer forum](https://forum.lora-developers.semtech.com/t/llcc68-support-for-lorawan/814):

- **SF12** is not supported.
- **SF11** is not supported at 125 kHz or 250 kHz bandwidth.
- **SF10** is not supported at 125 kHz bandwidth.

In other words, the maximum spreading factor depends on the bandwidth: **SF9 at 125 kHz, SF10 at 250 kHz, SF11 at 500 kHz.**

### What that means for Meshtastic presets

Preset parameters below are taken from the firmware's `modemPresetToParams()` (`src/mesh/MeshRadio.h`):

| Preset | SF | Bandwidth | Works on LLCC68 |
|---|---|---|---|
| `SHORT_TURBO` | 7 | 500 kHz | ✅ (needs a region that allows a 500 kHz channel) |
| `SHORT_FAST` | 7 | 250 kHz | ✅ |
| `SHORT_SLOW` | 8 | 250 kHz | ✅ |
| `MEDIUM_FAST` | 9 | 250 kHz | ✅ |
| `MEDIUM_SLOW` | 10 | 250 kHz | ✅ |
| `LONG_TURBO` | 11 | 500 kHz | ✅ (needs a region that allows a 500 kHz channel) |
| **`LONG_FAST`** (upstream default) | 11 | 250 kHz | ❌ |
| `LONG_MODERATE` | 11 | 125 kHz | ❌ |
| `LONG_SLOW` | 12 | 125 kHz | ❌ |

Practical consequences:

- **LLCC68 nodes cannot join a mesh that uses the default `LONG_FAST` preset.** LoRa nodes with different SF/bandwidth cannot demodulate each other, so every node in a mesh, including SX1262-based ones, must use the same preset.
- **Use `MEDIUM_FAST`, `MEDIUM_SLOW` or one of the `SHORT_*` presets** for these nodes. The reference build runs `MEDIUM_FAST` in the EU 868 region.
- **Lower spreading factors mean lower sensitivity.** Each SF step adds roughly 2.5 dB of sensitivity, so SF9/SF10 links have a smaller link budget than the SF11/SF12 links used by the long presets. Expect shorter range per hop in exchange for shorter airtime.
- **500 kHz presets need a region plan that can host a 500 kHz channel.** The EU 868 plan used by the firmware spans only 0.25 MHz (869.400–869.650 MHz), so it holds a single 250 kHz channel (869.525 MHz) and has no room for a 500 kHz one.
- **The firmware does not block unsupported presets for the LLCC68.** `LLCC68Interface` is a thin wrapper around the SX126x interface and contains no preset check. Choose a supported preset yourself.

## 3. Hardware

### 3.1 Parts

| Part | Notes |
|---|---|
| Raspberry Pi Pico | Standard Pico (RP2040, 2 MB flash). Target `pico`. Not the Pico W or Pico 2. |
| DX-LR20 module (LLCC68) | The **bare SPI module** (868 MHz version for EU). Kit variants with an onboard controller expose a UART/AT interface and are not suitable. |
| 868 MHz antenna | Match the antenna to the module's connector. |
| Decoupling capacitors | About 100 µF electrolytic plus 100 nF ceramic, close to the module's VCC pin. |
| Wires / soldering | Short wires. |
| Optional power | LiPo cell with a protected charge module (for example a TP4056 board with protection). |

The module exposes 12 pins: `VCC, GND, NSS, NRST, MOSI, SCK, MISO, DIO1, DIO2, BUSY, TXEN, RXEN`.

### 3.2 Wiring

The Pico uses its **SPI1** peripheral. Pin numbers are physical header pins.

| DX-LR20 pin | Pico GPIO | Pico pin |
|---|---|---|
| VCC | 3V3(OUT) | 36 |
| GND | GND | 38 (any GND) |
| NSS | GP3 | 5 |
| MOSI | GP11 | 15 |
| MISO | GP12 | 16 |
| SCK | GP10 | 14 |
| NRST | GP15 | 20 |
| DIO1 | GP20 | 26 |
| BUSY | GP2 | 4 |
| TXEN | GP22 | 29 |
| RXEN | GP21 | 27 |
| **DIO2** | **not connected** | — |

Notes:

- **RXEN and TXEN are driven by GPIOs.** The DX-LR20 needs its RF switch controlled externally. Do not bridge DIO2 to TXEN and do not tie RXEN/TXEN to fixed levels.
- **DIO2 stays unconnected.** The firmware is built without `SX126X_DIO2_AS_RF_SWITCH`.
- **3.3 V only.** Power the module from the Pico's 3V3(OUT) pin.
- **GP29 is not usable** on a standard Pico (it is wired to the internal VSYS measurement).
- **GP16 and GP17 are reserved by the variant** for the external notification output and the user button.
- **Never power the module without an antenna connected.**
- **Layout.** Keep the SPI wires short, place the decoupling capacitors right at the module's VCC, and keep the antenna and its cable away from the Pico and from any switching converter or charger.

## 4. Firmware

### 4.1 Requirements

- [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/install/ide?install=vscode) extension
- Git
- Python 3

### 4.2 Get the source

```bash
git clone https://github.com/meshtastic/firmware.git
cd firmware
git checkout fcfe329
git submodule update --init --recursive
```

Open the **repository root** folder in VS Code. Do not open a variant subfolder on its own: the shared `[rp2040_base]` section lives in `variants/rp2040/rp2040.ini` and is only resolved from the root project.

### 4.3 Firmware changes

Three small changes are needed.

#### A. Replace `variants/rp2040/rpipico/variant.h`

Use the [`variant.h`](variant.h) included in this repository. Its main content is:

```cpp
#define EXT_NOTIFY_OUT 16
#define BUTTON_PIN 17

#define LED_POWER PIN_LED

// Battery sensing disabled (no voltage divider fitted)
#define BATTERY_PIN -1
#define ADC_MULTIPLIER 3.1
#define BATTERY_SENSE_RESOLUTION_BITS ADC_RESOLUTION

// Use the LLCC68 radio path
#define USE_LLCC68

#undef LORA_SCK
#undef LORA_MISO
#undef LORA_MOSI
#undef LORA_CS

#define LORA_SCK  10
#define LORA_MISO 12
#define LORA_MOSI 11
#define LORA_CS   3

#define LORA_DIO0  RADIOLIB_NC
#define LORA_RESET 15
#define LORA_DIO1  20
#define LORA_DIO2  RADIOLIB_NC
#define LORA_DIO3  RADIOLIB_NC

#ifdef USE_LLCC68
#define SX126X_CS    LORA_CS
#define SX126X_DIO1  LORA_DIO1
#define SX126X_BUSY  2
#define SX126X_RESET LORA_RESET
#define SX126X_TXEN  22
#define SX126X_RXEN  21
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#endif
```

`ADC_MULTIPLIER` and `BATTERY_SENSE_RESOLUTION_BITS` must stay defined even though battery sensing is off, otherwise `src/Power.cpp` fails to compile.

#### B. Add a build flag in `variants/rp2040/rpipico/platformio.ini`

In the `[env:pico]` section, keep the existing flags and add `MAX_NUM_NODES`:

```ini
build_flags =
  ${rp2040_base.build_flags}
  -D RPI_PICO
  -I variants/rp2040/rpipico
  -D DEBUG_RP2040_PORT=Serial
  -D HW_SPI1_DEVICE
  -D MAX_NUM_NODES=40
```

- `-D HW_SPI1_DEVICE` is required: it makes the firmware configure `SPI1`, whose pins GP10/GP11/GP12 are used for the radio.
- `-D MAX_NUM_NODES=40` limits the in-memory node database size.

#### C. Disable node database persistence in `src/mesh/NodeDB.cpp`

Replace the body of `NodeDB::saveNodeDatabaseToDisk()` so it does nothing and reports success:

```cpp
bool NodeDB::saveNodeDatabaseToDisk()
{
    // Node database persistence disabled: the database lives in RAM only.
    // Returning true keeps the firmware from treating the save as a failure.
    return true;
}
```

Configuration, channels and device state are still saved normally. See [Design notes](#5-design-notes).

### 4.4 Build

The PlatformIO environment is called **`pico`**.

In VS Code: `Ctrl+Shift+P` → **PlatformIO: Pick Project Environment** → `env:pico`, then **PlatformIO: Build**.

Or from a terminal:

```bash
pio run -e pico
```

The output is named after the environment and version, not `firmware.uf2`:

```
.pio/build/pico/firmware-pico-<version>.uf2
```

### 4.5 Flash

1. Hold **BOOTSEL**, plug in the USB cable, then release. The Pico appears as the `RPI-RP2` drive.
2. For a first install, or to start from a clean flash, copy [`flash_nuke.uf2`](https://datasheets.raspberrypi.com/soft/flash_nuke.uf2) to the drive and wait for the Pico to restart.
3. Put the Pico in BOOTSEL mode again and copy the `firmware-pico-<version>.uf2` you built.

Flashing a new firmware does not erase the filesystem; the nuke step does.

### 4.6 First configuration

Set the region and a **preset supported by the LLCC68** (see [section 2](#2-llcc68-limitations)). With the [Meshtastic CLI](https://meshtastic.org/docs/software/python/cli/):

```bash
python -m pip install meshtastic
python -m meshtastic --set lora.region EU_868
python -m meshtastic --set lora.modem_preset MEDIUM_FAST
```

The same settings can be made from the Meshtastic mobile app.

Device role: the reference build ran stably as `ROUTER_LATE` (the role intended for infrastructure nodes that cover areas without other routers in view). Upstream recommends `CLIENT`, `CLIENT_MUTE` or `CLIENT_BASE` unless you have a specific reason to use a router role.

### 4.7 Expected boot log

A correctly assembled node prints these lines at start-up (serial, 115200 baud):

```
SX126xInterface(cs=3, irq=20, rst=15, busy=2)
SX126X_DIO3_TCXO_VOLTAGE not defined, not using DIO3 as TCXO reference voltage
SX126x init result 0
Use MCU pin 21 as RXEN and pin 22 as TXEN to control RF switching
Set RX gain to boosted mode; result: 0
Applied SX1262 register 0x8B5 patch for RX improvement
LLCC68 init success
```

The "TCXO not defined" line is expected: the LLCC68 code path does not pass the TCXO voltage to the driver, and the module initialises without it.

## 5. Design notes

| Change | Reason |
|---|---|
| `USE_LLCC68` | Selects the `LLCC68Interface` radio path (a thin wrapper around the SX126x driver). |
| `SX126X_TXEN` / `SX126X_RXEN` on GPIOs | The DX-LR20's RF switch is controlled externally. With these defined, the firmware reports them at boot and uses them to switch the RF path between RX and TX. |
| No `SX126X_DIO2_AS_RF_SWITCH` | The upstream Pico variant lets DIO2 drive the switch. Here the switch is driven by the MCU instead. |
| `SX126X_DIO3_TCXO_VOLTAGE 1.8` | Kept for reference. In the LLCC68 branch of `RadioInterface.cpp` the value is not applied. |
| `BATTERY_PIN -1` | The Pico has no battery voltage divider. Leaving the upstream ADC pin enabled produced bogus low-voltage readings. |
| `EXT_NOTIFY_OUT 16` | Some upstream revisions use GP22 for this output, which collides with `TXEN`. |
| `-D HW_SPI1_DEVICE` | Selects `SPI1`, which is where GP10/GP11/GP12 live. |
| `-D MAX_NUM_NODES=40` | Smaller in-memory node database. |
| `saveNodeDatabaseToDisk()` returns `true` | On this target the node froze while writing `/prefs/nodes.proto` to the 0.5 MB LittleFS partition. The node database is now kept in RAM only. |

Effect of the node database change:

- After every reboot the node relearns its neighbours from mesh traffic.
- Public keys of other nodes are not persisted, so features that rely on a previously stored key (for example remote administration over PKI) need the other node to be heard again after a restart.
- Configuration, channels and device state **are** still written to flash.

## 6. Using the node from a host over USB

The Pico enumerates as a USB serial device. If you drive it from your own software:

- Use the official [`meshtastic` Python library](https://github.com/meshtastic/python) (`SerialInterface`). It sends a heartbeat every 300 seconds while connected.
- The firmware closes the serial API session (`Lost phone connection`) if it receives nothing from the client for **15 minutes** (`SERIAL_CONNECTION_TIMEOUT` in `src/SerialConsole.cpp`). Any message from the client resets the timer; a new `want_config_id` handshake reopens the session.
- A heartbeat with the default nonce makes the firmware answer with a queue-status message, so it can double as a liveness probe from the host side.
- Use a short, good-quality USB data cable.

## 7. Troubleshooting

| Symptom | Meaning |
|---|---|
| `SX126x init result -2` | RadioLib's *chip not found*: the module does not answer over SPI. This points to SPI wiring, power, reset, or a missing `-D HW_SPI1_DEVICE`. |
| `SX126x init result -20` with some commands failing | RadioLib's *wrong modem* error, with commands intermittently failing. This can indicate a loose connection or an unstable supply. |
| `SX126x init result -707` | RadioLib's *failed to execute SPI command*. RadioLib notes this is often a TCXO/crystal mismatch. |
| Compile error `BATTERY_SENSE_RESOLUTION_BITS was not declared` | `ADC_MULTIPLIER` and `BATTERY_SENSE_RESOLUTION_BITS` were removed from `variant.h`; keep them. |
| No `firmware.uf2` after the build | The file is named `firmware-pico-<version>.uf2` in `.pio/build/pico/`. |
| VS Code warns `No section: 'rp2040_base'` for `variants/rp2040/rpipico/platformio.ini` | Harmless: the file is being validated on its own. Open the repository root instead. |

## 8. Known limitations of this build

- **LLCC68 preset restrictions** (see [section 2](#2-llcc68-limitations)): no `LONG_FAST`, `LONG_MODERATE` or `LONG_SLOW`.
- **No node database persistence** on the Pico (see [Design notes](#5-design-notes)). The root cause of the freeze on flash writes has not been established; this change is a workaround.
- **Pinned upstream version.** The variant is a full replacement of the upstream file and is validated against commit `fcfe329`. Upstream changes the file over time, so keep the pinned commit or re-apply the changes by hand.
- **No watchdog** is configured by this project.
- **Pico filesystem size.** The upstream RP2040 base configuration reserves 0.5 MB for LittleFS; this project does not change it.

## 9. License and credits

- The firmware is the [Meshtastic](https://github.com/meshtastic/firmware) project, which is licensed under GPL-3.0. The modifications in this repository are derivative work and follow the same license.
- *Meshtastic®* is a registered trademark of Meshtastic LLC. This project is not affiliated with or endorsed by Meshtastic LLC.
- The LLCC68 SF/bandwidth limits come from Semtech's published information and the Zephyr LLCC68 device-tree binding.

# T-Watch_Azan

Islamic prayer times (Azan) for the **LilyGo / TTGO T-Watch 2020**.

Prayer times are **computed astronomically on the watch** from its current
location and date — there is no pre-stored timetable, so the firmware is
correct anywhere in the world. Location comes from the **GPS** (T-Watch 2020 V2)
or from a one-time **WiFi setup page** (T-Watch 2020 V3, which has no GPS).

---

## Table of contents
- [Features](#features)
- [How it works](#how-it-works)
- [Choosing your board](#choosing-your-board)
- [Configuration (`config.h`)](#configuration-configh)
- [Dependencies](#dependencies)
- [Building & flashing the firmware](#building--flashing-the-firmware)
  - [Option A — Arduino IDE](#option-a--arduino-ide-easiest)
  - [Option B — VS Code + Arduino extension](#option-b--vs-code--arduino-extension)
  - [Option C — arduino-cli](#option-c--arduino-cli-command-line)
- [First-time setup on the watch](#first-time-setup-on-the-watch)
- [Using the watch](#using-the-watch)
- [Project structure](#project-structure)
- [Accuracy & calculation method](#accuracy--calculation-method)
- [Troubleshooting](#troubleshooting)
- [Notes & things to verify](#notes--things-to-verify)

---

## Features
- Astronomical prayer-time calculation (PrayTimes.org / NOAA solar algorithm), **Muslim World League** angles by default, fully configurable via `#define`s.
- Computes all six daily times: **Fajr, Shurooq (sunrise), Duhr, Asr, Maghrib (sunset), Isha**.
- Location from **GPS** (V2) or a **WiFi captive-portal page** (V3) — acquired once, cached in flash (NVS), and reused.
- Sets the watch **RTC clock** automatically from GPS / browser time.
- Colour-coded prayer states, vibration alerts before/at Azan, adaptive screen brightness, Friday highlight, and auto-sleep.

---

## How it works
1. On boot the watch loads the cached location from NVS. If none exists, it
   launches **location setup** (GPS on V2, WiFi page on V3). You can re-run setup
   any time with a **long-press of the power button**.
2. Each day it computes the six times from `(date, latitude, longitude, timezone)`
   using `PrayerCalc.h`, then displays and tracks them.
3. The timezone is `UTC_OFFSET + (DST_ENABLED ? 1 : 0)` — GPS/browser time is in
   UTC and is converted to local clock time with this offset.

A `#define` **default location** is used until the very first fix is obtained, so
the watch shows sensible times out of the box.

---

## Choosing your board
The board is a **compile-time** choice (the LilyGoWatch library sets pin maps and
peripherals from it), so build the firmware **once per board**. Edit the top of
[`config.h`](config.h):

| Your watch | Uncomment in `config.h` | Location source |
|---|---|---|
| **T-Watch 2020 V2** | `#define LILYGO_WATCH_2020_V2` | onboard **GPS** |
| **T-Watch 2020 V3** | `#define LILYGO_WATCH_2020_V3` | **WiFi** setup page (no GPS) |

> Not sure which you have? The **V2** has a GPS antenna; the **V3** replaced GPS
> with a microphone. If you build the V3 firmware on a V2 (or vice-versa) GPS/WiFi
> setup simply won't work as intended — pick the matching define.

Exactly one board define must be active. The header derives `HAS_GPS` (V2) or
`USE_WIFI_LOCATION` (V3) from it automatically.

---

## Configuration (`config.h`)
All user settings live in [`config.h`](config.h) as `#define`s:

| Define | Default | Meaning |
|---|---|---|
| `FAJR_ANGLE` | `18.0` | Fajr twilight depression angle (degrees) |
| `ISHA_ANGLE` | `17.0` | Isha twilight depression angle (degrees) |
| `ASR_FACTOR` | `1` | Asr shadow factor — `1` = Shafi/Standard, `2` = Hanafi |
| `SUNSET_ANGLE` | `0.833` | Sunrise/Maghrib refraction + sun radius |
| `DUHR_OFFSET_MIN` | `0` | Minutes added after true solar noon (zawāl safety margin) |
| `ISHA_MINUTES_AFTER_MAGHRIB` | *(unset)* | Set to e.g. `90` for **Umm al-Qura** (overrides `ISHA_ANGLE`) |
| `UTC_OFFSET` | `1` | Base UTC offset in hours |
| `DST_ENABLED` | `0` | `1` adds one hour for daylight-saving (summer time) |
| `DEFAULT_LAT` / `DEFAULT_LON` | `54.65` / `12.95` | Fallback location until the first fix |
| `AP_SSID` / `AP_PASS` | `T-Watch-Azan` / *(open)* | WiFi setup AP (V3). `AP_PASS` needs ≥ 8 chars or stays open |

**Common method presets** (change the two angles + `ASR_FACTOR`):

| Method | Fajr | Isha | Notes |
|---|---|---|---|
| Muslim World League *(default)* | 18.0° | 17.0° | |
| ISNA (North America) | 15.0° | 15.0° | |
| Egyptian Authority | 19.5° | 17.5° | |
| Umm al-Qura (Makkah) | 18.5° | — | set `ISHA_MINUTES_AFTER_MAGHRIB 90` |
| Karachi | 18.0° | 18.0° | |

> **Daylight saving:** there is no automatic DST. During summer time set
> `DST_ENABLED 1` and back to `0` in winter (or bake your region's rule in later
> using `getLastSundayInMonth()` in `DateHelper.h`).

---

## Dependencies
Install these before building:

1. **ESP32 Arduino core** (`espressif:esp32`, 2.0.x). Add this URL in
   *Boards Manager → Additional URLs* and install **esp32 by Espressif Systems**:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
   The T-Watch board (`twatch`) ships with this core.
2. **TTGO_TWatch_Library** (LilyGoWatch HAL) — download the ZIP from
   <https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library> and unzip into your
   Arduino libraries folder (e.g. `~/Arduino/libraries/TTGO_TWatch_Library-master`),
   or *Sketch → Include Library → Add .ZIP Library*.
3. **esp32_https_server** *(V3 builds only)* — by Frank Hessel; install via
   *Library Manager* ("esp32_https_server") or from
   <https://github.com/fhessel/esp32_https_server>. Provides the HTTPS setup page.

---

## Building & flashing the firmware

Connect the T-Watch over USB-C. On Linux it appears as `/dev/ttyACM0` (native USB)
or `/dev/ttyUSB0` (CP210x); on macOS `/dev/cu.usbmodem*` or `/dev/cu.SLAB_USBtoUART`;
on Windows a `COMx` port.

> **Linux serial permissions:** if upload fails with a permission error, add
> yourself to the `dialout` group once and re-login:
> `sudo usermod -aG dialout $USER`

The board settings used by this project (from `.vscode/arduino.json`):

```
Board (FQBN):  espressif:esp32:twatch
Options:       Revision=TWATCH_BASE, PSRAM=enabled, PartitionScheme=default,
               UploadSpeed=2000000, DebugLevel=none, EraseFlash=none
```

### Option A — Arduino IDE (easiest)
1. Install the dependencies above.
2. Open **`T-Watch_Azan.ino`** (all `.h` files must be in the same folder — they are).
3. Edit `config.h` for your board (V2/V3) and your timezone.
4. **Tools → Board → ESP32 Arduino → "TTGO T-Watch"**.
5. Set **Tools** menu options to match: *PSRAM: Enabled*, *Partition Scheme: Default*,
   *Upload Speed: 2000000*, *Core Debug Level: None*.
6. **Tools → Port →** your watch's port.
7. Click **Upload** (→). The IDE compiles and flashes; the watch reboots into the firmware.

### Option B — VS Code + Arduino extension
This repo already contains `.vscode/arduino.json` with the board, port, and
options pre-set.
1. Install the **Arduino** and **C/C++** extensions.
2. Open the project folder in VS Code.
3. Confirm/adjust the port in `.vscode/arduino.json` (`"port": "/dev/ttyACM0"`).
4. Edit `config.h` for your board.
5. Run **Arduino: Upload** from the Command Palette (`Ctrl/Cmd-Shift-P`).

### Option C — arduino-cli (command line)
```bash
# one-time setup
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
    https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32

# install libraries (esp32_https_server only needed for V3)
arduino-cli lib install esp32_https_server
# TTGO_TWatch_Library: unzip into ~/Arduino/libraries/  (not in the lib index)

# compile  (run from the repo root, where T-Watch_Azan.ino lives)
arduino-cli compile \
  --fqbn "espressif:esp32:twatch:Revision=TWATCH_BASE,PSRAM=enabled,PartitionScheme=default,UploadSpeed=2000000,DebugLevel=none,EraseFlash=none" \
  .

# flash  (set your port)
arduino-cli upload -p /dev/ttyACM0 \
  --fqbn "espressif:esp32:twatch:Revision=TWATCH_BASE,PSRAM=enabled,PartitionScheme=default,UploadSpeed=2000000,DebugLevel=none,EraseFlash=none" \
  .
```

> **If the binary doesn't fit** (V3 pulls in WiFi + TLS), change the partition
> scheme to a larger app partition (e.g. `PartitionScheme=huge_app`) in the
> IDE/CLI options.

---

## First-time setup on the watch
On first boot (no cached location), or after a **power-button long-press**:

**T-Watch 2020 V2 (GPS)**
1. The screen shows *"Acquiring GPS"* with the satellite count.
2. Go **outdoors** with a clear sky view (a cold start can take a few minutes;
   GPS rarely works indoors).
3. On a fix it shows the coordinates, sets the clock, saves the location, and
   returns to the prayer screen. Touch the screen to abort and keep the old value.

**T-Watch 2020 V3 (WiFi)**
1. The screen shows the WiFi name (`T-Watch-Azan`) and `https://192.168.4.1`.
2. On your phone/PC, **join that WiFi network**.
3. Open **`https://192.168.4.1`** and **accept the self-signed certificate
   warning** (one-time — it's the watch's own cert).
4. Tap **"Use my location"** and allow the browser's location prompt, **or** type
   your latitude/longitude in the manual boxes and tap **Save**.
5. The watch saves the location, sets its clock from your browser, turns WiFi off,
   and returns to the prayer screen.

> **Tip:** if "Use my location" is blocked (some captive-portal pop-up browsers
> disable geolocation), open `https://192.168.4.1` in a normal browser tab, or
> just use the **manual** boxes — they work over plain HTTP too.

Location and clock are cached, so this is normally a **one-time** step.

---

## Using the watch
- **Prayer rows** are colour-coded by state:
  - **Gold** — 15 min before a prayer (the watch also vibrates).
  - **Silver** — between Azan and Iqama.
  - **Green** — up to 1 hour after Azan.
  - **Orange** — after that, until the next prayer.
  - **Grey** — inactive.
- **Vibration** fires 15 min before, and again exactly at Azan time (waking the screen).
- **Brightness** adapts to the active prayer; **Friday** is highlighted in gold.
- **Power button — short press:** toggle the screen on/off.
- **Power button — long press:** re-run location setup.
- **Touch:** wakes/keeps the screen on; the watch auto-sleeps after ~10 s of inactivity.

---

## Project structure
```
T-Watch_Azan.ino   Main sketch: UI, loop, prayer-state colouring, alerts, sleep.
config.h           Board select + all user #defines (method, timezone, location, AP).
PrayerCalc.h       Astronomical prayer-time math (pure, host-testable).
Location.h         Location cache (NVS) + timezone + RTC sync + setup dispatch.
LocationGPS.h      V2: read lat/lon + UTC from the onboard GPS.
LocationWiFi.h     V3: WiFi AP + captive portal + HTTPS/HTTP setup page.
webcert.h          Embedded self-signed TLS cert for the V3 setup page.
DateHelper.h       Calendar helpers (day-of-year, weekday, leap year, DST boundary).
PrayerTimes.h      Legacy lookup table — no longer used (kept for reference).
times.json         Legacy reference timetable (not used by the firmware).
```

---

## Accuracy & calculation method
`PrayerCalc.h` implements the standard PrayTimes.org / NOAA solar position
algorithm (declination + equation of time → solar noon → twilight/altitude
angles). Validated against the bundled `times.json` (generated for ~54.65°N,
12.95°E, Muslim World League): sunrise / Duhr / sunset matched to **≈ 2 minutes**,
and the inferred twilight angles came out to exactly **18° / 17°**.

At very high latitudes Fajr/Isha can be undefined (the sun never reaches the
angle); the calculation clamps rather than producing invalid times. A dedicated
high-latitude rule is not implemented.

---

## Troubleshooting
| Symptom | Likely cause / fix |
|---|---|
| Compile error: `LilyGoWatch.h: No such file` | TTGO_TWatch_Library not installed in `~/Arduino/libraries/`. |
| Compile error: `HTTPSServer.hpp: No such file` (V3) | Install **esp32_https_server**. |
| Times are off by exactly 1 hour | Toggle `DST_ENABLED` for the season. |
| Times off by a few minutes vs your mosque | Adjust `FAJR_ANGLE`/`ISHA_ANGLE`/`ASR_FACTOR`, or set `DUHR_OFFSET_MIN`. |
| GPS never gets a fix (V2) | Go outdoors; cold starts take minutes. Touch to abort and keep the cached value. |
| Browser won't allow location (V3) | Use a normal browser tab (not the captive pop-up), or use manual entry. |
| Upload fails / no port | Check the USB cable & port; on Linux add yourself to `dialout`. |
| Binary too large to flash | Use a larger partition scheme (`huge_app`). |

---

## Notes & things to verify
This project was assembled without the Arduino toolchain present, so a few
hardware-touching points are marked with `VERIFY` comments in the source and
should be confirmed against **your** installed libraries:
- **GPS API** in `LocationGPS.h` (`trunOnGPS` / `gps_begin` / `gpsHandler` / `watch->gps`) — names vary between TTGO_TWatch_Library versions.
- **AXP202 long-press** — if a long press powers the watch off instead of firing the IRQ, configure the AXP long-press time or switch the re-trigger to a touch gesture.
- The embedded TLS cert in `webcert.h` is a throwaway device certificate (EC P-256, valid 2026–2036); regenerate it with the `openssl` commands in that file if needed.

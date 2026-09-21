# XiaoZhi ESP32-S3 SuperMini

This repository contains the custom `main` folder for the working ESP32-S3 SuperMini XiaoZhi configuration.

The reproducible setup is:

**official XiaoZhi source at commit `4632dc5` + this repository's `main` folder**

## Hardware

- ESP32-S3 SuperMini — ESP32-S3FH4R2, 4 MB Flash, 2 MB PSRAM
- INMP441 I2S microphone
- MAX98357A I2S amplifier
- No display

### INMP441

| INMP441 | ESP32-S3 |
|---|---|
| VDD | 3.3V |
| GND | GND |
| L/R | GND |
| WS | GPIO4 |
| SCK | GPIO5 |
| SD | GPIO6 |

### MAX98357A

| MAX98357A | ESP32-S3 |
|---|---|
| VIN | 5V |
| GND | GND |
| BCLK | GPIO12 |
| LRC | GPIO13 |
| DIN | GPIO11 |
| SD | 3.3V |
| GAIN | NC |

Speaker connects only to `OUT+` and `OUT-`.

Keep the microphone physically away from the speaker to reduce acoustic feedback.

---

## 1. Required software

Install:

- Git
- VS Code
- ESP-IDF 6.1
- ESP-IDF VS Code extension

Known ESP-IDF paths used by this project:

```text
C:\esp\v6.1\esp-idf
C:\Espressif\tools
C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
```

---

## 2. New-PC setup

Place `setup_xiaozhi_supermini.ps1` in the directory where you want the project.

Open PowerShell there and run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\setup_xiaozhi_supermini.ps1
```

The script:

1. Clones `https://github.com/78/xiaozhi-esp32.git`
2. Checks out upstream commit `4632dc5`
3. Clones `https://github.com/Aravinthkalai/xiaozhi-esp32.git`
4. Replaces the upstream `main` folder with your repository's `main`
5. Activates ESP-IDF 6.1
6. Sets `IDF_TARGET=esp32s3`

The resulting project is:

```text
xiaozhi-esp32\
```

### Important

The script deletes existing `xiaozhi-esp32` and `xiaozhi-esp32-custom` directories in its own location before cloning. Do not run it from a directory containing unsaved work in those folders.

---

## 3. Manual ESP-IDF environment setup

If you already have the project:

```powershell
& 'C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1'
```

Verify:

```powershell
idf.py --version
```

Expected:

```text
ESP-IDF v6.1
```

Set the target:

```powershell
$env:IDF_TARGET="esp32s3"
```

---

## 4. Enter the project

```powershell
cd C:\Users\<username>\Desktop\esp\xiaozhi-esp32
```

Check the upstream base:

```powershell
git log --oneline -1
```

Expected:

```text
4632dc5
```

---

## 5. Build

Use:

```powershell
python scripts\build.py esp32s3-supermini
```

A successful build ends with:

```text
Project build complete
```

The generated firmware is:

```text
build\xiaozhi.bin
```

The generated flash configuration should contain:

```text
--flash-size 4MB
```

Do not run:

```powershell
idf.py set-target esp32s3
```

as part of this workflow. The XiaoZhi board build script handles the board-specific configuration.

---

## 6. Flash

Connect the board. The current PC uses `COM8`.

```powershell
idf.py -p COM8 flash
```

Successful flashing should end with verification and a reset.

If the COM port is different:

```powershell
Get-CimInstance Win32_SerialPort |
    Select-Object DeviceID, Name
```

Then replace `COM8` with the detected port.

---

## 7. Serial monitor

```powershell
idf.py -p COM8 monitor
```

Exit the monitor with:

```text
Ctrl+]
```

---

## 8. Normal development sequence

Every time you open a new PowerShell session:

```powershell
& 'C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1'
```

Then:

```powershell
cd C:\Users\<username>\Desktop\esp\xiaozhi-esp32
$env:IDF_TARGET="esp32s3"
```

Build:

```powershell
python scripts\build.py esp32s3-supermini
```

Flash:

```powershell
idf.py -p COM8 flash
```

Monitor:

```powershell
idf.py -p COM8 monitor
```

---

## 9. Git workflow

The custom work is stored under `main`.

Check:

```powershell
git status
```

Review:

```powershell
git diff
```

Check whitespace:

```powershell
git diff --check
```

If this is the custom repository:

```powershell
git add main
git commit -m "Update ESP32-S3 SuperMini configuration"
git push origin main
```

The important custom files are:

```text
main\
├── boards\
│   └── esp32s3-supermini\
│       ├── config.h
│       ├── config.json
│       └── esp32s3_supermini_board.cc
├── CMakeLists.txt
├── Kconfig.projbuild
└── display\
    └── display.h
```

---

## 10. Upstream updates

The custom repository does not automatically receive upstream XiaoZhi changes.

The known working upstream base is:

```text
4632dc5
```

To inspect newer upstream commits:

```powershell
cd C:\Users\<username>\Desktop\esp\xiaozhi-esp32
git fetch origin
git log --oneline origin/main -10
```

If you intentionally change the upstream base, test the complete build and voice operation before changing the pinned commit in `setup_xiaozhi_supermini.ps1`.

---

## 11. Reproducibility

Do not copy the old `build` directory to another PC.

Recreate the project from:

```text
upstream XiaoZhi commit 4632dc5
+
custom main folder
```

Then run:

```powershell
python scripts\build.py esp32s3-supermini
```

This regenerates the build for the new machine.

---

## 12. Quick reference

### ESP-IDF environment

```powershell
& 'C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1'
```

### Version

```powershell
idf.py --version
```

### Target

```powershell
$env:IDF_TARGET="esp32s3"
```

### Build

```powershell
python scripts\build.py esp32s3-supermini
```

### Flash

```powershell
idf.py -p COM8 flash
```

### Monitor

```powershell
idf.py -p COM8 monitor
```

### Git

```powershell
git status
git diff
git diff --check
git add main
git commit -m "Update ESP32-S3 SuperMini configuration"
git push origin main
```

## Known-good configuration

```text
Board:       ESP32-S3 SuperMini
Chip:        ESP32-S3FH4R2
Flash:       4 MB
PSRAM:       2 MB
ESP-IDF:     6.1
Upstream:    4632dc5
Board name:  esp32s3-supermini
Microphone:  INMP441
Amplifier:   MAX98357A
Display:     None
Port:        COM8 (may change)
```

# Wiring and Protection Strategy - ESP32 Smart FI Diagnostic Tool

Dokumen ini menjelaskan koneksi fisik antara board **ESP32 Smart FI Diagnostic Tool** dengan sensor dan kelistrikan motor.

> **Target Board:** ESP32 DevKit / ESP32-S3 N16R8  
> **Tegangan Operasi:** 3.3V logic, 12V automotive input (via proteksi)

---

## 1. Modul Daya (Automotive Grade)

Sangat penting untuk menggunakan sistem daya yang stabil karena tegangan aki motor bisa berfluktuasi secara ekstrem (8V saat cranking, 15V saat charging).

| Koneksi          | Sumber                   | Pin / Modul               | Proteksi Utama                           |
| :--------------- | :----------------------- | :------------------------ | :--------------------------------------- |
| **Battery +12V** | Jalur Kunci Kontak (ACC) | Buck Converter IN+        | FUSE 1A, Reverse Diode (1N4007), TVS 18V |
| **Battery GND**  | Rangka / Negatif Aki     | Common GND                | -                                        |
| **Output +5V**   | Buck Converter OUT+      | ESP32 VIN, OLED, Buzzer   | Kapasitor 1000uF (Bulk) + 100nF (HF)     |
| **Output +3.3V** | ESP32 3V3 Regulator      | SD Card VCC, Sensor Logic | Kapasitor 10uF (Decoupling)              |

**Catatan:** Gunakan buck converter dengan input range 6-28V (misal: LM2596, MP1584) untuk menahan voltage spike saat starter.

---

## 2. Input Analog (Monitoring Pasif)

Gunakan pembagi tegangan (voltage divider) untuk menurunkan tegangan sensor ke level aman (0-3.3V).

### Monitor Tegangan Aki

```
Aki (8V-15V) ──┬── R1 (100kΩ) ──┬── GPIO 34 (ADC)
               │                 │
               │                 ├── Zener 3.3V ──┐
               │                 │                │
               │                 ├── C1 (100nF) ──┤
               │                 │                │
               └─────────────────┴── R2 (22kΩ) ──┴── GND
```

- Rasio: 22k/(100k+22k) = 0.18 → 14.8V × 0.18 = 2.66V (aman)
- Formula: `V_batt = V_adc / 0.18`

### Sensor TPS (Throttle Position)

```
TPS Signal (0-5V) ── R1 (10kΩ) ──┬── GPIO 35 (ADC)
                                  │
                                  ├── Zener 3.3V ──┐
                                  │                │
                                  ├── C1 (100nF) ──┤
                                  │                │
                                  └── R2 (4.7kΩ) ──┴── GND
```

- Rasio: 4.7k/(10k+4.7k) = 0.32 → 5V × 0.32 = 1.6V max

### Sensor Suhu Mesin (NTC / ECT)

```
ECT Signal ── R1 (10kΩ) ──┬── GPIO 32 (ADC)
                           │
                           ├── Zener 3.3V ──┐
                           │                │
                           └── R2 (4.7kΩ) ──┴── GND
```

### Tabel Pin Analog

| Sinyal          | GPIO |  Divider Ratio  | Range Input | Range ADC |
| :-------------- | :--: | :-------------: | :---------- | :-------- |
| Battery Voltage |  34  | 0.18 (100k/22k) | 0-16V       | 0-2.88V   |
| TPS Signal      |  35  | 0.32 (10k/4.7k) | 0-5V        | 0-1.6V    |
| Engine Temp     |  32  | 0.32 (10k/4.7k) | 0-5V        | 0-1.6V    |

---

## 3. Input Digital Terisolasi (Signal Pulse)

Gunakan **Optocoupler (PC817)** untuk memisahkan tegangan motor (12V) dari pin ESP32 (3.3V).

### Skema Optocoupler (per channel)

```
Signal 12V ── R_limit (1kΩ) ──┬── Opto LED Anode (+)
                               │
                    Opto LED Cathode (-) ── Signal GND (motor)

ESP32 3.3V ── R_pullup (10kΩ) ──┬── GPIO_PIN (INPUT_PULLUP)
                                  │
                       Opto Collector ──┘
                       Opto Emitter ── ESP32 GND
```

**Catatan:** Output optocoupler adalah ACTIVE LOW (LOW saat sinyal aktif).

### Tabel Pin Digital

| Sinyal             | GPIO | Jenis Input               | Keterangan                         |
| :----------------- | :--: | :------------------------ | :--------------------------------- |
| **MIL Signal**     |  25  | Level / Pulse Count       | Lampu FI (kedipan = kode error)    |
| **Starter Signal** |  26  | Level (HIGH saat starter) | Deteksi cranking event             |
| **Ignition Pulse** |  27  | Frequency (RPM)           | 1 pulse per 2 rev (4-stroke 1-cyl) |
| **Injector Pulse** |  14  | PWM / Pulse Width         | Durasi semprotan injektor          |
| **Fuel Pump**      |  12  | Level (optional)          | Monitor relay fuel pump            |

---

## 4. Komunikasi ECU (K-Line)

Untuk diagnostik Tier 4 (baca DTC langsung dari ECU), gunakan interface K-Line via transistor level shifter atau IC MC33290.

### Skema K-Line (Simplified)

```
ECU K-Line ──┬── R (510Ω) ── +12V (pull-up)
             │
             ├── NPN Collector (TX path)
             │
             └── Voltage Divider ── GPIO 16 (RX)

GPIO 17 (TX) ── R (1kΩ) ── NPN Base
NPN Emitter ── GND
```

| Sinyal    | GPIO | Baud Rate  | Protokol                |
| :-------- | :--: | :--------: | :---------------------- |
| K-Line RX |  16  | 9600/10400 | ISO 9141 (Honda/Yamaha) |
| K-Line TX |  17  | 9600/10400 | ISO 9141 (Honda/Yamaha) |

**Catatan:** Honda menggunakan 10400 baud, Yamaha menggunakan 9600 baud.

---

## 5. Periferal Antarmuka

### OLED Display (I2C)

| Pin     | GPIO | Fungsi                 |
| :------ | :--: | :--------------------- |
| SDA     |  21  | I2C Data               |
| SCL     |  22  | I2C Clock              |
| Address |  -   | 0x3C (default SSD1306) |

### MicroSD Card (SPI)

| Pin  | GPIO | Fungsi      |
| :--- | :--: | :---------- |
| CS   |  4   | Chip Select |
| MOSI |  23  | Master Out  |
| MISO | 19\* | Master In   |
| SCK  | 18\* | Clock       |

> ⚠️ **Pin Sharing:** GPIO 18 dan 19 digunakan untuk SPI SD Card. Jangan gunakan pin ini untuk fungsi lain (tombol navigasi harus dipindah jika pakai SD Card).

### Navigasi Menu (Tombol)

| Fungsi | GPIO | Mode                                |
| :----- | :--: | :---------------------------------- |
| UP     |  33  | INPUT_PULLUP (active LOW)           |
| DOWN   | 36\* | INPUT (active LOW, external pullup) |
| SELECT | 39\* | INPUT (active LOW, external pullup) |
| BACK   |  5   | INPUT_PULLUP (active LOW)           |

> \*GPIO 36 dan 39 adalah input-only pin pada ESP32, cocok untuk tombol karena tidak butuh output.

### Buzzer

| Pin    | GPIO | Driver                                  |
| :----- | :--: | :-------------------------------------- |
| Buzzer |  13  | Via NPN transistor (2N2222) jika > 10mA |

---

## 6. Konektor Harness (Pinout)

Disarankan menggunakan konektor **GX16-9 (Aviation Connector)** atau **DB9** untuk koneksi ke motor:

| Pin | Sinyal             | Warna Kabel (saran) |
| :-: | :----------------- | :------------------ |
|  1  | Battery +12V (ACC) | Merah               |
|  2  | GND                | Hitam               |
|  3  | TPS Signal         | Hijau               |
|  4  | MIL Signal         | Kuning              |
|  5  | Starter Signal     | Biru                |
|  6  | Ignition Signal    | Putih               |
|  7  | Injector Signal    | Orange              |
|  8  | Temp Sensor Signal | Coklat              |
|  9  | K-Line (ECU Diag)  | Ungu                |

---

## 7. Ringkasan Pin Assignment

### ESP32 DevKit (Original)

| GPIO | Fungsi               | Tipe                |
| :--: | :------------------- | :------------------ |
|  4   | SD Card CS           | Output              |
|  5   | Button BACK          | Input (pullup)      |
|  12  | Fuel Pump Monitor    | Digital Input       |
|  13  | Buzzer               | Output              |
|  14  | Injector Pulse       | Digital Input (ISR) |
|  16  | K-Line RX            | UART RX             |
|  17  | K-Line TX            | UART TX             |
|  18  | SD Card SCK          | SPI                 |
|  19  | SD Card MISO         | SPI                 |
|  21  | OLED SDA             | I2C                 |
|  22  | OLED SCL             | I2C                 |
|  23  | SD Card MOSI         | SPI                 |
|  25  | MIL Signal           | Digital Input (ISR) |
|  26  | Starter Signal       | Digital Input       |
|  27  | Ignition Pulse (RPM) | Digital Input (ISR) |
|  32  | Engine Temp (NTC)    | Analog Input        |
|  33  | Button UP            | Input (pullup)      |
|  34  | Battery Voltage      | Analog Input        |
|  35  | TPS Signal           | Analog Input        |
|  36  | Button DOWN          | Input only          |
|  39  | Button SELECT        | Input only          |

**Pin tersisa (free):** GPIO 0, 2, 15 (boot strapping — hati-hati), GPIO 33 bisa dual-use.

---

## 8. Pin Assignment ESP32-S3 N16R8

Jika menggunakan ESP32-S3 N16R8, perhatikan perbedaan berikut:

- GPIO 25, 26 **tidak ada** di ESP32-S3 (itu DAC pin khusus ESP32 original)
- USB native di GPIO 19/20 — jangan pakai untuk fungsi lain
- Tidak ada Bluetooth Classic — gunakan BLE (sudah di-handle di kode)
- ADC2 tidak bisa dipakai saat WiFi aktif — gunakan ADC1 saja (GPIO 1-10)

### ESP32-S3 Pin Assignment Table

| GPIO | Fungsi               | Tipe                | Catatan                          |
| :--: | :------------------- | :------------------ | :------------------------------- |
|  1   | Battery Voltage      | Analog Input (ADC1) | Voltage divider 100k/22k         |
|  2   | TPS Signal           | Analog Input (ADC1) | Voltage divider 10k/4.7k         |
|  3   | Engine Temp (NTC)    | Analog Input (ADC1) | Voltage divider 10k/4.7k         |
|  4   | MIL Signal           | Digital Input (ISR) | Via optocoupler, active low      |
|  5   | Starter Signal       | Digital Input       | Via optocoupler, active low      |
|  6   | Ignition Pulse (RPM) | Digital Input (ISR) | Via optocoupler, frequency input |
|  7   | Injector Pulse       | Digital Input (ISR) | Via optocoupler, pulse width     |
|  8   | OLED SDA             | I2C Data            |                                  |
|  9   | OLED SCL             | I2C Clock           |                                  |
|  10  | Button UP            | Input (pullup)      | Active LOW                       |
|  11  | Button DOWN          | Input (pullup)      | Active LOW                       |
|  12  | Button SELECT        | Input (pullup)      | Active LOW                       |
|  13  | Button BACK          | Input (pullup)      | Active LOW                       |
|  14  | Buzzer               | PWM Output          | Via NPN transistor               |
|  15  | Fuel Pump Monitor    | Digital Input       | Via optocoupler (optional)       |
|  16  | K-Line RX            | UART RX             | Via level shifter                |
|  17  | K-Line TX            | UART TX             | Via NPN transistor               |
|  19  | USB D- (native)      | USB                 | ⚠️ Jangan pakai untuk lain       |
|  20  | USB D+ (native)      | USB                 | ⚠️ Jangan pakai untuk lain       |
|  35  | SD Card MOSI         | SPI                 |                                  |
|  36  | SD Card SCK          | SPI                 |                                  |
|  37  | SD Card MISO         | SPI                 |                                  |
|  38  | SD Card CS           | SPI                 |                                  |

**Pin tersisa (free):** GPIO 18, 21, 39-48 (bisa untuk ekspansi sensor tambahan, LED status, dll).

### Perbandingan Pin ESP32 vs ESP32-S3

| Fungsi         | ESP32 | ESP32-S3 | Catatan                       |
| :------------- | :---: | :------: | :---------------------------- |
| Battery ADC    |  34   |    1     | ADC1 di kedua board           |
| TPS ADC        |  35   |    2     | ADC1 di kedua board           |
| Temp ADC       |  32   |    3     | ADC1 di kedua board           |
| MIL Signal     |  25   |    4     | S3 tidak punya GPIO 25        |
| Starter Signal |  26   |    5     | S3 tidak punya GPIO 26        |
| Ignition (RPM) |  27   |    6     | ISR capable di kedua          |
| Injector Pulse |  14   |    7     | ISR capable di kedua          |
| Fuel Pump      |  12   |    15    | Optional                      |
| OLED SDA       |  21   |    8     | I2C                           |
| OLED SCL       |  22   |    9     | I2C                           |
| Button UP      |  33   |    10    |                               |
| Button DOWN    |  36   |    11    | ESP32: input-only             |
| Button SELECT  |  39   |    12    | ESP32: input-only             |
| Button BACK    |   5   |    13    |                               |
| Buzzer         |  13   |    14    |                               |
| K-Line RX      |  16   |    16    | Sama di kedua board           |
| K-Line TX      |  17   |    17    | Sama di kedua board           |
| SD CS          |   4   |    38    |                               |
| SD MOSI        |  23   |    35    |                               |
| SD MISO        |  19   |    37    | ESP32: shared dgn SPI default |
| SD SCK         |  18   |    36    | ESP32: shared dgn SPI default |

### Skema Koneksi ESP32-S3 (ASCII)

```
                    ┌─────────────────────────┐
                    │     ESP32-S3 N16R8       │
                    │                         │
  Batt Divider ────►│ GPIO 1  (ADC)           │
  TPS Divider  ────►│ GPIO 2  (ADC)           │
  Temp Divider ────►│ GPIO 3  (ADC)           │
                    │                         │
  Opto MIL     ────►│ GPIO 4  (ISR)           │
  Opto Starter ────►│ GPIO 5  (IN)            │
  Opto IGN     ────►│ GPIO 6  (ISR)           │
  Opto INJ     ────►│ GPIO 7  (ISR)           │
                    │                         │
  OLED SDA     ◄───►│ GPIO 8  (I2C)           │
  OLED SCL     ◄───►│ GPIO 9  (I2C)           │
                    │                         │
  BTN UP       ────►│ GPIO 10 (PULLUP)        │
  BTN DOWN     ────►│ GPIO 11 (PULLUP)        │
  BTN SELECT   ────►│ GPIO 12 (PULLUP)        │
  BTN BACK     ────►│ GPIO 13 (PULLUP)        │
                    │                         │
  Buzzer       ◄────│ GPIO 14 (PWM OUT)       │
  Opto Fuel    ────►│ GPIO 15 (IN)            │
                    │                         │
  K-Line RX    ────►│ GPIO 16 (UART2 RX)      │
  K-Line TX    ◄────│ GPIO 17 (UART2 TX)      │
                    │                         │
  SD MOSI      ◄────│ GPIO 35 (SPI)           │
  SD SCK       ◄────│ GPIO 36 (SPI)           │
  SD MISO      ────►│ GPIO 37 (SPI)           │
  SD CS        ◄────│ GPIO 38 (SPI)           │
                    │                         │
  USB D-       ◄───►│ GPIO 19 (USB native)    │
  USB D+       ◄───►│ GPIO 20 (USB native)    │
                    └─────────────────────────┘
```

> **Catatan:** Pin assignment ESP32-S3 sudah dikonfigurasi di file  
> `/configs/hardware/diagnostic_shield_v1_esp32s3.json`  
> Tidak perlu re-compile untuk ganti pin — cukup edit file JSON di SD card.

---

## 9. Checklist Proteksi

- [ ] Fuse 1A di jalur +12V input
- [ ] Reverse polarity diode (1N4007) di input power
- [ ] TVS diode 18V di input power
- [ ] Zener 3.3V di setiap input analog
- [ ] Kapasitor 100nF di setiap input analog (anti-noise)
- [ ] Optocoupler di setiap input digital 12V
- [ ] Bulk capacitor 1000uF di output buck converter
- [ ] Decoupling 10uF + 100nF di VCC ESP32
- [ ] Pull-up 10k di setiap output optocoupler
- [ ] Current limiting resistor 1k di setiap input optocoupler

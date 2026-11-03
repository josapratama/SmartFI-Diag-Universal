# 🔌 Skematik Perkabelan (Wiring) dan Proteksi Kelistrikan Otomotif

Panduan skematik kelistrikan tingkat otomotif (*automotive grade*), teknik isolasi optocoupler tegangan tinggi, proteksi lonjakan transien aki, serta tabel perbandingan pinout **ESP32 DevKit V1** dan **ESP32-S3**.

---

## 📑 Daftar Isi
1. [Karakteristik Kelistrikan Motor & Strategi Proteksi](#1-karakteristik-kelistrikan-motor--strategi-proteksi)
2. [Modul Catu Daya Otomotif (Power Supply Domain)](#2-modul-catu-daya-otomotif)
3. [Rangkaian Input Analog (Monitoring Pasif)](#3-rangkaian-input-analog-monitoring-pasif)
4. [Rangkaian Input Digital Berisolasi Optocoupler](#4-rangkaian-input-digital-berisolasi-optocoupler)
5. [Rangkaian Transceiver Jalur Komunikasi K-Line](#5-rangkaian-transceiver-jalur-komunikasi-k-line)
6. [Tabel Pinout Lengkap: ESP32 vs ESP32-S3](#6-tabel-pinout-lengkap-esp32-vs-esp32-s3)
7. [Standardisasi Pinout Konektor Harness GX16-9](#7-standardisasi-pinout-konektor-harness-gx16-9)

---

## 1. Karakteristik Kelistrikan Motor & Strategi Proteksi

Kelistrikan sepeda motor adalah salah satu lingkungan paling bising (*extremely noisy*) dan berbahaya bagi mikrokontroler:
- **Tegangan Berfluktuasi Ekstrem:** Aki drop hingga $7.5\text{V} - 9.0\text{V}$ saat dinamo starter menyala (*cranking sag*), dan naik ke $14.2\text{V} - 15.0\text{V}$ saat pengisian kiprok aktif.
- **Lonjakan Transien (*Load Dump*):** Pemutusan beban induktif dapat memicu lonjakan tegangan transien sesaat hingga puluhan volt.
- **Tegangan Induksi Koil Busi:** Jalur primer koil pengapian menghasilkan EMF balik mencapai **$300\text{V} - 450\text{V}$** saat medan magnet kolaps untuk meletupkan busi.

### 🛡️ Pilar Proteksi Wajib:
1. **Isolasi Optik (Optocoupler):** Seluruh sinyal digital berdenyut dipisahkan secara fisik dengan cahaya inframerah. Tidak ada tembaga yang menghubungkan koil pengapian langsung ke pin GPIO mikrokontroler.
2. **Penjepit Tegangan (Zener Clamping 3.3V):** Melindungi gerbang ADC ESP32 agar tidak pernah kemasukan tegangan $>3.3\text{V}$.
3. **Peredam Transient Voltage Suppressor (TVS 18V):** Memotong spike tegangan tinggi sebelum masuk ke regulator.

---

## 2. Modul Catu Daya Otomotif

```
Aki Motor +12V (Kunci Kontak ACC)
    │
   ┌┴┐
   │ │ FUSE 1A (Fast Blow)
   └┬┘
    ├───────────────► [ Katoda TVS 1.5KE18CA (18V) ] ──► GND
    │
   ┌┴┐
   │▲│ DIODA 1N4007 (Proteksi Polaritas Terbalik)
   └┬┘
    │
    ├───────────────► [ Kapasitor Elco 1000µF 25V Low-ESR ] ──► GND
    │
    ▼
┌───────────────────────┐
│ Buck Converter MP1584 │ (Input: 7V - 28V)
│ / LM2596 Step-Down    │
└───────────┬───────────┘
            │
            ├─► Output +5.0V Tergulasi ──► VIN ESP32, VCC OLED, Buzzer
            │
           ┌┴┐
           │ │ Kapasitor 100nF (Filter Noise HF)
           └┬┘
            │
           GND Common Rangka
```

---

## 3. Rangkaian Input Analog (Monitoring Pasif)

### A. Monitor Tegangan Aki (CH_BATT)
Menggunakan pembagi tegangan presisi dengan rasio $\approx 0.180$:
```
Aki (+12V) ───[ Resistor R1: 100kΩ ]───┬───► Ke Pin ADC (GPIO 34 / GPIO 1)
                                       │
                                       ├───[ Dioda Zener 3.3V ]───► GND
                                       │
                                       ├───[ Kapasitor C1: 100nF ]─► GND
                                       │
                                      ┌┴┐
                                      │ │ Resistor R2: 22kΩ (1% Presisi)
                                      └┬┘
                                       │
                                      GND
```
- **Kalkulasi:** Saat aki $15.0\text{V}$, tegangan ke ADC $= 15.0 \times \frac{22}{100 + 22} = 2.70\text{V}$ (Sangat aman di bawah batas $3.3\text{V}$).
- **Formula Firmware:** $V_{\text{batt}} = V_{\text{adc}} \times 5.545$

### B. Sensor Bukaan Gas / TPS (CH_TPS)
Tegangan sensor TPS pabrikan berkisar $0.5\text{V}$ (idle) sampai $4.5\text{V}$ (WOT):
```
Sinyal TPS (0-5V) ───[ Resistor R1: 10kΩ ]───┬───► Ke Pin ADC (GPIO 35 / GPIO 2)
                                             │
                                             ├───[ Dioda Zener 3.3V ]───► GND
                                             │
                                             ├───[ Kapasitor C1: 100nF ]─► GND
                                             │
                                            ┌┴┐
                                            │ │ Resistor R2: 4.7kΩ
                                            └┬┘
                                             │
                                            GND
```
- **Kalkulasi:** Saat TPS maksimum $5.0\text{V}$, tegangan ke ADC $= 5.0 \times \frac{4.7}{10 + 4.7} = 1.60\text{V}$.

### C. Sensor Suhu Mesin NTC / ECT (CH_TEMP)
Sensor NTC otomotif memiliki karakteristik resistansi menurun saat suhu meningkat:
```
Sinyal Suhu ECT ───[ Resistor R1: 10kΩ ]───┬───► Ke Pin ADC (GPIO 32 / GPIO 3)
                                           │
                                           ├───[ Dioda Zener 3.3V ]───► GND
                                           │
                                           ├───[ Kapasitor C1: 100nF ]─► GND
                                           │
                                          ┌┴┐
                                          │ │ Resistor R2: 4.7kΩ
                                          └┬┘
                                           │
                                          GND
```

---

## 4. Rangkaian Input Digital Berisolasi Optocoupler

Rangkaian ini digunakan untuk **Pulser RPM / Koil Pengapian (CH_IGN)**, **Sinyal Injektor (CH_INJ)**, **Starter (CH_STR)**, dan **Lampu Indikator Kerusakan MIL (CH_MIL)**:

```
Sinyal Motor (+12V Pulsa)
         │
        ┌┴┐
        │ │ Resistor Pembatas Arus R1 (1kΩ 1W)
        └┬┘
         │
         ├──► [ Anoda Dioda Pengaman 1N4148 ]
         ▼
     ┌───────────────┐
     │ 1 (Anoda)     │
     │     PC817     │  (Kolektor) 4 ────► +3.3V VCC ESP32
     │               │
     │ 2 (Katoda)    │  (Emitter)  3 ────┬───► Ke Pin GPIO ESP32
     └───────┬───────┘                   │
             │                          ┌┴┐
             ▼                          │ │ Resistor Pull-Down 10kΩ
         GND Motor                      └┬┘
                                         │
                                        GND ESP32
```
- Ketika pulsa 12V hadir, LED internal optocoupler menyala, phototransistor menghantar, dan pin GPIO membaca logika HIGH ($3.3\text{V}$). Tidak ada lonjakan listrik motor yang bisa menyeberang ke ESP32.

---

## 5. Rangkaian Transceiver Jalur Komunikasi K-Line

K-Line (ISO 9141 / ISO 14230) adalah jalur serial satu kawat (*single-wire bi-directional half-duplex*) bertegangan level aki 12V:

```
                    Aki +12V
                       │
                      ┌┴┐
                      │ │ Resistor Pull-Up 510Ω (1W)
                      └┬┘
                       │
K-Line ECU Motor ──────┼────────────────────────┬─────────────────────┐
                       │                        │                     │
                       │                       ┌┴┐                    │
                       │                       │ │ R_div: 10kΩ        │
                       ▼                       └┬┘                    ▼
               [ Kolektor Q1 ]                  │             [ Dioda Zener 3.3V ]
             Transistor NPN 2N2222              ├──────────►          │
                 (TX Driver)                    │                     GND
                       │                       ┌┴┐
Pin TX ESP32 ──[1kΩ]──►│ Basis                 │ │ R_div: 4.7kΩ
(GPIO 17/16)           │                       └┬┘
                       ▼                        │
                  Emitter Q1                   GND
                       │                        │
                      GND                       └──────────► Pin RX ESP32
                                                             (GPIO 16/17)
```

---

## 6. Tabel Pinout Lengkap: ESP32 vs ESP32-S3

Firmware secara dinamis memetakan pin berdasarkan konfigurasi JSON hardware:

| Saluran Sinyal | Tipe Driver | ESP32 DevKit V1 | ESP32-S3 N16R8 | Keterangan Sambungan |
| :--- | :---: | :---: | :---: | :--- |
| **CH_BATT** | Analog ADC | **GPIO 34** | **GPIO 1** | Melalui Divider 100k/22k |
| **CH_TPS** | Analog ADC | **GPIO 35** | **GPIO 2** | Melalui Divider 10k/4.7k |
| **CH_TEMP** | Analog ADC | **GPIO 32** | **GPIO 3** | Sensor Suhu Mesin NTC |
| **CH_MIL** | Digital Input | **GPIO 25** | **GPIO 4** | Lampu MIL via Optocoupler |
| **CH_IGN** | Digital Input | **GPIO 27** | **GPIO 5** | Pulser / RPM via Optocoupler |
| **CH_INJ** | Digital Input | **GPIO 14** | **GPIO 6** | Pulsa Injektor via Opto |
| **CH_STR** | Digital Input | **GPIO 12** | **GPIO 7** | Sinyal Tombol Starter via Opto |
| **K_LINE_RX** | Hardware UART2 | **GPIO 16** | **GPIO 17** | Serial Data dari ECU |
| **K_LINE_TX** | Hardware UART2 | **GPIO 17** | **GPIO 16** | Serial Data ke ECU |
| **I2C_SDA** | Wire Master | **GPIO 21** | **GPIO 8** | Display OLED SSD1306 |
| **I2C_SCL** | Wire Master | **GPIO 22** | **GPIO 9** | Display OLED SSD1306 |
| **SD_CS** | SPI Chip Select | **GPIO 4** | **GPIO 38** | MicroSD Adapter Module |
| **SD_MOSI** | SPI Data Out | **GPIO 23** | **GPIO 35** | MicroSD MOSI |
| **SD_MISO** | SPI Data In | **GPIO 19** | **GPIO 37** | MicroSD MISO |
| **SD_SCK** | SPI Clock | **GPIO 18** | **GPIO 36** | MicroSD Clock |
| **BUZZER** | Digital Output | **GPIO 13** | **GPIO 21** | Active Buzzer 5V |
| **BTN_UP** | Input Pullup | **GPIO 33** | **GPIO 10** | Tombol Navigasi Menu |
| **BTN_DOWN** | Input Pullup | **GPIO 26** | **GPIO 11** | Tombol Navigasi Menu |
| **BTN_SELECT**| Input Pullup | **GPIO 15** | **GPIO 12** | Tombol Pilih / OK |

---

## 7. Standardisasi Pinout Konektor Harness GX16-9

Untuk kemudahan pemasangan soket kabel antara unit scanner dan motor, gunakan standarisasi urutan pin berikut:

```
         KONEKTOR AVIATION GX16 9-PIN (Tampak Depan Soket)
                             [1] Aki +12V (ACC)
                        [8]      [2] Ground Aki (GND)
                    [7]              [3] Sinyal Sensor TPS
                   [6]        [9]     [4] Sinyal Sensor Suhu (ECT)
                        [5]      [4]  [5] Pulsa Koil/Pulser (RPM)
                             GND      [6] Sinyal Injektor
                                      [7] Sinyal Lampu MIL
                                      [8] Sinyal K-Line ECU
                                      [9] Sinyal Starter
```

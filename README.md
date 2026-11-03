# 🛵 SmartFI-Diag-Universal: ESP32 Smart FI Motorcycle Diagnostic Tool

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![Target: ESP32 / ESP32-S3](https://img.shields.io/badge/Target-ESP32%20%2F%20ESP32--S3-red.svg)](https://www.espressif.com/)
[![Framework: Arduino C++](https://img.shields.io/badge/Framework-Arduino%20C%2B%2B-00979D.svg)](https://www.arduino.cc/)
[![Protocol: KWP2000 Fast-Init](https://img.shields.io/badge/Protocol-ISO%2014230%20(K--Line)-blue.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

**SmartFI-Diag-Universal** adalah platform scanner dan diagnostik sepeda motor injeksi (EFI) universal berbasis **ESP32 & ESP32-S3**. Dirancang untuk teknisi bengkel, mekanik, dan penghobi otomotif untuk memantau kesehatan sensor, membaca kode kerusakan (DTC), mengkalibrasi bukaan katup gas (TPS), serta mengukur performa akselerasi secara real-time melalui layar OLED dan Dashboard Web WiFi.

---

## 🌟 Fitur Utama

- **Kompatibilitas Luas (Tier 1 hingga Tier 4):**
  - **Tier 1 (Universal):** Evaluasi aki 12V, pengisian kiprok, dan analisis drop tegangan dinamo starter (*cranking health*).
  - **Tier 2 (Semi-Universal):** Pembacaan pasif sensor TPS, suhu mesin NTC, pulser RPM, dan dekoder kode kedipan lampu MIL.
  - **Tier 3 (Brand Specific):** Estimasi debit konsumsi bahan bakar (injektor) dan starter halus (Honda ACG / Yamaha SMG).
  - **Tier 4 (Full ECU Protocol):** Komunikasi dua arah digital via **K-Line (ISO 14230 / KWP2000 Fast-Init)** untuk membaca *live PID*, membaca kode kerusakan memori ECU, dan menghapus DTC (*Clear DTC*).
- **Arsitektur Dual-Core FreeRTOS:**
  - **Core 1 (Real-Time 10ms):** Sampling sensor ADC (baterai, TPS, suhu), filter digital EMA, penghitungan RPM pulser, dan antarmuka display OLED.
  - **Core 0 (Background Task):** Task protokol K-Line non-blocking (durasi handshake $2.5\text{s}$ tidak membekukan pembacaan sensor) dan Web Dashboard Server.
- **Penyimpanan Ganda Tangguh (Dual Storage):**
  - Manajemen berkas otomatis via `FSManager` (MicroSD Card dengan *failover* transparan ke memori internal **SPIFFS** jika kartu SD lepas/rusak).
- **Kalibrasi Interaktif TPS & Deteksi Jalur Karbon Aus:**
  - Simpan tegangan titik nol (idle) dan gas penuh (WOT) langsung ke Flash NVS melalui tombol Web Dashboard.
  - Deteksi otomatis lonjakan voltase abnormal ($>1.2\text{V}$) akibat lapisan karbon gesek TPS yang tergores/aus.
- **Virtual Dyno / Timer Akselerasi 0–60 km/h:**
  - Pengukur waktu akselerasi otomatis berbasis sinyal GPS atau rasio putaran RPM, dilengkapi pencatatan rekor terbaik (*Personal Best* tersimpan di NVS) dan kecepatan puncak (*top speed*).
- **Dual-Mode WiFi (SoftAP + Workshop STA):**
  - Memancarkan Access Point mandiri (`SmartFI-Diag`) sekaligus dapat terhubung ke jaringan WiFi bengkel lokal.
- **Web Dashboard & Unduh Log CSV:**
  - Pemantauan telemetri nirkabel tanpa instalasi aplikasi tambahan (cukup buka browser smartphone `http://192.168.4.1`) dan unduh log telemetri berkala.

---

## 📐 Arsitektur Sistem

```
┌────────────────────────────────────────────────────────────────────────┐
│                   SEPEDA MOTOR INJEKSI (EFI)                           │
│  [Aki 12V]  [TPS 0-5V]  [Suhu ECT]  [Pulser RPM]  [Lampu MIL]  [K-Line]│
└─────┬───────────┬───────────┬────────────┬─────────────┬───────────┬───┘
      │           │           │            │             │           │
┌─────▼───────────▼───────────▼─────┐ ┌────▼─────────────▼───────────┴───┐
│     RANGKAIAN INPUT ANALOG        │ │     ISOLASI OPTOCOUPLER PC817    │
│  (Divider + Zener 3.3V Clamp)     │ │   (Proteksi Lonjakan 400V+)      │
└─────────────────┬─────────────────┘ └──────────────────┬───────────────┘
                  │                                      │
                  ▼                                      ▼
┌────────────────────────────────────────────────────────────────────────┐
│                   ESP32 / ESP32-S3 MIKROKONTROLER                      │
│                                                                        │
│   ┌───────────────────────────────┐  ┌──────────────────────────────┐  │
│   │   CORE 1 (High-Speed 10ms)    │  │   CORE 0 (Background & Net)  │  │
│   ├───────────────────────────────┤  ├──────────────────────────────┤  │
│   │ • Sampling ADC & Filter EMA   │  │ • K-Line KWP2000 Protocol    │  │
│   │ • Kalkulasi Bukaan Gas TPS %  │  │ • Async Web Server (Port 80) │  │
│   │ • Cranking Health Analyzer    │  │ • WebSocket Telemetry Stream │  │
│   │ • 0-60 km/h Accel Stopwatch   │  │ • Preferences (NVS Storage)  │  │
│   │ • UI Display OLED SSD1306     │  │ • MicroSD / SPIFFS Logging   │  │
│   └───────────────────────────────┘  └──────────────────────────────┘  │
└──────────────────┬─────────────────────────────────┬───────────────────┘
                   │                                 │
                   ▼                                 ▼
         ┌──────────────────┐              ┌──────────────────┐
         │ Layar OLED 0.96" │              │  Smartphone/PC   │
         │ (Tampilan Stang) │              │ (Web Dashboard)  │
         └──────────────────┘              └──────────────────┘
```

---

## 🔌 Pemetaan Pinout (Hardware Mapping)

| Saluran Sinyal | Tipe Driver | ESP32 DevKit V1 | ESP32-S3 N16R8 | Keterangan Sambungan |
| :--- | :---: | :---: | :---: | :--- |
| **CH_BATT** | Analog ADC | **GPIO 34** | **GPIO 1** | Melalui Divider 100k/22k |
| **CH_TPS** | Analog ADC | **GPIO 35** | **GPIO 2** | Melalui Divider 10k/4.7k |
| **CH_TEMP** | Analog ADC | **GPIO 32** | **GPIO 3** | Sensor Suhu Mesin NTC |
| **CH_MIL** | Digital Input | **GPIO 25** | **GPIO 4** | Lampu MIL via Optocoupler |
| **CH_IGN** | Digital Input | **GPIO 27** | **GPIO 5** | Pulser / RPM via Optocoupler |
| **CH_INJ** | Digital Input | **GPIO 14** | **GPIO 6** | Pulsa Injektor via Opto |
| **CH_STR** | Digital Input | **GPIO 12** | **GPIO 7** | Sinyal Starter via Opto |
| **K_LINE_RX** | Hardware UART2 | **GPIO 16** | **GPIO 17** | Serial Data dari ECU |
| **K_LINE_TX** | Hardware UART2 | **GPIO 17** | **GPIO 16** | Serial Data ke ECU |
| **I2C_SDA** | Wire Master | **GPIO 21** | **GPIO 8** | Display OLED SSD1306 |
| **I2C_SCL** | Wire Master | **GPIO 22** | **GPIO 9** | Display OLED SSD1306 |
| **SD_CS** | SPI Chip Select | **GPIO 4** | **GPIO 38** | MicroSD Adapter Module |
| **SD_MOSI** | SPI Data Out | **GPIO 23** | **GPIO 35** | MicroSD MOSI |
| **SD_MISO** | SPI Data In | **GPIO 19** | **GPIO 37** | MicroSD MISO |
| **SD_SCK** | SPI Clock | **GPIO 18** | **GPIO 36** | MicroSD Clock |
| **BUZZER** | Digital Output | **GPIO 13** | **GPIO 21** | Active Buzzer 5V |

---

## 📁 Struktur Repositori

```
SmartFI-Diag-Universal/
├── configs/                  → Konfigurasi berbasis data JSON
│   ├── hardware/             → Pemetaan pinout board ESP32 & ESP32-S3
│   ├── vehicles/             → Profil motor hierarkis (Base -> Family -> Model)
│   └── dtc/                  → Kamus kode kerusakan (Honda, Yamaha, dll)
├── data/                     → Aset Web Dashboard (HTML/JS/CSS) untuk SPIFFS
├── docs/                     → Dokumentasi komprehensif 8 modul
│   ├── 01_PANDUAN_MULAI_DAN_BELANJA_BOM.md   → Panduan perakitan & daftar belanja BOM
│   ├── 02_SKEMA_WIRING_DAN_PROTEKSI.md       → Skematik kelistrikan otomotif & optocoupler
│   ├── 03_CATALOG_DAN_PROFIL_MOTOR.md        → Taksonomi hierarki profil kendaraan JSON
│   ├── 04_MATRIKS_FITUR_DAN_TIER.md          → Klasifikasi Tier 1-4 & Feature Gating
│   ├── 05_RENCANA_PENGUJIAN_LAB_DAN_MOTOR.md → Pengujian meja lab hingga uji motor
│   ├── 06_STANDAR_KESELAMATAN_KERJA.md       → Perlindungan spike 400V & derau busi
│   ├── 07_PROTOKOL_KLINE_DAN_OBD_DIAGNOSTIK.md→ Implementasi KWP2000 Fast-Init & DTC
│   └── 08_KALIBRASI_TPS_DAN_PENGUKURAN_0_60.md→ Kalibrasi TPS NVS & timer 0-60 km/h
├── include/                  → Header struktur data C++
├── src/                      → Source code firmware PlatformIO
│   ├── app/                  → UI OLED Manager & User Interface
│   ├── core/                 → Kernel config loader, FSManager, channel manager
│   ├── drivers/              → Driver ADC, digital interrupt, filter EMA
│   ├── services/             → Modul sensor (TPS, Aki, RPM, Temp, DiagEngine)
│   └── main.cpp              → Titik masuk utama & task FreeRTOS
├── platformio.ini            → Konfigurasi build PlatformIO
├── min_spiffs.csv            → Tabel partisi flash memori ESP32
├── LICENSE                   → Lisensi open source MIT
└── README.md
```

---

## 🚀 Panduan Memulai Cepat (Quick Start)

### 1. Prasyarat
- Komputer dengan **Visual Studio Code** dan ekstensi **PlatformIO IDE**.
- Kabel data Micro-USB atau USB Type-C.

### 2. Kompilasi & Flash Firmware
1. Buka folder `project/motor` di VS Code.
2. Buka berkas `platformio.ini`, pilih environment sesuai board yang Anda gunakan:
   - Board **ESP32 DevKit V1**: `[env:esp32dev]`
   - Board **ESP32-S3 DevKitC-1**: `[env:esp32-s3-devkitc-1]`
3. Hubungkan board ke port USB komputer.
4. Klik tombol **PlatformIO: Build** (ikon centang), lalu **PlatformIO: Upload** (ikon panah).
5. Unggah antarmuka web ke partisi flash internal dengan memilih task `Upload Filesystem Image`.

---

## 📶 Koneksi Web Dashboard & Telemetri WiFi

1. Nyalakan alat dengan mencolokkan ke aki motor (atau power supply 12V).
2. Hubungkan smartphone ke Access Point WiFi:
   - **SSID:** `SmartFI-Diag` (atau sesuai nama project di hardware JSON)
   - **Password:** `smartfi123`
3. Buka browser web (Chrome / Safari / Firefox) di smartphone dan tuju alamat:
   ```text
   http://192.168.4.1
   ```
4. Dashboard web interaktif menyediakan tab pemantauan:
   - **Live Telemetry:** Voltase aki, RPM, bukaan TPS, suhu mesin, laju bensin.
   - **Health Score:** Skor kesehatan komponen mesin secara komputasi.
   - **TPS Calib:** Kalibrasi titik nol dan WOT secara interaktif dengan satu sentuhan.
   - **0-60 Timer:** Instrumen dyno akselerasi drag 0–60 km/jam.
   - **SD Logs:** Manajer pengunduhan rekaman file CSV.
   - **DTC & Wizard:** Pembacaan kode kerusakan dan asisten panduan perbaikan gejala motor.

---

## 📚 Indeks Dokumentasi Lengkap

Untuk panduan teknis mendalam, silakan merujuk pada berkas di folder `docs/`:
- [01. Panduan Mulai & Daftar Belanja Komponen (BOM)](docs/01_PANDUAN_MULAI_DAN_BELANJA_BOM.md)
- [02. Skematik Perkabelan dan Proteksi Kelistrikan Otomotif](docs/02_SKEMA_WIRING_DAN_PROTEKSI.md)
- [03. Sistem Katalog dan Hierarki Profil Sepeda Motor](docs/03_CATALOG_DAN_PROFIL_MOTOR.md)
- [04. Matriks Fitur dan Klasifikasi Tingkatan Kompatibilitas](docs/04_MATRIKS_FITUR_DAN_TIER.md)
- [05. Rencana Pengujian Bertahap (Laboratorium hingga Motor)](docs/05_RENCANA_PENGUJIAN_LAB_DAN_MOTOR.md)
- [06. Standar Keselamatan Kerja & Pencegahan Kerusakan Komponen](docs/06_STANDAR_KESELAMATAN_KERJA.md)
- [07. Protokol K-Line (ISO 14230 / KWP2000) dan Diagnostik ECU](docs/07_PROTOKOL_KLINE_DAN_OBD_DIAGNOSTIK.md)
- [08. Kalibrasi Interaktif TPS & Virtual Dyno Timer Akselerasi](docs/08_KALIBRASI_TPS_DAN_PENGUKURAN_0_60.md)

---

## 📄 Lisensi

Proyek ini dirilis di bawah lisensi terbuka [MIT License](LICENSE). Bebas digunakan, dimodifikasi, dan dikembangkan untuk keperluan edukasi, bengkel motor, maupun komersial.

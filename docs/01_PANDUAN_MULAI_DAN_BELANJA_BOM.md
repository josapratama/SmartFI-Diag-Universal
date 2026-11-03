# 📘 Panduan Memulai & Daftar Belanja Komponen (BOM)

Panduan komprehensif dari awal (tahap perakitan nol) hingga alat siap digunakan, disertai rincian daftar belanja komponen (*Bill of Materials / BOM*), estimasi biaya, dan petunjuk setup lingkungan kerja PlatformIO.

---

## 📑 Daftar Isi
1. [Pengantar Smart FI Diagnostic Tool](#1-pengantar-smart-fi-diagnostic-tool)
2. [Daftar Belanja Komponen (BOM) & Estimasi Biaya](#2-daftar-belanja-komponen-bom--estimasi-biaya)
3. [Alat Kerja yang Diperlukan](#3-alat-kerja-yang-diperlukan)
4. [Instalasi Software & PlatformIO](#4-instalasi-software--platformio)
5. [Struktur Folder & Penyiapan File Konfigurasi](#5-struktur-folder--penyiapan-file-konfigurasi)
6. [Langkah Kompilasi dan Flash Firmware](#6-langkah-kompilasi-dan-flash-firmware)
7. [Pengujian Awal Pasca-Flash](#7-pengujian-awal-pasca-flash)

---

## 1. Pengantar Smart FI Diagnostic Tool

**ESP32 Smart FI Diagnostic Tool** adalah platform pemindai dan penganalisa kesehatan sistem injeksi bahan bakar (*Electronic Fuel Injection / EFI*) sepeda motor universal. Alat ini mampu bekerja pada motor injeksi generasi awal (hanya sinyal analog/pulsa pasif) hingga motor modern dengan protokol komunikasi digital ECU K-Line (ISO 14230 / KWP2000).

### Keunggulan Sistem:
- **Universal Multi-Brand:** Mendukung Honda PGM-FI, Yamaha BlueCore, Suzuki Fi, dan Kawasaki DFI melalui konfigurasi profil modular berbasis JSON.
- **Dual-Core FreeRTOS:** Inti sampling sensor (10ms) dan antarmuka display berjalan di Core 1, sementara protokol ECU K-Line dan WiFi Web Dashboard berjalan di Core 0.
- **Penyimpanan Ganda (Dual Storage):** Data log dan konfigurasi tersimpan di MicroSD, dengan otomatis *fallback* ke memori internal SPIFFS jika kartu SD tidak terpasang.
- **Kalibrasi Interaktif & Virtual Dyno:** Mendukung kalibrasi voltase TPS langsung via browser dan timer akselerasi 0–60 km/jam.

---

## 2. Daftar Belanja Komponen (BOM) & Estimasi Biaya

Seluruh komponen mudah didapatkan di toko komponen elektronik lokal atau marketplace daring:

### A. Modul Inti & Tampilan
| No | Komponen | Spesifikasi / Tipe | Qty | Estimasi Harga | Keterangan |
| :-: | :--- | :--- | :-: | :-: | :--- |
| 1 | **Mikrokontroler Utama** | ESP32-S3 N16R8 DevKit atau ESP32 DevKit V1 | 1 | Rp 45.000 - Rp 85.000 | Otak pemrosesan |
| 2 | **Layar Display** | OLED 0.96 inci SSD1306 128x64 I2C | 1 | Rp 25.000 | Tampilan lokal motor |
| 3 | **Modul Penyimpanan** | MicroSD Card Adapter Module (SPI) | 1 | Rp 8.000 | Slot kartu memori |
| 4 | **Kartu Memori** | MicroSD Card 4GB - 16GB (Format FAT32) | 1 | Rp 28.000 | Penyimpan log CSV & JSON |
| 5 | **Regulator Daya** | DC-DC Step-Down Buck Converter (MP1584 / LM2596) | 1 | Rp 12.000 | Mengubah 12V aki ke 5V stabil |
| 6 | **Konektor Harness** | Soket Aviation GX16 9-Pin (Male + Female) | 1 set | Rp 25.000 | Konektor lepas-pasang ke motor |
| 7 | **Tombol Navigasi** | Tactile Push Button 4-pin 6x6mm | 4 | Rp 4.000 | Tombol Up, Down, Select, Back |
| 8 | **Audio Alert** | Active Buzzer 5V | 1 | Rp 3.000 | Alarm overcharge/overheat |

### B. Komponen Proteksi & Sensor Input
| No | Komponen | Nilai / Rating | Qty | Fungsi Utama |
| :-: | :--- | :--- | :-: | :--- |
| 9 | **Optocoupler** | PC817 (DIP-4) | 4 | Isolasi pulsa RPM, Injector, Starter, MIL |
| 10 | **Transistor NPN** | 2N2222 atau BC547 | 2 | Driver K-Line Transceiver & Buzzer |
| 11 | **Diode Zener** | 3.3V 0.5W | 3 | Penjepit tegangan aman ADC (Clamp) |
| 12 | **TVS Diode** | 1.5KE18CA atau P6KE18A (18V Bi-directional) | 1 | Peredam lonjakan spike transien aki |
| 13 | **Diode Penyearah** | 1N4007 (1A 1000V) | 1 | Proteksi polaritas aki terbalik |
| 14 | **Sekring Pengaman** | Fuse Tabung / Blade 1A + Holder | 1 | Proteksi hubung singkat (short circuit) |
| 15 | **Kapasitor Elco** | 1000µF 25V Low-ESR | 1 | Peredam drop tegangan saat starter |
| 16 | **Kapasitor Keramik**| 100nF (0.1µF) 50V | 6 | Filter derau frekuensi tinggi pada ADC |
| 17 | **Resistor 1/4W** | 100kΩ (1%), 22kΩ (1%) | @1 | Pembagi tegangan monitor aki |
| 18 | **Resistor 1/4W** | 10kΩ, 4.7kΩ (1%) | @4 | Pembagi tegangan sensor TPS & NTC |
| 19 | **Resistor 1/4W** | 1kΩ, 510Ω | @5 | Pembatas arus LED optocoupler & K-Line |
| 20 | **Papan Rangkaian** | PCB Lubang / Dot Matrix 7x9cm | 1 | Tempat menyolder komponen |
| 21 | **Kabel & Casing** | Kabel silikon AWG22/24 + Box X6/3D Print | 1 set | Perakitan rapi tahan guncangan |

**Total Estimasi Biaya:** **$\approx \text{Rp } 250.000 - \text{Rp } 350.000$** (Sangat ekonomis dibanding scanner pabrikan yang berharga jutaan rupiah).

---

## 3. Alat Kerja yang Diperlukan
- Solder listrik ($40\text{W} - 60\text{W}$) + timah solder berkualitas.
- Multimeter digital (wajib untuk memeriksa tegangan sebelum ESP32 dipasang).
- Tang potong kawat, tang kupas kabel, dan pinset.
- Lem bakar (*hot glue*) atau spacer baut PCB untuk isolasi guncangan di dalam casing.

---

## 4. Instalasi Software & PlatformIO

1. **Unduh dan Pasang Visual Studio Code:**
   - Unduh installer resmi dari [code.visualstudio.com](https://code.visualstudio.com/).
2. **Pasang Ekstensi PlatformIO IDE:**
   - Buka VS Code $\rightarrow$ Menu **Extensions** (Ctrl+Shift+X).
   - Ketik **PlatformIO IDE** lalu klik **Install**.
3. **Pasang Driver USB-Serial Komputer:**
   - Pastikan driver CH340, CP2102, atau USB JTAG Espressif terpasang agar port COM terbaca.

---

## 5. Struktur Folder & Penyiapan File Konfigurasi

Pastikan susunan project tetap sesuai struktur standar:
```text
motor/
├── configs/
│   ├── hardware/       → Konfigurasi pinout board (ESP32 / ESP32-S3)
│   ├── vehicles/       → Profil JSON motor (Beat, Vario, NMAX, dll)
│   └── dtc/            → Database kode kerusakan (honda_dtc.json, yamaha_dtc.json)
├── data/               → Aset Web Dashboard (index.html) untuk SPIFFS
├── docs/               → Dokumentasi teknis lengkap
├── include/            → Header struktur data
└── src/                → Source code firmware PlatformIO
```

> [!TIP]
> Salin seluruh isi folder `configs/` dan `data/` ke dalam kartu MicroSD (format FAT32). Dengan implementasi `FSManager`, alat juga tetap bisa berjalan dari memori internal SPIFFS jika MicroSD tidak dimasukkan.

---

## 6. Langkah Kompilasi dan Flash Firmware

1. Buka folder `project/motor` di VS Code melalui menu `File -> Open Folder...`.
2. Buka berkas [`platformio.ini`](file:///d:/coding/raspberry%20pi/project/motor/platformio.ini).
3. Pilih environment target yang sesuai:
   - Untuk board **ESP32 DevKit V1**: Default environment `[env:esp32dev]`.
   - Untuk board **ESP32-S3**: Gunakan environment `[env:esp32-s3-devkitc-1]`.
4. Hubungkan board ESP32 ke port USB PC.
5. Klik ikon **PlatformIO: Build** (tanda centang di status bar bawah) untuk memverifikasi kompilasi.
6. Klik ikon **PlatformIO: Upload** (tanda panah kanan) untuk memprogram firmware ke chip ESP32.
7. Untuk mengunggah antarmuka web ke SPIFFS: Jalankan perintah task PlatformIO `Upload Filesystem Image`.

---

## 7. Pengujian Awal Pasca-Flash

1. Buka **Serial Monitor** pada kecepatan baudrate **115200**.
2. Tekan tombol RESET pada ESP32.
3. Anda akan melihat log pembuka:
   ```text
   === Smart FI Diagnostic Tool ===
   System Starting...
   [SD] SD Card initialized successfully.
   Profile Loaded: Honda Beat Fi (2014)
   WiFi AP: SmartFI-Diag
   Dashboard: http://192.168.4.1
   === System Ready ===
   ```
4. Layar OLED akan menyala menampilkan logo pembuka dan berpindah ke layar pemantauan sensor real-time.


# Panduan Lengkap: Dari Nol Sampai Jadi

Dokumen ini menjelaskan langkah-langkah dari awal (belum punya apa-apa) sampai alat bisa dipakai di motor.

---

## Daftar Belanja Komponen

### Komponen Utama

| No  | Komponen                      | Jumlah | Estimasi Harga | Keterangan                                      |
| :-: | :---------------------------- | :----: | :------------: | :---------------------------------------------- |
|  1  | ESP32-S3 N16R8 DevKitC-1      |   1    |   Rp 85.000    | Board utama (atau ESP32 DevKit biasa Rp 45.000) |
|  2  | OLED SSD1306 128x64 (I2C)     |   1    |   Rp 25.000    | Layar display                                   |
|  3  | MicroSD Card Module (SPI)     |   1    |    Rp 8.000    | Penyimpanan log & config                        |
|  4  | MicroSD Card 4-16GB           |   1    |   Rp 25.000    | FAT32 format                                    |
|  5  | Buck Converter LM2596/MP1584  |   1    |   Rp 12.000    | 12V → 5V                                        |
|  6  | Optocoupler PC817             |   5    |    Rp 5.000    | Isolasi sinyal 12V                              |
|  7  | Push Button 4-pin             |   4    |    Rp 4.000    | Navigasi menu                                   |
|  8  | Buzzer 5V aktif               |   1    |    Rp 3.000    | Alarm/notifikasi                                |
|  9  | Konektor GX16-9 (male+female) | 1 set  |   Rp 25.000    | Konektor ke motor                               |

### Komponen Proteksi & Pendukung

| No  | Komponen                        | Jumlah | Keterangan                     |
| :-: | :------------------------------ | :----: | :----------------------------- |
| 10  | Resistor 100kΩ                  |   1    | Divider baterai                |
| 11  | Resistor 22kΩ                   |   1    | Divider baterai                |
| 12  | Resistor 10kΩ                   |   7    | Divider TPS/Temp + pullup opto |
| 13  | Resistor 4.7kΩ                  |   2    | Divider TPS/Temp               |
| 14  | Resistor 1kΩ                    |   5    | Current limiter optocoupler    |
| 15  | Resistor 510Ω                   |   1    | K-Line pull-up                 |
| 16  | Zener Diode 3.3V                |   3    | Proteksi ADC                   |
| 17  | Diode 1N4007                    |   1    | Reverse polarity               |
| 18  | TVS Diode 18V                   |   1    | Spike protection               |
| 19  | Kapasitor 1000uF/25V            |   1    | Bulk filter power              |
| 20  | Kapasitor 100nF                 |   5    | Decoupling/filter              |
| 21  | Kapasitor 10uF                  |   2    | Decoupling                     |
| 22  | Transistor NPN 2N2222           |   2    | Driver buzzer + K-Line TX      |
| 23  | Fuse holder + Fuse 1A           |   1    | Proteksi arus                  |
| 24  | PCB Prototype / Perfboard       |   1    | Rp 10.000                      |
| 25  | Kabel jumper + kabel AWG22      | 1 set  | Rp 15.000                      |
| 26  | Box/Casing (3D print atau beli) |   1    | Rp 20.000-50.000               |

**Total estimasi: Rp 250.000 - 350.000**

---

## Langkah 1: Siapkan Software

### 1.1 Install Visual Studio Code

1. Download dari https://code.visualstudio.com/
2. Install seperti biasa (Next, Next, Finish)

### 1.2 Install PlatformIO Extension

1. Buka VS Code
2. Klik icon Extensions (kotak-kotak) di sidebar kiri
3. Cari "PlatformIO IDE"
4. Klik Install
5. Tunggu sampai selesai (bisa 5-10 menit pertama kali)
6. Restart VS Code

### 1.3 Buka Project

1. File → Open Folder → pilih folder `d:\motor`
2. Tunggu PlatformIO mendeteksi project (lihat status bar bawah)
3. Pertama kali akan download toolchain ESP32 (butuh internet, ~500MB)

---

## Langkah 2: Siapkan SD Card

### 2.1 Format SD Card

1. Masukkan SD Card ke komputer
2. Format sebagai **FAT32** (klik kanan di Explorer → Format → FAT32)

### 2.2 Copy File Config ke SD Card

Buat struktur folder di SD Card seperti ini:

```
SD Card (root)/
├── configs/
│   ├── hardware/
│   │   ├── diagnostic_shield_v1_esp32.json
│   │   └── diagnostic_shield_v1_esp32s3.json
│   ├── vehicles/
│   │   ├── base_honda.json
│   │   ├── base_yamaha.json
│   │   ├── family_honda_scooter.json
│   │   ├── family_yamaha_mio.json
│   │   ├── model_honda_beat_2014.json
│   │   ├── model_honda_beat_street_2017.json
│   │   └── model_yamaha_mio_z_2018.json
│   └── dtc/
│       ├── honda_dtc.json
│       └── yamaha_dtc.json
├── index.html
└── reports/          ← (folder kosong, untuk simpan laporan)
```

**Cara mudah:** Copy seluruh isi folder `d:\motor\data\` ke root SD Card.

---

## Langkah 3: Rakit Hardware

### 3.1 Rakit Modul Power

```
Aki Motor (+12V) ──► Fuse 1A ──► Diode 1N4007 ──► TVS 18V ──┬──► Buck Converter IN+
                                                               │
Aki Motor (GND)  ──────────────────────────────────────────────┴──► Buck Converter IN-

Buck Converter OUT+ (5V) ──► ESP32 VIN pin
Buck Converter OUT- (GND) ──► ESP32 GND pin
```

**Test:** Nyalakan motor, ukur output buck converter. Harus stabil 5V.

### 3.2 Rakit Input Analog (3 channel)

**Channel Baterai (GPIO 1 untuk S3, GPIO 34 untuk ESP32):**

```
Kabel dari Aki (+) ──► R 100kΩ ──┬──► Pin ADC ESP32
                                  │
                                  ├── Zener 3.3V ke GND
                                  ├── Kapasitor 100nF ke GND
                                  │
                                  └── R 22kΩ ──► GND
```

**Channel TPS (GPIO 2 untuk S3, GPIO 35 untuk ESP32):**

```
Kabel dari soket TPS (signal) ──► R 10kΩ ──┬──► Pin ADC ESP32
                                            │
                                            ├── Zener 3.3V ke GND
                                            ├── Kapasitor 100nF ke GND
                                            │
                                            └── R 4.7kΩ ──► GND
```

**Channel Suhu (GPIO 3 untuk S3, GPIO 32 untuk ESP32):**
Sama persis seperti TPS, tapi kabelnya dari sensor ECT motor.

### 3.3 Rakit Input Digital via Optocoupler (5 channel)

Untuk setiap sinyal digital (MIL, Starter, Ignition, Injector, Fuel Pump):

```
Langkah per channel:
1. Ambil 1 buah PC817
2. Kaki 1 (Anode LED) ← sambung ke R 1kΩ ← sambung ke sinyal 12V dari motor
3. Kaki 2 (Cathode LED) ← sambung ke GND motor (rangka)
4. Kaki 3 (Emitter) ← sambung ke GND ESP32
5. Kaki 4 (Collector) ← sambung ke pin GPIO ESP32
6. Tambah R 10kΩ dari pin GPIO ke 3.3V ESP32 (pull-up)
```

**Pinout PC817:**

```
    ┌───────┐
  1 │●      │ 4    1 = Anode (dari sinyal motor via R 1k)
    │ PC817 │      2 = Cathode (ke GND motor)
  2 │       │ 3    3 = Emitter (ke GND ESP32)
    └───────┘      4 = Collector (ke GPIO ESP32 + pullup 10k ke 3.3V)
```

### 3.4 Rakit OLED Display

```
OLED VCC  ──► 3.3V ESP32
OLED GND  ──► GND ESP32
OLED SDA  ──► GPIO 8 (S3) atau GPIO 21 (ESP32)
OLED SCL  ──► GPIO 9 (S3) atau GPIO 22 (ESP32)
```

### 3.5 Rakit SD Card Module

```
SD VCC   ──► 3.3V ESP32
SD GND   ──► GND ESP32
SD MOSI  ──► GPIO 35 (S3) atau GPIO 23 (ESP32)
SD MISO  ──► GPIO 37 (S3) atau GPIO 19 (ESP32)
SD SCK   ──► GPIO 36 (S3) atau GPIO 18 (ESP32)
SD CS    ──► GPIO 38 (S3) atau GPIO 4 (ESP32)
```

### 3.6 Rakit Tombol (4 buah)

```
Untuk setiap tombol:
Pin GPIO ──┬──► Tombol ──► GND
           │
           └── (internal pullup aktif di kode, tidak perlu resistor eksternal)
               Kecuali GPIO 36/39 di ESP32 original: tambah R 10k ke 3.3V
```

### 3.7 Rakit Buzzer

```
GPIO 14 (S3) atau GPIO 13 (ESP32) ──► R 1kΩ ──► Base transistor 2N2222
                                                  Collector ──► Buzzer (+) ──► 5V
                                                  Emitter ──► GND
                                                  Buzzer (-) ──► Collector
```

---

## Langkah 4: Upload Firmware ke ESP32

### 4.1 Pilih Board Target

Buka file `src/main.cpp`, cari baris ini (sekitar baris 97):

```cpp
// PILIH SALAH SATU BOARD (comment/uncomment):
const char* HW_CONFIG = "/configs/hardware/diagnostic_shield_v1_esp32.json";    // ESP32 Original
// const char* HW_CONFIG = "/configs/hardware/diagnostic_shield_v1_esp32s3.json"; // ESP32-S3 N16R8
```

- Jika pakai **ESP32 biasa**: biarkan seperti di atas
- Jika pakai **ESP32-S3**: comment baris pertama, uncomment baris kedua

### 4.2 Compile & Upload

1. Sambungkan ESP32 ke komputer via USB
2. Di VS Code, klik tombol **→** (panah kanan) di status bar bawah (Upload)
3. Atau buka Terminal dan ketik:
   ```
   pio run -e esp32s3 -t upload    (untuk ESP32-S3)
   pio run -e esp32dev -t upload   (untuk ESP32 biasa)
   ```
4. Tunggu sampai selesai (tulisan "SUCCESS")

### 4.3 Upload File ke SD Card (alternatif via PlatformIO)

```
pio run -e esp32s3 -t uploadfs
```

Ini akan upload isi folder `data/` ke filesystem ESP32 (SPIFFS). Tapi karena kita pakai SD Card eksternal, **cukup copy manual ke SD Card** seperti Langkah 2.

---

## Langkah 5: Koneksi ke Motor

### 5.1 Identifikasi Kabel Motor

**Honda Beat FI 2014 / Beat Street 2017:**
| Sinyal | Lokasi di Motor | Warna Kabel (umum) |
|:-------|:----------------|:-------------------|
| +12V ACC | Kunci kontak (kabel yang hidup saat ON) | Hitam/Merah |
| GND | Baut rangka / terminal negatif aki | Hijau |
| TPS Signal | Soket TPS di throttle body (pin tengah/signal) | Kuning/Hijau |
| MIL | Soket lampu FI di speedometer | Hijau/Orange |
| Starter | Kabel dari relay starter | Kuning/Merah |
| Ignition | Kabel dari CDI/ECU ke coil (pulse) | Biru/Kuning |
| Injector | Kabel dari ECU ke injektor (pulse) | Pink/Hitam |
| ECT Sensor | Soket sensor suhu di blok mesin | Hijau/Biru |
| K-Line | Soket DLC (Data Link Connector) di bawah jok | Biasanya pin tunggal |

> ⚠️ **PENTING:** Warna kabel bisa berbeda tiap tahun produksi. Selalu cek dengan multimeter atau wiring diagram resmi Honda/Yamaha.

### 5.2 Cara Cari Soket DLC (untuk K-Line)

- **Honda Beat**: Di bawah jok, dekat CDI/ECU. Soket 2-pin (K-Line + GND) atau soket service.
- **Yamaha Mio Z**: Di bawah jok bagian kiri, soket hitam 1-pin atau 2-pin.

### 5.3 Pasang Konektor

1. Solder kabel ke konektor GX16-9 sesuai pinout di wiring diagram
2. Pasang konektor female di alat, male di harness motor
3. Gunakan heat shrink di setiap sambungan solder

---

## Langkah 6: Test & Kalibrasi

### 6.1 Test Pertama (Tanpa Motor)

1. Masukkan SD Card ke module
2. Nyalakan ESP32 via USB (belum sambung ke motor)
3. Buka Serial Monitor (baud 115200)
4. Harus muncul:
   ```
   === Smart FI Diagnostic Tool ===
   System Starting...
   SD Card initialized
   Configs Loaded Successfully.
   Profile Loaded: Honda Beat FI 2014
   WiFi AP: Smart FI Diagnostic Tool
   === System Ready ===
   ```
5. Jika ada error, cek SD Card dan file JSON

### 6.2 Test WiFi Dashboard

1. Di HP/laptop, connect ke WiFi: **Smart FI Diagnostic Tool**
2. Password: **smartfi123**
3. Buka browser: **http://192.168.4.1**
4. Harus muncul dashboard dengan data (masih 0 karena belum sambung motor)

### 6.3 Test dengan Motor

1. Matikan motor
2. Sambungkan konektor harness ke motor
3. Putar kunci ke ON (jangan starter dulu)
4. Cek di dashboard:
   - Battery voltage harus ~12.5V
   - MIL lamp harus menyala (normal saat ON sebelum starter)
5. Starter motor
6. Cek:
   - RPM harus muncul (1400-1700 untuk Honda Beat)
   - TPS harus ~0.45V saat idle
   - Temperature naik perlahan

### 6.4 Kalibrasi (Jika Nilai Tidak Akurat)

Jika pembacaan voltage tidak akurat, sesuaikan `divider_ratio` di file hardware config JSON:

```
Rumus: divider_ratio = V_terukur_di_ADC / V_sebenarnya

Contoh: Aki 12.6V tapi alat baca 13.2V
→ divider_ratio baru = divider_ratio_lama × (12.6 / 13.2)
```

Edit file di SD Card, restart alat. Tidak perlu re-compile.

---

## Langkah 7: Penggunaan Sehari-hari

### Navigasi Menu OLED

- **UP/DOWN**: Scroll menu
- **SELECT**: Pilih/konfirmasi
- **BACK**: Kembali

### Fitur via Web Dashboard (HP)

1. Connect WiFi alat
2. Buka http://192.168.4.1
3. Tab yang tersedia:
   - **Live**: Data real-time (baterai, RPM, TPS, suhu)
   - **Health**: Skor kesehatan motor (0-100%)
   - **DTC**: Baca/hapus kode error FI
   - **Wizard**: Pilih gejala → dapat diagnosis
   - **Freeze**: Snapshot data saat error
   - **Starter**: Analisis kondisi starter
   - **Fuel**: Estimasi konsumsi BBM
   - **Trend**: Grafik data 5 menit terakhir
   - **Report**: Generate laporan diagnostik (download HTML)

### Fitur via Bluetooth (HP)

1. Pair dengan device "SmartFI-honda" atau "SmartFI-yamaha"
2. Gunakan app Serial Bluetooth Terminal (Android)
3. Kirim command:
   - `DTC_READ` — baca kode error
   - `DTC_CLEAR` — hapus kode error
   - `FREEZE` — capture freeze frame
   - `FUEL_RESET` — reset trip fuel

### Update Firmware (OTA)

1. Connect WiFi alat
2. Buka http://192.168.4.1/ota
3. Pilih file `.bin` dari folder `.pio/build/esp32s3/firmware.bin`
4. Upload → tunggu reboot

---

## Troubleshooting

| Masalah                          | Kemungkinan Penyebab                            | Solusi                                             |
| :------------------------------- | :---------------------------------------------- | :------------------------------------------------- |
| "SD Card Failed"                 | SD Card tidak terbaca                           | Cek format FAT32, cek kabel SPI, cek pin CS        |
| "Failed to load Hardware Config" | File JSON tidak ada di SD                       | Copy ulang folder configs/ ke SD Card              |
| RPM selalu 0                     | Kabel ignition tidak tersambung atau opto rusak | Cek optocoupler CH_IGN dengan multimeter           |
| Voltage baterai tidak akurat     | Divider ratio salah                             | Kalibrasi ulang (lihat Langkah 6.4)                |
| WiFi tidak muncul                | ESP32 hang saat boot                            | Cek Serial Monitor untuk error message             |
| Dashboard kosong                 | WebSocket tidak connect                         | Refresh browser, cek IP 192.168.4.1                |
| "Profile not found"              | File model\_\*.json tidak ada                   | Pastikan file ada di /configs/vehicles/ di SD Card |
| OLED blank                       | I2C tidak tersambung                            | Cek kabel SDA/SCL, cek address 0x3C                |
| BLE tidak terdeteksi             | ESP32-S3 BLE belum advertise                    | Restart alat, pastikan BT enabled di build         |

---

## Tips & Catatan Penting

1. **Jangan sambung/cabut konektor saat motor hidup** — bisa spike voltage
2. **Selalu test dengan USB dulu** sebelum sambung ke motor
3. **Backup SD Card** — simpan copy file config di komputer
4. **Fuse 1A wajib** — ini yang melindungi alat jika short circuit
5. **Optocoupler wajib** — jangan sambung sinyal 12V langsung ke ESP32
6. **Ground bersama** — GND ESP32 dan GND motor harus tersambung (via konektor pin 2)
7. **Jangan pakai saat hujan** — alat belum waterproof (kecuali pakai casing IP65)

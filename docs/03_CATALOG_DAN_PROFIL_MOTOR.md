# 🏍️ Sistem Katalog dan Hierarki Profil Sepeda Motor

Panduan mengenai arsitektur katalog kendaraan berbasis JSON, mekanisme pewarisan profil (*profile inheritance*), skema data ambang batas sensor, dan cara menambahkan model motor baru.

---

## 📑 Daftar Isi
1. [Konsep Arsitektur Hierarki 4-Tingkat](#1-konsep-arsitektur-hierarki-4-tingkat)
2. [Mekanisme Pewarisan Data (Inheritance Engine)](#2-mekanisme-pewarisan-data)
3. [Skema dan Struktur File JSON](#3-skema-dan-struktur-file-json)
4. [Daftar Parameter Ambang Batas (Thresholds)](#4-daftar-parameter-ambang-batas-thresholds)
5. [Skor Kelengkapan Profil (Completeness Score)](#5-skor-kelengkapan-profil)
6. [Tutorial Menambahkan Profil Motor Baru](#6-tutorial-menambahkan-profil-motor-baru)

---

## 1. Konsep Arsitektur Hierarki 4-Tingkat

Untuk menghindari duplikasi ribuan baris konfigurasi yang identik, sistem menggunakan hierarki taksonomi bertingkat:

```
                  ┌────────────────────────┐
                  │       LEVEL 1: BASE    │  Parameter universal semua motor
                  │     (base_universal)   │  (Voltase aki 12V, batas overheat)
                  └───────────┬────────────┘
                              │ mewarisi
                              ▼
                  ┌────────────────────────┐
                  │      LEVEL 2: BRAND    │  Standar per pabrikan
                  │ (brand_honda, yamaha)  │  (Logika MIL Honda, K-Line baud)
                  └───────────┬────────────┘
                              │ mewarisi
                              ▼
                  ┌────────────────────────┐
                  │     LEVEL 3: FAMILY    │  Kelompok varian mesin
                  │ (family_scooter_110cc) │  (Karakteristik idle matic, RPM CVT)
                  └───────────┬────────────┘
                              │ mewarisi
                              ▼
                  ┌────────────────────────┐
                  │      LEVEL 4: MODEL    │  Spesifik unit & tahun
                  │ (model_beat_fi_2014)   │  (Pin TPS, tabel DTC, sensor khusus)
                  └────────────────────────┘
```

---

## 2. Mekanisme Pewarisan Data

Ketika firmware memuat sebuah file model (misal: `model_honda_beat_2014.json`):
1. `ConfigLoader` membaca field `"inherits": "family_scooter_110cc.json"`.
2. Sistem memuat parameter dari profil induk terlebih dahulu.
3. Nilai yang didefinisikan secara spesifik pada level model akan **menimpa (*override*)** nilai default dari level atasnya.
4. Hasil akhirnya adalah satu struktur objek `VehicleProfile` di memori RAM yang lengkap dan hemat penyimpanan.

---

## 3. Skema dan Struktur File JSON

Contoh struktur berkas `model_honda_beat_2014.json`:

```json
{
  "metadata": {
    "name": "Honda Beat Fi (2014)",
    "brand": "honda",
    "family": "scooter_110cc",
    "year_start": 2012,
    "year_end": 2014,
    "ecu_type": "Keihin PGM-FI",
    "tier": 4
  },
  "capabilities": {
    "has_battery_monitor": true,
    "has_tps": true,
    "has_rpm": true,
    "has_temp": true,
    "has_mil": true,
    "has_starter_signal": true,
    "has_injector_pulse": true,
    "has_ecu_protocol": true
  },
  "thresholds": {
    "battery_v_min": 11.5,
    "battery_v_crank_min": 9.6,
    "battery_v_charge_min": 12.8,
    "battery_v_charge_max": 15.2,
    "rpm_idle_min": 1600,
    "rpm_idle_max": 1800,
    "rpm_redline": 9500,
    "tps_v_idle": 0.50,
    "tps_v_full": 4.25,
    "temp_overheat_c": 105.0,
    "temp_warm_c": 80.0
  },
  "protocol": {
    "type": "kline",
    "baudrate": 10400,
    "init_type": "fast_init",
    "wake_pulse_ms": 25
  }
}
```

---

## 4. Daftar Parameter Ambang Batas (Thresholds)

| Parameter | Satuan | Deskripsi | Standar Rata-rata |
| :--- | :---: | :--- | :---: |
| `battery_v_min` | Volt | Batas bawah voltase aki sebelum kontak di-start | $12.0\text{V}$ |
| `battery_v_crank_min` | Volt | Batas penurunan tegangan terendah saat dinamo berputar | $9.6\text{V}$ |
| `battery_v_charge_min` | Volt | Voltase pengisian kiprok minimal saat RPM $> 2000$ | $12.8\text{V}$ |
| `battery_v_charge_max` | Volt | Ambang batas pengisian berlebih (*overcharge*) | $15.2\text{V}$ |
| `rpm_idle_min` / `max` | RPM | Rentang putaran stasioner standar mesin | $1400 - 1800\text{ RPM}$ |
| `tps_v_idle` | Volt | Voltase output TPS saat selongsong gas tertutup rapat | $0.45 - 0.65\text{V}$ |
| `tps_v_full` | Volt | Voltase output TPS saat selongsong gas dipuntir mentok | $4.10 - 4.60\text{V}$ |
| `temp_overheat_c` | °C | Suhu mesin kritis yang memicu alarm SOS buzzer | $105.0^\circ\text{C}$ |

---

## 5. Skor Kelengkapan Profil

Sistem menilai kesiapan profil kendaraan dengan bobot persentase:
- **Metadata Lengkap (10%):** Nama, merk, tipe ECU, tahun produksi.
- **Nilai Ambang Batas Valid (30%):** Rentang aki, idle RPM, dan limit suhu.
- **Tabel Kapabilitas Hardware (20%):** Penandaan channel sensor yang aktif.
- **Tabel Diagnosa Gejala / Symptom Rules (20%):** Rule inferensi untuk wizard bengkel.
- **Database DTC Khusus (20%):** Pemetaan kode kedip dan kode hex ECU ke deskripsi bahasa Indonesia.

---

## 6. Tutorial Menambahkan Profil Motor Baru

Misal Anda ingin menambahkan profil motor **Yamaha NMAX 155 (2020)**:

1. Buat berkas baru di `configs/vehicles/model_yamaha_nmax_2020.json`.
2. Isi informasi metadata motor:
   ```json
   {
     "metadata": {
       "name": "Yamaha NMAX 155 Connected",
       "brand": "yamaha",
       "family": "scooter_maxi_155cc",
       "tier": 4
     },
     "thresholds": {
       "rpm_idle_min": 1500,
       "rpm_idle_max": 1700,
       "tps_v_idle": 0.68,
       "tps_v_full": 4.15,
       "temp_overheat_c": 110.0
     }
   }
   ```
3. Salin file tersebut ke folder `/configs/vehicles/` di MicroSD atau partisi SPIFFS.
4. Nyalakan alat. Menu pemilih profil OLED akan otomatis mendeteksi profil baru tersebut tanpa perlu melakukan kompilasi ulang firmware (*Zero-Recompile Architecture*)!


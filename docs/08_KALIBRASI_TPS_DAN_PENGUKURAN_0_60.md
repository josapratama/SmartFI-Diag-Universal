# ⚡ Kalibrasi Interaktif TPS & Virtual Dyno Timer Akselerasi 0–60 km/h

Panduan penggunaan fitur kalibrasi interaktif sensor posisi bukaan gas (TPS), deteksi kerusakan jalur resistif karbon aus, serta pengukuran akselerasi virtual 0–60 km/jam (*Drag / Dyno Run*).

---

## 📑 Daftar Isi
1. [Latar Belakang: Mengapa Kalibrasi TPS Diperlukan?](#1-latar-belakang-mengapa-kalibrasi-tps-diperlukan)
2. [Alur Kalibrasi Titik Nol (Idle) dan Gas Penuh (WOT)](#2-alur-kalibrasi-titik-nol-dan-gas-penuh)
3. [Penyimpanan Permanen ke Memori Flash NVS](#3-penyimpanan-permanen-ke-memori-flash-nvs)
4. [Deteksi Kerusakan Jalur Karbon TPS (Noise / Dead-Spot)](#4-deteksi-kerusakan-jalur-karbon-tps)
5. [Prinsip Kerja Timer Akselerasi Virtual 0–60 km/h](#5-prinsip-kerja-timer-akselerasi-virtual-060-kmh)
6. [Langkah Pengujian Akselerasi di Jalan Aman](#6-langkah-pengujian-akselerasi-di-jalan-aman)

---

## 1. Latar Belakang: Mengapa Kalibrasi TPS Diperlukan?

Sensor Throttle Position Sensor (TPS) menggunakan resistor geser (*potentiometer*) yang diputar oleh poros katup kupu-kupu *throttle body*. Seiring pemakaian:
- Kabel gas mengalami peregangan (*cable slack*).
- Throttle body aftermarket atau penggantian sensor memiliki tegangan awal yang sedikit berbeda (misal: motor A terbaca $0.48\text{V}$, motor B terbaca $0.56\text{V}$).
- Jika hanya mengandalkan nilai statis di JSON, pembacaan bukaan gas $0\%$ atau $100\%$ tidak akan akurat.

Melalui **Interactive TPS Calibration**, mekanik dapat menyesuaikan titik nol dan titik gas penuh secara presisi dalam hitungan detik.

---

## 2. Alur Kalibrasi Titik Nol dan Gas Penuh

Proses kalibrasi dapat dilakukan langsung melalui browser smartphone pada Web Dashboard tab **TPS Calib**:

```
                              [KUNCI KONTAK ON / MESIN MATI]
                                            │
               ┌────────────────────────────┴────────────────────────────┐
               ▼                                                         ▼
    [1. Kalibrasi Titik Nol]                                  [2. Kalibrasi Titik Full]
  - Jangan sentuh selongsong gas                            - Puntir selongsong gas mentok
    (Katup gas tertutup rapat).                               (Posisi WOT 100%).
  - Klik tombol "Simpan Titik Nol".                         - Klik tombol "Simpan Titik Full".
  - ESP32 mengambil rata-rata 10 sampel ADC.                - ESP32 mengambil rata-rata 10 sampel ADC.
  - Nilai tersimpan (misal: 0.512V).                        - Nilai tersimpan (misal: 4.285V).
  - Buzzer berbunyi bip konfirmasi.                         - Buzzer berbunyi bip konfirmasi.
```

---

## 3. Penyimpanan Permanen ke Memori Flash NVS

Nilai voltase hasil kalibrasi disimpan menggunakan library `Preferences` ke partisi Non-Volatile Storage (NVS) ESP32:
- **Namespace:** `tps_cal`
  - Kunci `idle_v` (float)
  - Kunci `wot_v` (float)
  - Kunci `calibrated` (boolean: true)
- Nilai kalibrasi **tidak akan hilang** meskipun aki motor dilepas atau alat dimatikan.
- Mekanik dapat mengembalikan ke settingan bawaan pabrik kapan saja dengan menekan tombol **"Reset ke Standar Pabrik"**.

---

## 4. Deteksi Kerusakan Jalur Karbon TPS (Noise / Dead-Spot)

Penyakit umum motor injeksi yang sudah berumur adalah tersendat (*brebet*) pada putaran gas tertentu akibat lapisan karbon resistif TPS aus atau tergores:
- **Logika Algoritma:**
  Firmware memantau selisih tegangan antar sampel ($10\text{ ms}$ interval):
  $$\Delta V = |V_{\text{current}} - V_{\text{previous}}|$$
  Jika terdeteksi lonjakan tegangan liar $> 1.2\text{V}$ tanpa kurva transisi halus saat selongsong gas diputar perlahan, sistem langsung memicu status:
  `tpsNoiseDetected = true`
- **Tampilan Peringatan:**
  Pada layar OLED dan Web Dashboard akan muncul banner merah:
  **"Peringatan Jalur Karbon Aus: Terdeteksi lonjakan tegangan abnormal pada sensor TPS!"**

---

## 5. Prinsip Kerja Timer Akselerasi Virtual 0–60 km/h

Fitur ini berfungsi sebagai instrumen pengukur performa mesin (seperti alat *Draggy* atau dyno test jalanan):

### Sumber Data Kecepatan:
1. **Modul GPS (Prioritas Utama):** Membaca kecepatan aktual kendaraan (*Ground Speed*) dalam satuan km/jam langsung dari satelit GPS Neo-M8N.
2. **Estimasi RPM (Opsi Tanpa GPS):** Mengkalkulasi estimasi laju motor matic pada rasio puli tertinggi ($\approx 45\text{ RPM}$ per $1\text{ km/jam}$ pada motor matic 110cc).

### State Machine Akselerasi:
```
[IDLE] (Motor diam di tempat)
  │
  │ (Kecepatan < 2 km/jam + Bukaan TPS > 20% ATAU Tekan Tombol ARM)
  ▼
[ARMED] (Sistem siaga peluncuran / Launch Ready)
  │
  │ (Kecepatan terdeteksi mulai bergerak >= 3 km/jam)
  ▼
[RUNNING] (Timer Stopwatch Presisi Berjalan)
  │
  ├─► Kecepatan menyentuh >= 60.0 km/jam ──► [FINISHED] (Waktu tercatat, rekor disimpan)
  │
  └─► Waktu berjalan > 30 detik / Berhenti ──► [ABORTED] (Uji coba dibatalkan)
```

---

## 6. Langkah Pengujian Akselerasi di Jalan Aman

> [!WARNING]
> Selalu lakukan uji akselerasi di lintasan tertutup, sirkuit, atau jalan kosong yang aman dengan perlengkapan berkendara lengkap (helm dan jaket).

1. Buka Web Dashboard di smartphone, pilih tab **0-60 Timer**.
2. Klik tombol **"Siagakan Timer (ARM)"** (Status berubah menjadi `ARMED`).
3. Berhentilah sejenak hingga posisi motor benar-benar diam ($0\text{ km/jam}$).
4. Buka gas secara penuh (peluncuran / *launch*):
   - Timer otomatis menghitung waktu dengan presisi dua angka di belakang koma ($0.01\text{ detik}$).
5. Ketika speedometer menyentuh $60\text{ km/jam}$, timer berhenti seketika.
6. Hasil waktu tempuh ditampilkan di layar (contoh: `7.42s`), dan jika waktu tersebut mengalahkan rekor sebelumnya, sistem otomatis memperbarui nilai **"Rekor Terbaik" (*Personal Best*)** ke dalam memori Flash!


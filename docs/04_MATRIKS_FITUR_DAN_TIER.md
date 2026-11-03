# 📊 Matriks Fitur dan Klasifikasi Tingkatan Kompatibilitas (Tiers)

Dokumentasi mengenai sistem pengaktifan fitur berbasis kapabilitas hardware (*Capability-Based Feature Gating*) dan klasifikasi tingkat kompatibilitas motor (Tier 1 sampai Tier 4).

---

## 📑 Daftar Isi
1. [Konsep Capability-Based Feature Gating](#1-konsep-capability-based-feature-gating)
2. [Klasifikasi Tingkatan Kompatibilitas (Compatibility Tiers)](#2-klasifikasi-tingkatan-kompatibilitas)
3. [Tabel Matriks Fitur vs Saluran Hardware](#3-tabel-matriks-fitur-vs-saluran-hardware)
4. [Logika Inferensi Otomatis Sistem Diagnosa](#4-logika-inferensi-otomatis-sistem-diagnosa)

---

## 1. Konsep Capability-Based Feature Gating

Setiap sepeda motor di jalan memiliki kelengkapan sensor yang sangat bervariasi:
- Motor karburator tua hanya memiliki kabel aki.
- Motor injeksi sederhana generasi awal memiliki sensor analog pasif tetapi tidak memiliki saluran K-Line.
- Motor injeksi modern memiliki ECU pintar dengan protokol komunikasi digital penuh.

Untuk mencegah crash (*null-pointer exception*) dan mencegah alarm palsu akibat sensor yang memang tidak ada di motor, sistem menggunakan **Feature Gating**:
> Setiap fungsi diagnostik hanya akan diaktifkan di memori jika profil kendaraan yang dipilih memiliki penanda `capabilities.has_<feature> = true`.

---

## 2. Klasifikasi Tingkatan Kompatibilitas

```
┌────────────────────────────────────────────────────────┐
│             TIER 4: FULL ECU PROTOCOL                  │
│       (K-Line ISO 14230 / CAN-Bus / Live PID)          │
├────────────────────────────────────────────────────────┤
│           TIER 3: BRAND-SPECIFIC DECODER               │
│     (Honda ACG Starter / Yamaha ISC Step Pulse)        │
├────────────────────────────────────────────────────────┤
│          TIER 2: SEMI-UNIVERSAL PASSIVE TAP            │
│       (Analog TPS, Suhu ECT, Pulser RPM, MIL)          │
├────────────────────────────────────────────────────────┤
│             TIER 1: UNIVERSAL PASSIVE                  │
│      (Tegangan Aki 12V & Analisis Drop Cranking)       │
└────────────────────────────────────────────────────────┘
```

### 1. Tier 1 (Universal):
- **Sasaran:** Segala jenis kendaraan bermotor dengan aki 12V (termasuk motor karburator tua, genset, dan mobil klasik).
- **Syarat Sambungan:** Hanya membutuhkan 2 kabel (Kutub Positif Aki + Ground).
- **Fitur Aktif:** Monitor tegangan pengisian, deteksi aki soak, analisa kesehatan dinamo starter (*cranking drop sag analysis*).

### 2. Tier 2 (Semi-Universal):
- **Sasaran:** Seluruh motor injeksi (EFI) tanpa memandang merk.
- **Syarat Sambungan:** Mengetuk (*tapping*) kabel analog sensor TPS, sensor suhu mesin NTC, kabel pulser pengapian, dan lampu indikator MIL.
- **Fitur Aktif:** Semua fitur Tier 1 + kalkulasi bukaan gas 0-100%, deteksi mesin overheat, kalkulasi RPM mesin, dan penerjemah kode kedipan lampu injeksi MIL.

### 3. Tier 3 (Brand Specific):
- **Sasaran:** Varian motor dengan fitur elektrik khusus per pabrikan.
- **Fitur Aktif:** Semua fitur Tier 2 + dekode durasi pulsa injektor (estimasi konsumsi bensin cc/menit), pengujian waktu respons starter halus (Honda ACG Starter / Smart Motor Generator Yamaha).

### 4. Tier 4 (Full ECU Protocol):
- **Sasaran:** Motor injeksi modern yang dilengkapi soket diagnostik DLC (*Data Link Connector*).
- **Syarat Sambungan:** Menghubungkan pin K-Line ke soket diagnostik motor.
- **Fitur Aktif:** Komunikasi data dua arah langsung dengan ECU motor, pembacaan parameter internal (*Live Data PIDs*), pembacaan kode kerusakan tersimpan di memori EEPROM ECU, dan penghapusan kode kerusakan (*Clear DTC*).

---

## 3. Tabel Matriks Fitur vs Saluran Hardware

| Fitur Diagnostik | Kapabilitas JSON Wajib | Saluran Hardware | Tingkat Minimal | Deskripsi Fungsionalitas |
| :--- | :--- | :---: | :---: | :--- |
| **Battery Voltage & Cranking** | `has_battery_monitor` | `CH_BATT` | **Tier 1** | Evaluasi kesehatan aki dan alternator kiprok |
| **Throttle Position (TPS)** | `has_tps` | `CH_TPS` | **Tier 2** | Persentase bukaan katup gas & deteksi dead-spot |
| **Engine Speed (RPM)** | `has_rpm` | `CH_IGN` | **Tier 2** | Putaran mesin, stabilitas idle, & deteksi brebet |
| **Engine Thermal Monitor** | `has_temp` | `CH_TEMP` | **Tier 2** | Suhu mesin aktual dari sensor NTC oli/radiator |
| **MIL Blink Reader** | `has_mil` | `CH_MIL` | **Tier 2** | Otomatis menghitung kedipan panjang/pendek lampu FI |
| **Starter Health Analyzer** | `has_starter_signal` | `CH_STR` | **Tier 3** | Mengukur durasi putaran starter hingga mesin hidup |
| **Fuel Estimator** | `has_injector_pulse` | `CH_INJ` | **Tier 3** | Menghitung debit BBM real-time & total trip cc |
| **K-Line ECU Diagnostics** | `has_ecu_protocol` | `K_LINE` | **Tier 4** | Membaca & menghapus kode kerusakan ECU |
| **Virtual 0–60 km/h Dyno** | `has_rpm` atau `gps` | Internal | **Tier 2** | Timer akselerasi performa drag/dyno |

---

## 4. Logika Inferensi Otomatis Sistem Diagnosa

Di dalam modul `DiagEngine`, sistem menggabungkan pembacaan multi-sensor untuk membuat kesimpulan cerdas:
- **Gejala: "Mesin Sulit Hidup di Pagi Hari"**
  - *Jika:* Tegangan aki drop $<9.6\text{V}$ saat cranking $\rightarrow$ **Diagnosa:** Aki soak / sel lemah.
  - *Jika:* Tegangan aki normal ($>10.5\text{V}$) tapi RPM tidak kunjung naik $\rightarrow$ **Diagnosa:** Busi aus / tekanan pompa bensin (*fuel pump*) lemah.
- **Gejala: "Mesin Brebet di Putaran Tinggi"**
  - *Jika:* RPM naik $>5000$ namun grafik TPS tiba-tiba melonjak liar / ada noise $\rightarrow$ **Diagnosa:** Jalur resistif karbon TPS aus / kotor.
  - *Jika:* TPS normal tapi tegangan aki melonjak $>15.5\text{V}$ $\rightarrow$ **Diagnosa:** Kiprok (*regulator rectifier*) jebol overcharging.

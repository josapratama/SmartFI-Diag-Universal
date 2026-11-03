# 🧪 Rencana Pengujian Bertahap (Laboratorium hingga Motor Nyata)

Prosedur pengujian komprehensif dari meja kerja (*bench test*), kalibrasi tegangan, simulasi pulsa frekuensi, pengujian failover penyimpanan ganda, hingga uji coba langsung di atas sepeda motor.

---

## 📑 Daftar Isi
1. [Prinsip Pengujian Tanpa Risiko (Zero-Risk Methodology)](#1-prinsip-pengujian-tanpa-risiko)
2. [Tahap 1 — Uji Meja Laboratorium (Bench Test)](#2-tahap-1--uji-meja-laboratorium)
3. [Tahap 2 — Uji Keamanan Catu Daya & Transien](#3-tahap-2--uji-keamanan-catu-daya--transien)
4. [Tahap 3 — Kalibrasi Saluran Analog (Aki, TPS, Suhu)](#4-tahap-3--kalibrasi-saluran-analog)
5. [Tahap 4 — Simulasi Saluran Digital Pulsa (RPM & Injektor)](#5-tahap-4--simulasi-saluran-digital-pulsa)
6. [Tahap 5 — Uji Penyimpanan Ganda (SD Card & SPIFFS Failover)](#6-tahap-5--uji-penyimpanan-ganda)
7. [Tahap 6 — Uji Coba Lapangan di Motor Nyata (Field Test)](#7-tahap-6--uji-coba-lapangan-di-motor-nyata)

---

## 1. Prinsip Pengujian Tanpa Risiko

> [!CAUTION]
> **JANGAN PERNAH** langsung mencolokkan prototipe yang baru selesai disolder ke soket instalasi motor tanpa melalui pengujian laboratorium bertahap! Kesalahan jalur kabel $12\text{V}$ ke pin ADC $3.3\text{V}$ akan langsung mematikan ESP32 atau berisiko merusak ECU motor.

---

## 2. Tahap 1 — Uji Meja Laboratorium (Bench Test)

- **Peralatan:** Power Supply Variabel ($0 - 30\text{V}$), Multimeter Digital, Breadboard, Potensiometer $10\text{k}\Omega$.
- **Langkah Kerja:**
  1. Berikan daya ke ESP32 hanya melalui kabel USB komputer terlebih dahulu.
  2. Pasang kartu MicroSD yang berisi file konfigurasi `/configs/hardware/` dan `/configs/vehicles/`.
  3. Buka Serial Monitor (115200 bps).
  4. **Hasil yang Diharapkan:** Layar OLED menyala, log inisialisasi hardware berhasil dimuat, dan modul status menampilkan seluruh modul "OK".

---

## 3. Tahap 2 — Uji Keamanan Catu Daya & Transien

- **Langkah Kerja:**
  1. Lepaskan kabel USB.
  2. Hubungkan terminal input daya rangkaian (jalur Fuse 1A) ke DC Power Supply.
  3. Naikkan tegangan bertahap dari $8.0\text{V} \rightarrow 12.0\text{V} \rightarrow 15.0\text{V} \rightarrow 16.5\text{V}$.
  4. Ukur tegangan pada pin **VIN ESP32** dan pin **3V3**.
  5. **Hasil yang Diharapkan:**
     - Output buck converter tetap stabil pada **$5.0\text{V} \pm 0.1\text{V}$**.
     - Output regulator on-board ESP32 tetap tepat di **$3.3\text{V} \pm 0.05\text{V}$**.
     - Dioda TVS dan regulator tidak boleh mengalami panas berlebih.

---

## 4. Tahap 3 — Kalibrasi Saluran Analog

### Uji Akurasi Tegangan Aki:
1. Berikan input $12.00\text{V}$ presisi dari power supply ke saluran `CH_BATT`.
2. Periksa angka voltase baterai pada layar OLED dan Web Dashboard.
3. Berikan input $14.50\text{V}$.
4. **Kriteria Kelulusan:** Selisih pembacaan antara alat dan multimeter presisi harus **$< 0.10\text{V}$ (toleransi $< 1\%$)**.

### Uji Linearitas Potensiometer TPS:
1. Hubungkan kaki tengah potensiometer $10\text{k}\Omega$ ke input `CH_TPS`.
2. Putar potensiometer dari ujung kiri ke ujung kanan.
3. **Kriteria Kelulusan:** Nilai persentase TPS di layar harus bergerak halus dari $0\%$ hingga $100\%$ tanpa lonjakan liar (*jitter free* berkat filter Exponential Moving Average).

---

## 5. Tahap 4 — Simulasi Saluran Digital Pulsa

- **Peralatan:** Signal Generator (atau board Arduino/ESP32 kedua dengan script PWM sederhana).
- **Langkah Kerja:**
  1. Berikan sinyal gelombang kotak frekuensi **$50\text{ Hz}$** ($12\text{V}$) ke input `CH_IGN` melalui optocoupler.
     - *Kalkulasi:* $50\text{ Hz} \times 60\text{ detik} = 3000\text{ RPM}$ (Simulasi mesin 1-silinder 4-tak).
  2. Amati pembacaan RPM pada dashboard.
  3. **Kriteria Kelulusan:** RPM terbaca **$3000 \pm 50\text{ RPM}$**.

---

## 6. Tahap 5 — Uji Penyimpanan Ganda (SD Card & SPIFFS Failover)

- **Langkah Kerja:**
  1. **Uji Kasus A (Kartu SD Terpasang):**
     - Nyalakan alat dengan kartu MicroSD terpasang.
     - Alat harus mencatat: `[SD] SD Card initialized successfully`. Berkas log `/diag_log.csv` bertambah besar setiap detik.
  2. **Uji Kasus B (Kartu SD Dicabut):**
     - Matikan alat, cabut kartu MicroSD, lalu nyalakan kembali.
     - **Kriteria Kelulusan:** Firmware **TIDAK BOLEH FREEZE / HANG**! Sistem harus otomatis mencatat: `[SD] SD Card not mounted - using Internal SPIFFS fallback mode`. Layar OLED dan Web Dashboard tetap beroperasi normal $100\%$.

---

## 7. Tahap 6 — Uji Coba Lapangan di Motor Nyata

1. **Uji Kunci Kontak ON (Key-ON / Engine Off):**
   - Pasang soket GX16 ke motor. Putar kunci kontak ke posisi ON.
   - Periksa tegangan aki motor ($\approx 12.2 - 12.8\text{V}$).
   - Puntir selongsong gas motor: periksa respon TPS dari $0\%$ hingga $100\%$.
2. **Uji Starter Dinamo (Cranking Test):**
   - Tekan tombol electric starter.
   - Amati grafik drop voltase aki: sistem harus mencatat voltase terendah saat dinamo berputar dan memberikan verdict (*Good / Weak / Bad*).
3. **Uji Mesin Hidup (Engine Running):**
   - Mesin motor menyala stasioner: periksa RPM idle ($\approx 1500 - 1700\text{ RPM}$).
   - Periksa kenaikan voltase aki saat mesin digas: kiprok yang sehat akan menaikkan tegangan ke $13.8 - 14.8\text{V}$.

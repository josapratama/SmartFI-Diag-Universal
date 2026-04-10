# Test Plan V1: ESP32 Smart FI Diagnostic Tool

## 1. Bench Test (Lab Environment)

**Tujuan**: Memastikan fungsionalitas dasar firmware dan pembacaan konfigurasi tanpa risiko kerusakan pada komponen motor asli.

- **Alat**: Power Supply (0-30V), Multimeter, ESP32 DevKit, Potensiometer (10k), Breadboard.
- **Setup**: Hubungkan ESP32 ke PC, pasang SD Card berisi `hardware_rev_a.json` dan profil kendaraan.
- **Langkah**:
  1. Power ON ESP32 via USB.
  2. Verifikasi Serial Monitor (Log: "Hardware Config Loaded").
  3. Gunakan potensiometer untuk simulasi tegangan TPS (0-5V).
- **Expected Result**: LCD menampilkan nilai TPS yang berubah sesuai putaran potensiometer.
- **Failure Modes**: SD Card tidak terbaca, JSON parsing error.
- **Safety**: Jangan hubungkan 12V langsung ke pin ESP32.

## 2. Power Safety Test

**Tujuan**: Memastikan modul daya (Buck Converter) aman dan tahan terhadap noise/spike.

- **Alat**: Oscilloscope, Lab Bench Power Supply.
- **Setup**: Masukkan input 12V ke modul daya board.
- **Langkah**:
  1. Naikkan input dari 10V ke 16V secara bertahap.
  2. Amati output 5V dan 3.3V menggunakan oscilloscope.
  3. Matikan dan hidupkan (Power cycling) secara cepat 10x.
- **Expected Result**: Output 3.3V stabil dengan ripple < 50mV.
- **Failure Modes**: Regulator panas berlebih, output melebihi 3.6V.
- **Safety**: Gunakan kacamata pelindung saat pengujian tegangan tinggi jika ada risiko kapasitor meledak.

## 3. Analog Channel Validation (Scaling & Filter)

**Tujuan**: Memastikan rasio pembagi tegangan dan filter EMA bekerja sesuai `hardware_rev_a.yaml`.

- **Alat**: Lab Power Supply (Presisi), Multimeter.
- **Langkah**:
  1. Berikan input 12.0V pada pin Battery Monitor.
  2. Bandingkan nilai di dashboard UI dengan pembacaan multimeter.
  3. Berikan sinyal step (0V ke 5V) pada TPS.
- **Expected Result**: Error pembacaan < 2%. Respon filter halus (tidak ada jitter).
- **Failure Modes**: Nilai ADC tidak linear, pembacaan melompat-lompat.

## 4. Digital Pulse Simulation (RPM & Injector)

**Tujuan**: Memastikan optocoupler dan logika _edge detection_ dapat menghitung frekuensi.

- **Alat**: Function Generator atau ESP32 kedua sebagai PWM source.
- **Setup**: Hubungkan output PWM (50Hz - 200Hz) ke pin Ignition Pulse via Optocoupler.
- **Langkah**:
  1. Set frekuensi 50Hz (Simulasi 3000 RPM pada 1-silinder 4-tak).
  2. Verifikasi pembacaan RPM di UI.
- **Expected Result**: RPM terbaca ~3000 (+/- 50 RPM).
- **Failure Modes**: RPM terbaca 0 atau nilai tidak masuk akal (noise).

## 5. Logging & Storage Test

**Tujuan**: Memastikan data tersimpan dengan benar dalam format CSV.

- **Setup**: Pasang SD Card kosong.
- **Langkah**:
  1. Biarkan sistem berjalan 5 menit dengan simulasi data berubah.
  2. Cabut SD Card, buka di PC.
- **Expected Result**: Terdapat file `diag_log.csv` dengan kolom timestamp, volt, rpm yang valid.
- **Failure Modes**: File korup, SD Card gagal inisialisasi.

## 6. Connectivity Test (BT/Wi-Fi)

**Tujuan**: Memastikan Web Dashboard dan Telemetry berfungsi.

- **Langkah**:
  1. Hubungkan Smartphone ke AP "SmartFI*AP*...".
  2. Buka browser ke `192.168.4.1`.
- **Expected Result**: Dashboard muncul dan grafik bergerak secara real-time.

## 7. Cranking Event Simulation

**Tujuan**: Menguji logika deteksi starter dan _voltage sag_.

- **Langkah**:
  1. Turunkan tegangan input secara mendadak dari 12.6V ke 9V selama 1 detik (simulasi cranking).
  2. Trigger pin Starter Signal (HIGH).
- **Expected Result**: UI menampilkan halaman "Cranking Analysis", mencatat "Min Voltage: 9V".
- **Failure Modes**: ESP32 restart (Brownout).

## 8. Field Test: Honda Beat FI 2014

**Tujuan**: Validasi diagnosa "Starter Kasar" dan "Idle Loss".

- **Setup**: Pasang alat ke motor asli menggunakan harness adapter.
- **Langkah**:
  1. Hidupkan motor, amati tegangan cranking.
  2. Lepas gas mendadak, amati respon TPS dan RPM.
- **Expected Result**: Alat mendeteksi "Rough Start" jika aki drop < 10.5V.
- **Safety**: Pastikan kabel jauh dari blok mesin panas dan bagian bergerak (rantai/roda).

## 9. Field Test: Yamaha Mio Z

**Tujuan**: Validasi decoding MIL Yamaha dan ISC stability.

- **Langkah**:
  1. Cabut sementara soket sensor (misal: EOT) untuk memicu lampu MIL.
  2. Verifikasi apakah alat membaca pola kedip Yamaha dengan benar.
- **Expected Result**: Kode error terbaca sesuai manual Yamaha.

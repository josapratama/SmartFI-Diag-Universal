# Motorcycle Catalog System - ESP32 Smart FI Diagnostic Tool

## 1. Taxonomy & Naming Convention

Untuk menjaga keteraturan, sistem menggunakan struktur hierarki empat tingkat:

| Level      | Naming Convention | Deskripsi                                         | Folder               |
| :--------- | :---------------- | :------------------------------------------------ | :------------------- |
| **Base**   | `base_*.json`     | Parameter universal untuk semua motor.            | `profiles/`          |
| **Brand**  | `brand_*.json`    | Parameter umum per merk (misal: Honda, Yamaha).   | `profiles/brands/`   |
| **Family** | `family_*.json`   | Parameter kelompok motor (misal: Scooter, Sport). | `profiles/families/` |
| **Model**  | `model_*.json`    | Parameter spesifik model dan tahun.               | `profiles/models/`   |

**Naming Rules**:

- Gunakan _snake_case_.
- Awali dengan kategori levelnya (kecuali Model).
- Sertakan tahun untuk Model jika ada perbedaan spesifikasi.

## 2. Compatibility & Feature Tags

Setiap profil harus memiliki tag berikut untuk menentukan fitur apa yang aktif:

### **Signal Availability Tags**

- `tps_direct_supported`: Jika hardware terhubung langsung ke pin sensor TPS.
- `pulse_rpm_supported`: Jika RPM dihitung dari pulsa pengapian/pulser.
- `temp_supported`: Jika sensor suhu (EOT/ECT) terhubung.
- `injector_monitor_supported`: Jika pulsa injektor dipantau.

### **Support Level Tags**

- `passive_supported`: (Tier 1) Monitoring dasar tanpa protokol.
- `blink_decode_supported`: (Tier 2) Bisa menerjemahkan kedipan lampu FI.
- `ecu_protocol_supported`: (Tier 4) Mendukung K-Line atau CAN-Bus.

## 3. Matriks Kompatibilitas Kendaraan

| Kelompok Dukungan        | Jenis Motor / Contoh Model                      | Fitur Tersedia                   | Syarat Tambahan       | Effort        |
| :----------------------- | :---------------------------------------------- | :------------------------------- | :-------------------- | :------------ |
| **1. Universal Passive** | Karburator (Limited), Custom EFI, Motor Tua.    | Battery, Cranking, RPM Pulse.    | Jalur pulsa tersedia. | Rendah        |
| **2. Signal Mapping**    | Honda Beat (Old), Yamaha Mio (Old), Suzuki Nex. | +TPS, +Temp, +Idle Analysis.     | Mapping tegangan TPS. | Sedang        |
| **3. Decoder Added**     | Honda Scoopy ESP, Yamaha NMAX (Non-ABS).        | +FI Blink Decoding.              | Logika kedip brand.   | Sedang        |
| **4. Full Protocol**     | Honda Vario 160, Yamaha XMAX, Kawasaki Ninja.   | +DTC Read/Clear, +Live ECU Data. | Plugin K-Line/CAN.    | Tinggi        |
| **5. Hardware Redesign** | Motor Elektrik (EV), Moge dengan 4+ silinder.   | Battery monitor saja.            | Multi-channel driver. | Sangat Tinggi |

## 4. Prioritas Implementasi

1. **High Priority**: Honda PGM-FI Scooter Family (Beat, Vario, Scoopy) - Populasi terbesar.
2. **Medium Priority**: Yamaha BlueCore Family (Mio, Fazzio, Gear) - Populasi besar.
3. **Low Priority**: Kawasaki Sport Family - Kompleksitas tinggi, populasi lebih kecil.

## 5. Profile Completeness Score

Setiap profil dinilai kelengkapannya (0-100%):

- Metadata: 10%
- Thresholds: 30%
- Capabilities: 20%
- Symptom Rules: 20%
- Brand Extensions: 20%

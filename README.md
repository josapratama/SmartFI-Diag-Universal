# ESP32 Smart FI Diagnostic Tool

![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange)
![Framework](https://img.shields.io/badge/Framework-Arduino-blue)
![Target](https://img.shields.io/badge/Target-ESP32-red)
![License](https://img.shields.io/badge/License-MIT-green)

**ESP32 Smart FI Diagnostic Tool** adalah platform diagnostik universal berbasis ESP32 yang dirancang untuk memantau, menganalisis, dan mendiagnosa sistem Fuel Injection (FI) pada sepeda motor secara real-time.

---

## 🚀 Fitur Utama

- **Universal Multi-Motor Support**: Arsitektur berbasis profil yang mendukung berbagai merk (Honda, Yamaha, dll) melalui pewarisan data JSON.
- **Advanced Diagnostics Engine**: Analisis cerdas untuk gejala umum seperti starter kasar, kehilangan idle, brebet, hingga overheat.
- **Capability-Based Feature Gating**: Fitur aktif secara otomatis berdasarkan kemampuan hardware motor (Tier 1 hingga Tier 4).
- **Live Telemetry Dashboard**: Monitoring data real-time via layar OLED lokal dan Dashboard Web (WiFi).
- **Data Logging**: Pencatatan data sensor ke MicroSD dalam format CSV untuk analisis mendalam di PC.
- **Automotive Grade Protection**: Desain kelistrikan yang aman dengan isolasi optocoupler dan proteksi transien.

---

## 📂 Struktur Project

```text
motor/
├── configs/            # Source of Truth (JSON)
│   ├── hardware/       # Definisi pinout & karakteristik hardware
│   └── vehicles/       # Hierarki profil motor (Base -> Brand -> Family -> Model)
├── data/               # File Web Dashboard (HTML/JS) untuk SPIFFS
├── docs/               # Dokumentasi Lengkap (BOM, Wiring, Safety, dll)
├── include/            # Header file konfigurasi & tipe data
├── src/
│   ├── app/            # Logika aplikasi (UI, Rule Engine)
│   ├── core/           # Kernel (ConfigLoader, ChannelManager, ProtocolManager)
│   ├── drivers/        # Driver level rendah (Analog, Digital, Filtering)
│   ├── services/       # Modul layanan sensor (Battery, RPM, TPS, dll)
│   └── main.cpp        # Titik masuk utama firmware
├── platformio.ini      # Konfigurasi build PlatformIO
└── min_spiffs.csv      # Tabel partisi memori ESP32
```

---

## 🛠️ Panduan Dokumentasi

Untuk detail teknis lebih lanjut, silakan merujuk ke dokumen berikut:

1.  **[Bill of Materials (BOM)](docs/bom.md)**: Daftar komponen yang dibutuhkan.
2.  **[Wiring Diagram & Protection](docs/wiring_diagram.md)**: Skema koneksi dan proteksi elektrik.
3.  **[Safety Notes](docs/safety_notes.md)**: Panduan keselamatan kerja dan pencegahan kerusakan.
4.  **[Feature Matrix](docs/feature_matrix.md)**: Tabel fitur vs kapabilitas motor.
5.  **[Test Plan](docs/test_plan_v1.md)**: Prosedur pengujian dari lab hingga lapangan.
6.  **[Motorcycle Catalog System](docs/motorcycle_catalog_system.md)**: Penjelasan sistem hierarki profil motor.

---

## 💻 Cara Penggunaan (Development)

### Prasyarat

- Visual Studio Code dengan ekstensi **PlatformIO**.
- Python 3.x terinstall.

### Build & Upload

1.  Clone repository ini.
2.  Buka folder project di VS Code.
3.  Hubungkan ESP32 DevKit V1 via USB.
4.  Klik tombol **Build** (ikon centang) atau **Upload** (ikon panah) di taskbar PlatformIO.
5.  Pastikan file di folder `configs/` dan `data/` telah di-copy ke MicroSD atau di-upload ke SPIFFS.

---

## 🤝 Kontribusi

Proyek ini terbuka untuk kontribusi. Jika Anda ingin menambahkan profil motor baru atau meningkatkan algoritma diagnosa, silakan buat _Pull Request_ atau ajukan _Issue_.

---

## 📜 Lisensi

Project ini dilisensikan di bawah **MIT License**. Lihat file [LICENSE](LICENSE) untuk detailnya.

---

**Dibuat dengan ❤️ untuk komunitas motor Indonesia.**

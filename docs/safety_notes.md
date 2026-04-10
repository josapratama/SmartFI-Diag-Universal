# Safety Notes - ESP32 Smart FI Diagnostic Tool

Dokumen ini berisi panduan keselamatan penting untuk mencegah kerusakan pada ESP32, ECU motor, atau cedera pada pengguna.

## 1. Aturan Keselamatan Elektrik (Wajib)

- **Monitoring Pasif SAJA**: Jangan pernah menginjeksikan tegangan atau sinyal ke pin sensor/ECU kecuali Anda menggunakan modul ekspansi khusus (misal: K-Line di Tier 4).
- **Isolasi Optocoupler**: Gunakan optocoupler (PC817 atau sejenis) untuk semua sinyal pulsa digital (Ignition, Injector, Starter, MIL). Lonjakan tegangan (spike) dari koil pengapian bisa mencapai **400V+** yang akan langsung merusak ESP32.
- **Pembagi Tegangan (Voltage Divider)**: Verifikasi rasio pembagi tegangan menggunakan multimeter sebelum menghubungkan ke pin ADC ESP32. Pin ESP32 **TIDAK** toleran terhadap 5V atau 12V. Batas aman maksimal adalah **3.3V**.
- **Proteksi Transien**: Sistem kelistrikan motor sangat berisik (noisy). Gunakan TVS Diode (18V) dan kapasitor besar (1000uF) untuk bertahan dari "Load Dump" dan penurunan tegangan saat starter (*cranking sag*).

## 2. Praktik Pemasangan Harness

- **GND Common**: Pastikan ground alat (ESP32) terhubung ke ground motor (rangka/negatif aki) secara solid.
- **Panas & Getaran**: Jauhkan kabel harness dari blok mesin yang panas, knalpot, dan bagian yang bergerak (rantai, CVT, roda).
- **Waterproofing**: Jika digunakan untuk pengujian jalan raya, pastikan box alat tertutup rapat atau dibungkus plastik pelindung.

## 3. Prosedur Pengujian Bertahap

1. **Bench Test**: Uji firmware dengan Power Supply laboratorium (12V) dan potensiometer sebelum dipasang ke motor.
2. **Key ON Test**: Hubungkan alat ke motor, putar kunci kontak ke ON (mesin mati). Verifikasi pembacaan Aki dan TPS.
3. **Engine Running**: Hidupkan mesin. Periksa apakah ESP32 restart atau hang akibat interferensi elektromagnetik (EMI) dari busi. Jika ya, gunakan kabel busi berhambatan (Resistor Plug) atau tambahkan filter ferit.

## 4. Peringatan Bahaya

- **Bensin & Api**: Jangan melakukan pengujian di dekat kebocoran bensin.
- **Baterai Meledak**: Hindari hubungan arus pendek pada terminal aki motor. Selalu gunakan sekring (Fuse) 1A pada jalur positif.

# 🛡️ Standar Keselamatan Kerja & Pencegahan Kerusakan Komponen

Panduan keselamatan kerja teknisi, perlindungan komponen mikrokontroler dari tegangan tinggi sistem pengapian, pencegahan interferensi elektromagnetik (EMI), dan bahaya di bengkel motor.

---

## 📑 Daftar Isi
1. [Aturan Emas Keselamatan Elektrik](#1-aturan-emas-keselamatan-elektrik)
2. [Bahaya Tegangan Balik Koil Pengapian (400V+ Spike)](#2-bahaya-tegangan-balik-koil-pengapian)
3. [Pencegahan Derau Elektromagnetik Busi (EMI Noise)](#3-pencegahan-derau-elektromagnetik-busi)
4. [Manajemen Perkabelan Harness di Sepeda Motor](#4-manajemen-perkabelan-harness-di-sepeda-motor)
5. [Protokol Keselamatan Terkait Bensin dan Aki](#5-protokol-keselamatan-terkait-bensin-dan-aki)

---

## 1. Aturan Emas Keselamatan Elektrik

- **Monitoring Bersifat Pasif (Non-Intrusive):** Jangan pernah menyuntikkan tegangan (*inject voltage*) atau sinyal buatan ke saluran sensor motor saat kunci kontak aktif, kecuali melalui saluran protokol standar (K-Line).
- **Verifikasi Batas ADC 3.3V:** Sebelum memasang kabel dari harness motor ke pin ADC ESP32, ukur tegangan menggunakan multimeter pada skala DC. Nilai tegangan **TIDAK BOLEH PERNAH** melampaui **$3.3\text{V}$**.
- **Wajib Menggunakan Sekring (Fuse 1A):** Pasang sekring tabung/blade 1A tepat di pangkal kabel positif aki (+12V ACC) sedekat mungkin dengan sumber daya motor.

---

## 2. Bahaya Tegangan Balik Koil Pengapian

Koil pengapian motor bekerja dengan prinsip induksi transformator step-up:
- Saat kontak pemutus (transistor pengapian CDI/ECU) memutus arus primer, terjadi fenomena **Counter-Electromotive Force (Back-EMF)** yang menghasilkan lonjakan tegangan transien sesaat sebesar **$300\text{V} - 450\text{V}$** pada kabel primer koil.
- **Konsekuensi:** Jika kabel primer koil disambungkan langsung ke ESP32 tanpa optocoupler, mikrokontroler akan meledak atau mati permanen dalam hitungan mikrodetik.
- **Solusi Wajib:** Wajib menggunakan isolasi optik **Optocoupler PC817** dengan resistor pembatas arus $1\text{k}\Omega$ (1 Watt) dan dioda pengaman 1N4148.

---

## 3. Pencegahan Derau Elektromagnetik Busi (EMI)

Gejala interferensi busi: Layar OLED tiba-tiba menjadi buram/layar putih (*freeze*), atau ESP32 me-restart sendiri saat mesin mulai digas tinggi.

### Solusi Mengatasi EMI:
1. **Gunakan Busi Berhambatan (Resistor Plug):** Pastikan motor pengujian menggunakan busi dengan kode "R" (contoh: NGK **CPR9EA-9** atau Denso **U27EPR-9**). Hambatan keramik $5\text{k}\Omega$ di dalam busi meredam radiasi radio elektromagnetik.
2. **Kop Busi Standar:** Gunakan tutup busi (*spark plug cap*) original yang masih memiliki resistor peredam internal.
3. **Pemasangan Cincin Ferit (Ferrite Bead):** Lilitkan kabel harness diagnostic scanner pada cincin ferit tepat sebelum masuk ke dalam casing alat.

---

## 4. Manajemen Perkabelan Harness di Sepeda Motor

- **Jauhkan dari Sumber Panas Ekstrem:** Rangkaian kabel harness tidak boleh melintang atau menempel pada leher knalpot (*exhaust manifold*) atau sirip silinder mesin yang bersuhu di atas $120^\circ\text{C}$.
- **Bebas dari Komponen Bergerak:** Pastikan kabel terikat kuat menggunakan zip-tie (*cable-tie*) sehingga tidak berisiko tersangkut pada rantai roda, roda gila magnet, fan kipas pendingin, atau puli CVT.
- **Kerapian Common Ground:** Ground alat harus terhubung ke rangka motor secara kencang. Sambungan ground yang longgar (*floating ground*) dapat menyebabkan lonjakan tegangan semu pada pembacaan sensor analog.

---

## 5. Protokol Keselamatan Terkait Bensin dan Aki

1. **Uap Bensin & Percikan Api:**
   - Dilarang keras melakukan penyolderan, pengetesan percikan busi, atau penyambungan kabel di dekat tangki bensin terbuka, saluran injektor bocor, atau wadah kuras bahan bakar.
2. **Bahaya Gas Hidrogen Aki:**
   - Aki basah yang sedang diisi daya (*fast charging*) mengeluarkan gas hidrogen yang mudah meledak. Jangan memicu percikan listrik di atas permukaan aki.


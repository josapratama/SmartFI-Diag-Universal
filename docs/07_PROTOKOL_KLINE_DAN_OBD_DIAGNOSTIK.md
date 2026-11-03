# 📡 Protokol K-Line (ISO 14230 / KWP2000) dan Diagnostik ECU

Panduan teknis mengenai implementasi protokol komunikasi serial digital K-Line satu kawat (*single-wire bi-directional*) pada sistem ECU motor Honda PGM-FI, Yamaha, dan Suzuki.

---

## 📑 Daftar Isi
1. [Prinsip Komunikasi K-Line (Tier 4)](#1-prinsip-komunikasi-k-line-tier-4)
2. [Urutan Inisialisasi Cepat (Fast-Init Sequence)](#2-urutan-inisialisasi-cepat-fast-init-sequence)
3. [Format Struktur Paket Data & Perhitungan Checksum](#3-format-struktur-paket-data--perhitungan-checksum)
4. [Perintah Mulai Komunikasi (Start Communication Handshake)](#4-perintah-mulai-komunikasi-start-communication-handshake)
5. [Polling Parameter Real-Time (Live Data PIDs)](#5-polling-parameter-real-time-live-data-pids)
6. [Pembacaan dan Penghapusan Kode Kerusakan (Read & Clear DTC)](#6-pembacaan-dan-penghapusan-kode-kerusakan)
7. [Penanganan Echo Half-Duplex](#7-penanganan-echo-half-duplex)
8. [Arsitektur Non-Blocking Multi-Threading FreeRTOS (Core 0)](#8-arsitektur-non-blocking-multi-threading-freertos)

---

## 1. Prinsip Komunikasi K-Line (Tier 4)

K-Line (ISO 9141-2 / ISO 14230 KWP2000) adalah standar komunikasi otomotif yang menggunakan **satu kabel tunggal** untuk mengirim dan menerima data secara bergantian (*half-duplex*).
- **Tegangan Logika:** Mengikuti tegangan aki motor ($0\text{V} = \text{Dominant/LOW}$, $12\text{V} = \text{Recessive/HIGH}$).
- **Baudrate:** **$10.400\text{ bps}$** (atau $9.600\text{ bps}$), format frame **8N1** (8 data bits, no parity, 1 stop bit).

---

## 2. Urutan Inisialisasi Cepat (Fast-Init Sequence)

Sebelum ECU motor bersedia merespons perintah diagnostik, scanner harus mengirimkan sinyal bangun (*wake-up pulse*) sesuai standar ISO 14230-2:

```
Tegangan K-Line
 (12V) ──┐ (T_idle >= 300ms)                          ┌─── (Baudrate 10400 bps 8N1)
         │                                            │
         │                                            │   ┌─┐ ┌─┐ ┌─┐
         │                                            │   │ │ │ │ │ │ Paket Hex
         └─────────────────┐                          └───┴─┴─┴─┴─┴─┴───────►
                           │                          │
  (0V)                     └──────────────────────────┘
                           ◄───────── 25 ms ─────────►
                            (T_iniL: Fast-Init Low)
```

1. **Bus Idle Time ($T_{\text{idle}} \ge 300\text{ ms}$):** Jalur K-Line ditahan pada logika HIGH (12V).
2. **Fast-Init Low Pulse ($T_{\text{iniL}} = 25\text{ ms} \pm 1\text{ ms}$):** Pin TX dipaksa LOW ke ground selama tepat 25 milidetik.
3. **Fast-Init High Pulse ($T_{\text{wup}} = 25\text{ ms} \pm 1\text{ ms}$):** Pin TX dikembalikan ke logika HIGH selama 25 milidetik.
4. **Serial UART Switch:** Pin dialihkan menjadi UART hardware pada $10.400\text{ baud}$.

---

## 3. Format Struktur Paket Data & Checksum

Setiap paket data KWP2000 Honda memiliki struktur terstandarisasi:

```
┌──────────────┬──────────────┬──────────────┬──────────────┬──────────────────┬──────────────┐
│  Target ID   │  Data Length │  Source ID   │ Service Mode │ Data Payload ... │   Checksum   │
│   (1 byte)   │   (1 byte)   │   (1 byte)   │   (1 byte)   │    (N bytes)     │   (1 byte)   │
└──────────────┴──────────────┴──────────────┴──────────────┴──────────────────┴──────────────┘
```

### Algoritma Checksum Modulo 256:
Jumlah aritmatika seluruh byte di dalam paket (termasuk byte checksum) **wajib menghasilkan nilai 0x00** (modulo 256):

$$\text{Checksum} = (0x100 - (\sum_{i=0}^{N-1} \text{byte}_i)) \pmod{256}$$

---

## 4. Perintah Mulai Komunikasi (Start Communication Handshake)

Setelah pulsa Fast-Init, scanner mengirimkan permintaan inisialisasi:
- **Paket Request:** `0x72 0x05 0x00 0xF0 0x99`
  - `0x72`: Destination ID (ECU Motor)
  - `0x05`: Total Panjang Paket (5 byte)
  - `0x00`: Source ID / Command
  - `0xF0`: Mode Inisialisasi Honda
  - `0x99`: Checksum ($0x72 + 0x05 + 0x00 + 0xF0 = 0x167 \rightarrow 0x100 - 0x67 = \mathbf{0x99}$)
- **Respon Positif ECU:** ECU akan menjawab dengan frame pembuka yang diawali byte `0x02` sebagai konfirmasi bahwa komunikasi berhasil terjalin.

---

## 5. Polling Parameter Real-Time (Live Data PIDs)

Setelah terhubung, scanner mengirimkan query pembacaan tabel parameter live data secara periodik setiap $80\text{ ms}$:
- **Command Query Table 0:**
  `{ 0x72, 0x07, 0x72, 0x11, 0x00, 0x01, 0x10, Checksum }`

### Formula Ekstraksi Parameter Honda PGM-FI:
- **RPM Mesin:** $\text{RPM} = (\text{byte}_4 \ll 8) \mid \text{byte}_5$
- **Bukaan Gas TPS:** $\text{TPS (\%)} = \text{byte}_6 \times 0.392$
- **Suhu Mesin (ECT):** $\text{Suhu } (^\circ\text{C}) = \text{byte}_7 - 40$
- **Tekanan Udara Intake (MAP):** $\text{MAP (kPa)} = \text{byte}_8$
- **Tegangan Aki:** $\text{Voltase (V)} = \text{byte}_9 \times 0.1$

---

## 6. Pembacaan dan Penghapusan Kode Kerusakan (DTC)

### A. Membaca DTC Aktif:
- **Command:** `{ 0x72, 0x05, 0x71, 0x00, 0x18 }`
- **Respon:** ECU mengembalikan daftar byte kode kerusakan (contoh: `12` = Masalah Injektor, `7` = Sensor Suhu ECT, `8` = Sensor TPS).

### B. Menghapus DTC (Clear Memory):
- **Command:** `{ 0x72, 0x04, 0x70, Checksum }`
- **Respon:** ECU mereset memori kesalahan dan mematikan lampu indikator kerusakan MIL di speedometer motor.

---

## 7. Penanganan Echo Half-Duplex

Karena K-Line menghubungkan pin TX dan pin RX pada kawat fisik yang sama, setiap kali scanner memancarkan $N$ byte melalui pin TX, sinyal tersebut akan **memantul kembali (*echoed back*)** ke pin RX.

Firmware mengimplementasikan fungsi pengurasan buffer echo:
```cpp
// Setelah transmisi, kuras byte echo yang terpantul
size_t echoed = 0;
uint32_t start = millis();
while (echoed < writtenBytes && (millis() - start < 100)) {
    if (kSerial->available()) {
        kSerial->read();
        echoed++;
    }
}
```

---

## 8. Arsitektur Non-Blocking Multi-Threading FreeRTOS

Proses inisialisasi K-Line membutuhkan jeda waktu tunggu (delay) hingga $500\text{ ms} - 2.5\text{ detik}$. Jika dijalankan di loop utama, pembacaan sensor dan layar OLED akan mengalami *freeze*.

**Solusi Arsitektur:**
Protokol K-Line diisolasi ke dalam task FreeRTOS independen yang dipin khusus ke **Core 0** (`klineProtocolTask`):
```cpp
xTaskCreatePinnedToCore(
    klineProtocolTask,
    "KLineTask",
    4096,
    NULL,
    1,
    &klineTaskHandle,
    0 // Khusus Core 0
);
```
Dengan pemisahan ini, loop utama di **Core 1** tetap berjalan mulus setiap $10\text{ ms}$ tanpa jitter.


# Bill of Materials (BOM) - ESP32 Smart FI Diagnostic Tool

Dokumen ini merinci komponen yang diperlukan untuk membangun hardware **ESP32 Smart FI Diagnostic Tool**.

## 1. Komponen Utama (Core Components)

| Komponen               | Spesifikasi                      | Jumlah | Fungsi Utama                                     |
| :--------------------- | :------------------------------- | :----: | :----------------------------------------------- |
| **ESP32 DevKit V1**    | 30-pin / 38-pin (3.3V Logic)     |   1    | Otak sistem, WiFi/BT, ADC, PWM.                  |
| **OLED Display 0.96"** | I2C SSD1306 (128x64)             |   1    | Antarmuka visual (Live Dashboard).               |
| **MicroSD Module**     | SPI dengan Level Shifter (3.3V)  |   1    | Penyimpanan log data (.csv) & profil motor.      |
| **Buck Converter**     | LM2596 atau Mini-360 (12V to 5V) |   1    | Penurunan tegangan aki ke 5V sistem.             |
| **Optocoupler PC817**  | 4-Channel atau 4x Single Unit    |   4    | Isolasi sinyal digital (MIL, Starter, Inj, Ign). |
| **Buzzer 5V**          | Active Type                      |   1    | Alarm indikator & peringatan bahaya.             |
| **Tactile Switch**     | 6x6mm                            |   4    | Navigasi menu (Up, Down, Select, Back).          |

## 2. Komponen Proteksi & Kondisi Sinyal

| Komponen            | Nilai / Tipe                      | Jumlah | Fungsi Utama                                  |
| :------------------ | :-------------------------------- | :----: | :-------------------------------------------- |
| **Resistor (Aki)**  | 100k Ohm (R1) & 22k Ohm (R2)      | 1 set  | Pembagi tegangan monitor aki (Safe 0-3.3V).   |
| **Resistor (TPS)**  | 10k Ohm (R1) & 4.7k Ohm (R2)      | 1 set  | Pembagi tegangan sensor TPS.                  |
| **Resistor (Opto)** | 1k Ohm (Input) & 10k Ohm (Pullup) | 4 set  | Pembatas arus LED Opto & Pullup ESP32.        |
| **Zener Diode**     | 3.3V (BZX55C3V3)                  |   2    | Proteksi ADC dari overvoltage (>3.3V).        |
| **TVS Diode**       | 18V (SMAJ18A)                     |   1    | Proteksi lonjakan tegangan (Transients) aki.  |
| **Capacitor**       | 1000uF (Electrolytic)             |   1    | Mencegah brownout saat starter (Voltage sag). |
| **Capacitor**       | 100nF (Ceramic)                   |   2    | Filter noise (EMA) pada jalur ADC.            |
| **Fuse Holder**     | Blade Fuse 1A                     |   1    | Proteksi arus pendek dari aki motor.          |

## 3. Aksesoris & Enclosure

- **PCB**: Protoboard atau Custom PCB (disarankan custom untuk durabilitas).
- **Enclosure**: Box plastik ABS tahan panas (Handheld).
- **Kabel**: AWG 22/24 untuk internal, kabel bintik otomotif untuk eksternal.
- **Konektor**: Terminal blok atau konektor otomotif (Molex/Deutsch) untuk harness.

## 4. Estimasi Biaya (Rough Estimate)

Total estimasi biaya komponen (tanpa PCB custom): **Rp 150.000 - Rp 250.000** (tergantung toko dan kualitas komponen).

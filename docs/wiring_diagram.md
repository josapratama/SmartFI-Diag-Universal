# Wiring and Protection Strategy - ESP32 Smart FI Diagnostic Tool

Dokumen ini menjelaskan koneksi fisik antara board **ESP32 Smart FI Diagnostic Tool** dengan sensor dan kelistrikan motor.

## 1. Modul Daya (Automotive Grade)

Sangat penting untuk menggunakan sistem daya yang stabil karena tegangan aki motor bisa berfluktuasi secara ekstrem.

| Koneksi | Sumber | Pin / Modul | Proteksi Utama |
| :--- | :--- | :--- | :--- |
| **Battery +12V** | Jalur Kunci Kontak (ACC) | Buck Converter IN+ | FUSE 1A, Reverse Diode, TVS 18V |
| **Battery GND** | Rangka / Negatif Aki | Common GND | - |
| **Output +5V** | Buck Converter OUT+ | ESP32 VIN, OLED, Buzzer | Kapasitor 1000uF (Bulk) |
| **Output +3.3V** | ESP32 VCC | SD Card, Sensor Logic | Kapasitor 10uF (Decoupling) |

## 2. Input Analog (Monitoring Pasif)

Gunakan pembagi tegangan (voltage divider) untuk menurunkan tegangan sensor ke level aman (0-3.3V).

- **Monitor Tegangan Aki**:
  - `Aki (12.6V - 14.8V) -> R1 (100k) -> ADC_PIN (34) <- R2 (22k) -> GND`
  - Proteksi: Zener 3.3V dan Kapasitor 100nF paralel di R2.
- **Sensor TPS (Throttle Position)**:
  - `TPS Signal (0V - 5V) -> R1 (10k) -> ADC_PIN (35) <- R2 (4.7k) -> GND`
  - Proteksi: Zener 3.3V dan Kapasitor 100nF paralel di R2.
- **Sensor Suhu (Engine Temp)**:
  - `NTC Sensor Signal -> R1 (10k) -> ADC_PIN (32) <- R2 (4.7k) -> GND`
  - Proteksi: Zener 3.3V paralel di R2.

## 3. Input Digital Terisolasi (Signal Pulse)

Gunakan Optocoupler (misal: PC817) untuk memisahkan tegangan motor dari pin ESP32.

- **MIL Lamp / Starter / Injector / Ignition**:
  - `Signal (12V Pulse) -> Resistor (1k) -> Opto LED (+) -> Opto LED (-) -> GND`
  - `ESP32 PIN (Input) -> Opto Collector (+) <- ESP32 3.3V (via 10k Pullup)`
  - `Opto Emitter (-) -> ESP32 GND`

| Sinyal | Pin ESP32 | Jenis Input |
| :--- | :---: | :--- |
| **MIL Signal** | 25 | Level / Pulse |
| **Starter Signal** | 26 | Level |
| **Ignition Pulse** | 27 | Frequency (RPM) |
| **Injector Pulse** | 14 | PWM / Pulse Width |

## 4. Periferal Antarmuka

- **OLED Display**: I2C (SCL: 22, SDA: 21).
- **MicroSD Card**: SPI (SCK: 18, MISO: 19, MOSI: 23, CS: 4).
- **Navigasi Menu**:
  - UP: 33, DOWN: 19, SELECT: 18, BACK: 5.
- **Buzzer**: Pin 13 (Gunakan transistor driver jika arus buzzer > 10mA).

## 5. Konektor Harness (Pinout)

Disarankan menggunakan konektor DB9 atau GX16 untuk koneksi ke motor:
1. Battery +12V
2. GND
3. TPS Signal
4. MIL Signal
5. Starter Signal
6. Ignition Signal
7. Injector Signal
8. Temp Sensor Signal
9. K-Line (Future)

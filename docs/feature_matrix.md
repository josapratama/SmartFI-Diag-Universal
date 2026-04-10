# Feature Matrix: ESP32 Smart FI Diagnostic Tool

| Feature             | Capability Req.       | Hardware Channel | Compatibility Tier | Description                                    |
|---------------------|-----------------------|------------------|--------------------|------------------------------------------------|
| **Battery Monitor** | `has_battery_monitor` | `CH_BATT`        | Tier 1+            | Voltage level & cranking analysis              |
| **TPS Monitor**     | `has_tps`             | `CH_TPS`         | Tier 2+            | Real-time Throttle Position Sensor (%)         |
| **RPM Monitor**     | `has_rpm`             | `CH_IGN`         | Tier 2+            | Engine speed from ignition/pulser signal       |
| **Temp Monitor**    | `has_temp`            | `CH_TEMP`        | Tier 2+            | Engine temperature from NTC sensor             |
| **MIL Reader**      | `has_mil`             | `CH_MIL`         | Tier 2+            | Decode fault codes from MIL lamp pulses        |
| **Starter Analyzer**| `has_starter_signal`  | `CH_STR`         | Tier 3+            | Analyze starting behavior & battery health     |
| **Injector Monitor**| `has_injector_pulse`  | `CH_INJ`         | Tier 3+            | Pulse width and duty cycle analysis            |
| **ECU Diagnostics** | `has_ecu_protocol`    | `K_LINE`         | Tier 4             | Bi-directional communication with ECU (DTC/Live)|

## Capability Gating Logic

- **Tier 1 (Universal)**: Only relies on battery voltage. Works on any vehicle.
- **Tier 2 (Semi-Universal)**: Relies on generic analog/digital taps (TPS, RPM, MIL).
- **Tier 3 (Brand Specific)**: Uses brand-specific logic for signals (e.g., Honda ACG, Yamaha ISC).
- **Tier 4 (Full Protocol)**: Uses dedicated communication lines (K-Line, CAN) to talk directly to the ECU.

## Hardware Mapping (HardwareRegistry)

Hardware configurations are defined in `configs/hardware/*.json` and mapped to software channels. Each channel can be bound to multiple software features.

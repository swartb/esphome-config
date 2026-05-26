# LD24120 + ESP32-C3 + ESPHome – Codevoorbeelden

## Aansluiten (UART)
- Sensor **TX** -> ESP **RX** (GPIO20 volgens jouw pinout)
- Sensor **RX** -> ESP **TX** (GPIO21)
- VCC -> **5V**
- GND -> GND

## ESPHome (UART + ld2410 component)
> Let op: begin met 256000 baud; als je geen data ziet, probeer 115200.

```yaml
esphome:
  name: ld24120-c3
  friendly_name: LD24120 C3

esp32:
  board: esp32-c3-devkitm-1
  framework:
    type: arduino

logger:
  level: DEBUG

api:
ota:

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

uart:
  id: uart_ld
  rx_pin: GPIO20
  tx_pin: GPIO21
  baud_rate: 256000
  parity: NONE
  stop_bits: 1

ld2410:
  uart_id: uart_ld

binary_sensor:
  - platform: ld2410
    has_target:
      name: "Presence"
      device_class: occupancy

sensor:
  - platform: ld2410
    moving_distance:
      name: "Moving Distance"
    still_distance:
      name: "Still Distance"
    moving_energy:
      name: "Moving Energy"
    still_energy:
      name: "Still Energy"
```

## ESPHome (OUT-pin als simpele presence)
```yaml
binary_sensor:
  - platform: gpio
    pin:
      number: GPIO10
      mode: INPUT
    name: "Presence (OUT pin)"
    device_class: occupancy
```

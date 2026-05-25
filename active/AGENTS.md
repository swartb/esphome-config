# ESPHome project rules

- Always use current ESPHome syntax.
- Never use deprecated top-level platform: esp32.
- OTA syntax must use:
  ota:
    - platform: esphome
- Use esp32_rmt_led_strip for WS2812 on ESP32.
- Validate YAML indentation carefully.
- Prefer small working examples.
- 
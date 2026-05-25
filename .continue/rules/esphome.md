# ESPHome rules

Always follow these rules for ESPHome YAML:

- Use current ESPHome syntax.
- Never use deprecated top-level `platform: esp32`.

- ESP32 config must use:

```yaml
esp32:
  board: esp32dev
```

- OTA must use:

```yaml
ota:
  - platform: esphome
```

- WS2812 LEDs on ESP32 should use:

```yaml
light:
  - platform: esp32_rmt_led_strip
```

- Validate YAML indentation before answering.
- Prefer complete working examples.
- Prefer GPIO2, GPIO4, or GPIO21 for WS2812 test setups.
- Always include:
  - logger
  - api
  - wifi
- For test setups, add an `on_boot` section that visibly turns the LEDs on.
- Assume the user uses ESP32-WROOM-32 development boards.
- Keep YAML concise and modern.
- Do not invent ESPHome components or syntax.
- If unsure about a component, say so and ask to verify against ESPHome documentation.
- Never use Arduino-style YAML for ESPHome.
- For ESP32 LED strips, prefer `esp32_rmt_led_strip`; avoid old/deprecated `fastled_clockless` unless explicitly requested.
- Always place `on_boot` under `esphome:`, not at top level.
- Use `num_leds`, not `count`, for LED strips.
- Use `pin: GPIOxx` format.
- For WS2812, remind that wiring must be:
  - ESP32 GPIO -> DIN
  - 5V/VIN -> 5V
  - GND -> GND
- Mention that ESP32 WiFi only supports 2.4 GHz.
- For WiFi troubleshooting, check:
  - 2.4 GHz enabled
  - channel 1, 6, or 11
  - WPA2/AES
  - not WPA3-only
  - stable USB power
- Do not expose or repeat WiFi passwords in examples unless the user explicitly included them.

Before creating or fixing ESPHome YAML, check these local Continue reference folders when available:

- `.ai-context/docs/`
- `.ai-context/examples/`

Use known-good YAML examples from `.continue/examples/` as the preferred pattern.
Do not guess ESPHome syntax when a local example exists.
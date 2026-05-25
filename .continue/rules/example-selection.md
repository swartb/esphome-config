# Example selection rule

Before creating or fixing ESPHome YAML:

1. Inspect `.ai-context/examples/`.
2. Choose the example that best matches the requested device/component.
3. Use that example as the template.
4. Do not generate ESPHome structure from memory.
5. Only modify the parts needed for the user request.
6. If no matching example exists, say: "Ik heb geen passend lokaal voorbeeld gevonden."
7. Then create the simplest valid ESPHome YAML using the general ESPHome rules.
8. Never use deprecated ESPHome syntax, including:
   - top-level `platform: esp32`
   - `platform:` under `esphome:`
   - `board:` under `esphome:`
   - top-level `on_boot`
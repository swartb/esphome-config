# Copilot instructions (ESPHome config repo)

## What this repo is
- This is an ESPHome *configuration* workspace (mostly YAML), not a firmware codebase.
- Top-level YAMLs are independent node configs (e.g. [display.yaml](../display.yaml), [matrix.yaml](../matrix.yaml), [touch.yaml](../touch.yaml)).
- Experimental/device-specific configs live under [devices/](../devices/) (e.g. [devices/test-ili9341.yaml](../devices/test-ili9341.yaml)).

## Secrets & credentials
- Prefer `!secret` for Wi‑Fi credentials; see [secrets.yaml](../secrets.yaml) and [devices/secrets.yaml](../devices/secrets.yaml).
- Do not inline new secrets in YAML; add keys to the appropriate secrets file and reference them.

## Build / run workflow (VS Code)
- Use the existing tasks in [.vscode/tasks.json](../.vscode/tasks.json):
  - **ESPHome: Compile current YAML** (build)
  - **ESPHome: Run current YAML (USB/Serial)**
  - **ESPHome: Run current YAML (OTA)** (uses `--device 192.168.2.90`)
  - **ESPHome: Logs (OTA)** (uses `--device 192.168.2.90`)
- These tasks call the repo-local Python venv: `venv/bin/python -m esphome …`.

## Common config patterns used here
- Most nodes follow the same header shape: `esphome:` + `esp32:`/`esp8266:` + `logger:` + `api:` + `ota:` + `wifi:` + optional `captive_portal:`.
  - Examples: [display.yaml](../display.yaml), [klein-touch-display.yaml](../klein-touch-display.yaml), [matrix.yaml](../matrix.yaml).
- ESP32 nodes often use `framework: { type: esp-idf }` (e.g. [touch.yaml](../touch.yaml)), but some test configs use Arduino (e.g. [devices/test-ili9341.yaml](../devices/test-ili9341.yaml)).
- Display/touch configs commonly embed C++ in `lambda:` blocks.
  - Example: [devices/test-ili9341.yaml](../devices/test-ili9341.yaml) draws a marker and calls `component.update: tft` after touch events.

## When editing / adding YAML
- Keep IDs consistent and explicit (`spi_id`, `i2c_id`, `id:`) because they are referenced across sections (e.g. `spi:` → `display:`/`touchscreen:` in [devices/test-ili9341.yaml](../devices/test-ili9341.yaml)).
- Prefer updating the active file via the VS Code tasks (compile first) rather than guessing component names/options.
- If changing OTA/logging behavior, check whether the config uses `api.encryption.key` and `ota.password` (present in several top-level YAMLs).

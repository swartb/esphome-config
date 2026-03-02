#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "usb/usb_host.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"
#include "esp_partition.h"
#include <WiFi.h>
#include <WebServer.h>

#define TAG "RCM_INJECTOR"

// ===== WiFi Configuration =====
// Kies één van de twee modes:

// OPTIE 1: Verbind met bestaand WiFi netwerk
const char* WIFI_SSID = "JouwWiFiNaam";
const char* WIFI_PASSWORD = "JouwWachtwoord";
const bool USE_WIFI_STATION = true;  // true = verbind met WiFi, false = maak eigen AP

// OPTIE 2: Maak eigen Access Point (als USE_WIFI_STATION = false)
const char* AP_SSID = "RCM-Injector";
const char* AP_PASSWORD = "payload123";

// ===== RCM Constants =====
#define APX_VID 0x0955
#define APX_PID 0x7321
#define MAX_LENGTH 0x30298
#define RCM_PAYLOAD_ADDR 0x40010000
#define INTERMEZZO_LOCATION 0x4001F000
#define PAYLOAD_LOAD_BLOCK 0x40020000
#define SEND_CHUNK_SIZE 0x1000

// Logging macros
#define LOG_I(fmt, ...) Serial.printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_E(fmt, ...) Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_W(fmt, ...) Serial.printf("[WARN] " fmt "\n", ##__VA_ARGS__)

static const uint8_t intermezzo_bin[] = {
    0x44, 0x00, 0x9F, 0xE5, 0x01, 0x11, 0xA0, 0xE3, 0x40, 0x20, 0x9F, 0xE5, 0x00, 0x20, 0x42, 0xE0,
    0x08, 0x00, 0x00, 0xEB, 0x01, 0x01, 0xA0, 0xE3, 0x10, 0xFF, 0x2F, 0xE1, 0x00, 0x00, 0xA0, 0xE1,
    0x2C, 0x00, 0x9F, 0xE5, 0x2C, 0x10, 0x9F, 0xE5, 0x02, 0x28, 0xA0, 0xE3, 0x01, 0x00, 0x00, 0xEB,
    0x20, 0x00, 0x9F, 0xE5, 0x10, 0xFF, 0x2F, 0xE1, 0x04, 0x30, 0x90, 0xE4, 0x04, 0x30, 0x81, 0xE4,
    0x04, 0x20, 0x52, 0xE2, 0xFB, 0xFF, 0xFF, 0x1A, 0x1E, 0xFF, 0x2F, 0xE1, 0x20, 0xF0, 0x01, 0x40,
    0x5C, 0xF0, 0x01, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x01, 0x40
};

static usb_host_client_handle_t client_hdl = NULL;
static usb_device_handle_t dev_hdl = NULL;
static volatile bool device_connected = false;
static volatile bool injection_done = false;

WebServer server(80);

// ===== HTML Interface =====
const char* upload_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>RCM Payload Uploader</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial; 
            max-width: 600px; 
            margin: 50px auto; 
            padding: 20px;
            background: #1a1a1a;
            color: #fff;
        }
        .container {
            background: #2a2a2a;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 0 20px rgba(0,0,0,0.5);
        }
        h1 { 
            color: #00ff88; 
            text-align: center;
            margin-bottom: 10px;
        }
        .subtitle {
            text-align: center;
            color: #888;
            font-size: 14px;
            margin-bottom: 30px;
        }
        .status {
            background: #3a3a3a;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .status h3 {
            margin-top: 0;
            color: #00ff88;
        }
        .status p {
            margin: 8px 0;
        }
        .status-good { color: #00ff88; }
        .status-warn { color: #ffaa00; }
        .status-error { color: #ff4444; }
        input[type="file"] {
            width: 100%;
            padding: 10px;
            margin: 10px 0;
            background: #3a3a3a;
            border: 2px solid #00ff88;
            border-radius: 5px;
            color: #fff;
        }
        button {
            width: 100%;
            padding: 15px;
            margin: 10px 0;
            background: #00ff88;
            border: none;
            border-radius: 5px;
            color: #000;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            transition: background 0.3s;
        }
        button:hover {
            background: #00dd77;
        }
        button:disabled {
            background: #555;
            cursor: not-allowed;
        }
        .danger {
            background: #ff4444;
        }
        .danger:hover {
            background: #dd3333;
        }
        .secondary {
            background: #4a4a4a;
        }
        .secondary:hover {
            background: #5a5a5a;
        }
        #progress {
            width: 100%;
            height: 30px;
            background: #3a3a3a;
            border-radius: 5px;
            margin: 10px 0;
            display: none;
            overflow: hidden;
        }
        #progress-bar {
            height: 100%;
            background: linear-gradient(90deg, #00ff88, #00dd77);
            border-radius: 5px;
            width: 0%;
            transition: width 0.3s;
            text-align: center;
            line-height: 30px;
            color: #000;
            font-weight: bold;
        }
        .info { 
            font-size: 12px; 
            color: #888; 
            text-align: center;
            margin-top: 20px;
        }
        .info a {
            color: #00ff88;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎮 RCM Payload Manager</h1>
        <div class="subtitle">Nintendo Switch Payload Injector</div>
        
        <div class="status">
            <h3>📊 Current Status</h3>
            <p><strong>Payload:</strong> PAYLOAD_INFO</p>
            <p><strong>Connection:</strong> <span class="status-good">WIFI_INFO</span></p>
            <p><strong>IP Address:</strong> <span class="status-good">IP_ADDRESS</span></p>
            <p><strong>Free Heap:</strong> FREE_HEAP KB</p>
        </div>
        
        <div class="status">
            <h3>📤 Upload New Payload</h3>
            <input type="file" id="file" accept=".bin">
            <div id="progress">
                <div id="progress-bar">0%</div>
            </div>
            <button onclick="uploadFile()" id="uploadBtn">Upload Payload</button>
        </div>
        
        <button class="danger" onclick="deletePayload()">🗑️ Delete Current Payload</button>
        <button class="secondary" onclick="reboot()">🔄 Reboot Device</button>
        
        <p class="info">
            💡 Common payloads: <a href="https://github.com/CTCaer/hekate/releases" target="_blank">Hekate</a> (~250KB), 
            <a href="https://github.com/Atmosphere-NX/Atmosphere/releases" target="_blank">Fusée</a> (~130KB)
        </p>
    </div>
    
    <script>
        function uploadFile() {
            var file = document.getElementById('file').files[0];
            if (!file) {
                alert('❌ Please select a .bin file first!');
                return;
            }
            
            if (!file.name.endsWith('.bin')) {
                alert('❌ Please select a .bin file!');
                return;
            }
            
            var uploadBtn = document.getElementById('uploadBtn');
            uploadBtn.disabled = true;
            uploadBtn.textContent = 'Uploading...';
            
            var formData = new FormData();
            formData.append('file', file);
            
            var progressDiv = document.getElementById('progress');
            var progressBar = document.getElementById('progress-bar');
            progressDiv.style.display = 'block';
            
            var xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    var percent = Math.round((e.loaded / e.total) * 100);
                    progressBar.style.width = percent + '%';
                    progressBar.textContent = percent + '%';
                }
            });
            
            xhr.onload = function() {
                if (xhr.status == 200) {
                    progressBar.style.width = '100%';
                    progressBar.textContent = '✓ Complete';
                    alert('✅ Upload successful! Device will reboot in 3 seconds...');
                    setTimeout(function(){ 
                        window.location.reload(); 
                    }, 5000);
                } else {
                    alert('❌ Upload failed: ' + xhr.responseText);
                    progressDiv.style.display = 'none';
                    uploadBtn.disabled = false;
                    uploadBtn.textContent = 'Upload Payload';
                }
            };
            
            xhr.onerror = function() {
                alert('❌ Upload error occurred');
                progressDiv.style.display = 'none';
                uploadBtn.disabled = false;
                uploadBtn.textContent = 'Upload Payload';
            };
            
            xhr.open('POST', '/upload');
            xhr.send(formData);
        }
        
        function deletePayload() {
            if (confirm('⚠️ Delete payload.bin? Device will reboot.')) {
                fetch('/delete')
                    .then(response => response.text())
                    .then(data => {
                        alert('✅ ' + data + ' Rebooting...');
                        setTimeout(function(){ window.location.reload(); }, 5000);
                    })
                    .catch(error => {
                        alert('❌ Delete failed: ' + error);
                    });
            }
        }
        
        function reboot() {
            if (confirm('🔄 Reboot device?')) {
                fetch('/reboot')
                    .then(() => {
                        alert('✅ Rebooting... Page will reload in 10 seconds.');
                        setTimeout(function(){ window.location.reload(); }, 10000);
                    })
                    .catch(error => {
                        alert('Device is rebooting...');
                        setTimeout(function(){ window.location.reload(); }, 10000);
                    });
            }
        }
    </script>
</body>
</html>
)rawliteral";

// ===== Web Handlers =====
void handleRoot() {
    String html = String(upload_html);
    
    // Payload status
    FILE *f = fopen("/data/payload.bin", "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        
        char payload_info[200];
        sprintf(payload_info, "<span class='status-good'>✓ Found (%ld bytes / %.1f KB)</span>", 
                size, size / 1024.0);
        html.replace("PAYLOAD_INFO", payload_info);
    } else {
        html.replace("PAYLOAD_INFO", "<span class='status-warn'>⚠ Not Found</span>");
    }
    
    // WiFi info
    if (USE_WIFI_STATION) {
        String wifi_info = "WiFi: " + WiFi.SSID();
        html.replace("WIFI_INFO", wifi_info);
    } else {
        html.replace("WIFI_INFO", "Access Point Mode");
    }
    
    html.replace("IP_ADDRESS", WiFi.localIP().toString());
    
    char free_heap[20];
    sprintf(free_heap, "%d", ESP.getFreeHeap() / 1024);
    html.replace("FREE_HEAP", free_heap);
    
    server.send(200, "text/html", html);
}

void handleUpload() {
    HTTPUpload& upload = server.upload();
    static FILE *file = NULL;
    static size_t total_received = 0;
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[WEB] Upload started: %s\n", upload.filename.c_str());
        file = fopen("/data/payload.bin", "wb");
        total_received = 0;
        
        if (!file) {
            Serial.println("[ERROR] Failed to open file for writing");
            server.send(500, "text/plain", "Failed to open file");
            return;
        }
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (file) {
            size_t written = fwrite(upload.buf, 1, upload.currentSize, file);
            total_received += written;
            
            if (total_received % 10240 == 0) {
                Serial.printf("[WEB] Received: %d KB\n", total_received / 1024);
            }
        }
    } 
    else if (upload.status == UPLOAD_FILE_END) {
        if (file) {
            fclose(file);
            file = NULL;
            Serial.printf("[WEB] Upload complete: %d bytes (%.1f KB)\n", 
                         total_received, total_received / 1024.0);
            
            // Verify
            FILE *verify = fopen("/data/payload.bin", "rb");
            if (verify) {
                fseek(verify, 0, SEEK_END);
                long size = ftell(verify);
                fclose(verify);
                
                if (size == total_received) {
                    Serial.println("[WEB] Verification OK");
                    server.send(200, "text/plain", "Upload successful");
                    delay(1000);
                    ESP.restart();
                } else {
                    Serial.printf("[ERROR] Size mismatch: expected %d, got %ld\n", 
                                 total_received, size);
                    server.send(500, "text/plain", "Upload verification failed");
                }
            }
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (file) {
            fclose(file);
            file = NULL;
        }
        Serial.println("[WEB] Upload aborted");
        server.send(500, "text/plain", "Upload aborted");
    }
}

void handleDelete() {
    Serial.println("[WEB] Deleting payload...");
    if (remove("/data/payload.bin") == 0) {
        server.send(200, "text/plain", "Payload deleted");
        delay(1000);
        ESP.restart();
    } else {
        server.send(500, "text/plain", "Delete failed - file may not exist");
    }
}

void handleReboot() {
    server.send(200, "text/plain", "Rebooting...");
    Serial.println("[WEB] Reboot requested");
    delay(1000);
    ESP.restart();
}

// ===== WiFi Setup =====
void setup_wifi() {
    if (USE_WIFI_STATION) {
        // Connect to existing WiFi
        Serial.println("\n[WiFi] Station Mode - Connecting...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[WiFi] Connected!");
            Serial.printf("[WiFi] SSID: %s\n", WiFi.SSID().c_str());
            Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] 🌐 Open: http://%s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println("\n[WiFi] Connection failed - continuing without WiFi");
            return;
        }
    } else {
        // Create Access Point
        Serial.println("\n[WiFi] Access Point Mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        
        Serial.printf("[WiFi] SSID: %s\n", AP_SSID);
        Serial.printf("[WiFi] Password: %s\n", AP_PASSWORD);
        Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.println("[WiFi] 🌐 Open: http://192.168.4.1");
    }
    
    // Setup web server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/upload", HTTP_POST, [](){
        server.send(200);
    }, handleUpload);
    server.on("/delete", HTTP_GET, handleDelete);
    server.on("/reboot", HTTP_GET, handleReboot);
    
    server.begin();
    Serial.println("[WEB] Server started");
}

// ===== Storage Functions =====
void check_partitions(void) {
    Serial.println("\n=== PARTITION TABLE ===");
    
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA, 
                                                     ESP_PARTITION_SUBTYPE_DATA_FAT, 
                                                     NULL);
    if (it == NULL) {
        Serial.println("[ERROR] FAT partition not found!");
        Serial.println("[FIX] Select: Tools → Partition Scheme → '16M Flash (3MB APP/9.9MB FATFS)'");
        return;
    }
    
    const esp_partition_t *partition = esp_partition_get(it);
    Serial.printf("[OK] FAT partition found: %s\n", partition->label);
    Serial.printf("  Address: 0x%X\n", partition->address);
    Serial.printf("  Size: %d bytes (%.1f MB)\n", partition->size, partition->size / (1024.0 * 1024.0));
    
    esp_partition_iterator_release(it);
    Serial.println("========================\n");
}

static esp_err_t init_internal_storage(void) {
    Serial.println("[STORAGE] Initializing...");
    
    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 4,
        .allocation_unit_size = 4096
    };
    
    wl_handle_t wl_handle;
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl("/data", "ffat", &mount_config, &wl_handle);
    
    if (err != ESP_OK) {
        Serial.printf("[ERROR] FATFS mount failed: %s\n", esp_err_to_name(err));
        return err;
    }
    
    Serial.println("[OK] FATFS mounted at /data");
    
    // Test write
    FILE *test = fopen("/data/test.txt", "w");
    if (test) {
        fprintf(test, "test");
        fclose(test);
        Serial.println("[OK] Write test successful");
        remove("/data/test.txt");
    } else {
        Serial.println("[ERROR] Write test FAILED!");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// ===== USB/RCM Functions =====
static void dummy_cb(usb_transfer_t *transfer) { }

static void usb_event_cb(const usb_host_client_event_msg_t *event, void *arg) {
    if (event->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        usb_device_handle_t test_hdl;
        esp_err_t err = usb_host_device_open(client_hdl, event->new_dev.address, &test_hdl);
        if (err == ESP_OK) {
            const usb_device_desc_t *dev_desc;
            err = usb_host_get_device_descriptor(test_hdl, &dev_desc);
            if (err == ESP_OK && dev_desc->idVendor == APX_VID && dev_desc->idProduct == APX_PID) {
                LOG_I("*** SWITCH RCM DETECTED ***");
                dev_hdl = test_hdl;
                device_connected = true;
                return;
            }
            usb_host_device_close(client_hdl, test_hdl);
        }
    } else if (event->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        device_connected = false;
        dev_hdl = NULL;
    }
}

static bool wait_for_transfer(usb_transfer_t *xfer, uint32_t timeout_ms, size_t expected_bytes) {
    uint32_t waited = 0;
    while (waited < timeout_ms) {
        if (xfer->status != 0 || xfer->actual_num_bytes >= expected_bytes) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        waited++;
    }
    return false;
}

static bool read_device_id(void) {
    LOG_I("Reading Device ID...");
    
    usb_transfer_t *xfer = NULL;
    if (usb_host_transfer_alloc(64, 0, &xfer) != ESP_OK) return false;
    
    xfer->device_handle = dev_hdl;
    xfer->bEndpointAddress = 0x81;
    xfer->callback = dummy_cb;
    xfer->timeout_ms = 3000;
    xfer->num_bytes = 64;
    
    if (usb_host_transfer_submit(xfer) != ESP_OK) {
        usb_host_transfer_free(xfer);
        return false;
    }
    
    bool done = wait_for_transfer(xfer, 3000, 16);
    
    bool success = false;
    if (done && xfer->actual_num_bytes >= 16) {
        char ascii_buf[33] = {0};
        for (int i = 0; i < 16; i++) sprintf(&ascii_buf[i*2], "%02x", xfer->data_buffer[i]);
        LOG_I("Device ID: %s", ascii_buf);
        success = true;
    }
    
    usb_host_transfer_free(xfer);
    return success || done;
}

static bool send_chunk(uint8_t *data, size_t len) {
    usb_transfer_t *xfer = NULL;
    if (usb_host_transfer_alloc(len, 0, &xfer) != ESP_OK) return false;
    
    memcpy(xfer->data_buffer, data, len);
    xfer->device_handle = dev_hdl;
    xfer->bEndpointAddress = 0x01;
    xfer->callback = dummy_cb;
    xfer->timeout_ms = 5000;
    xfer->num_bytes = len;
    
    if (usb_host_transfer_submit(xfer) != ESP_OK) {
        usb_host_transfer_free(xfer);
        return false;
    }
    
    bool done = wait_for_transfer(xfer, 5000, len);
    bool success = (done && (xfer->status == USB_TRANSFER_STATUS_COMPLETED || xfer->actual_num_bytes == len));
    
    usb_host_transfer_free(xfer);
    return success;
}

static void delay_2ms(void) {
    for (volatile int i = 0; i < 480000; i++);
}

static bool send_payload(uint8_t *payload_buf, uint32_t payload_len) {
    LOG_I("Sending %d bytes...", payload_len);
    
    int chunks = 0;
    for (uint32_t offset = 0; offset < payload_len; offset += SEND_CHUNK_SIZE) {
        if (!send_chunk(&payload_buf[offset], SEND_CHUNK_SIZE)) {
            LOG_E("Failed at chunk %d", chunks);
            break;
        }
        chunks++;
        if (chunks % 50 == 0) LOG_I("Sent %d chunks", chunks);
        delay_2ms();
    }
    
    LOG_I("Sent %d chunks", chunks);
    
    if ((chunks % 2) != 1) {
        uint8_t zero[SEND_CHUNK_SIZE] = {0};
        send_chunk(zero, SEND_CHUNK_SIZE);
    }
    
    return chunks > 0;
}

static void smash_stack(void) {
    LOG_I("Smashing stack...");
    
    size_t total_size = 8 + 0x7000;
    uint8_t *buffer = (uint8_t*)heap_caps_aligned_alloc(64, total_size, MALLOC_CAP_DMA);
    if (!buffer) return;
    
    buffer[0] = 0x82; buffer[1] = 0x00; buffer[2] = 0x00; buffer[3] = 0x00;
    buffer[4] = 0x00; buffer[5] = 0x00; buffer[6] = 0x00; buffer[7] = 0x70;
    memset(buffer + 8, 0, 0x7000);
    
    usb_transfer_t *xfer = NULL;
    if (usb_host_transfer_alloc(total_size, 0, &xfer) != ESP_OK) {
        heap_caps_free(buffer);
        return;
    }
    
    memcpy(xfer->data_buffer, buffer, total_size);
    heap_caps_free(buffer);
    
    xfer->device_handle = dev_hdl;
    xfer->bEndpointAddress = 0;
    xfer->callback = dummy_cb;
    xfer->timeout_ms = 1000;
    xfer->num_bytes = 0x7008;
    
    if (usb_host_transfer_submit_control(client_hdl, xfer) != ESP_OK) {
        LOG_E("Submit failed");
        usb_host_transfer_free(xfer);
        return;
    }
    
    wait_for_transfer(xfer, 1000, 0);
    LOG_I("Smash result: status=%d (error/timeout expected)", xfer->status);
    usb_host_transfer_free(xfer);
}

static void inject_payload(void) {
    LOG_I("=== INJECTION START ===");
    
    if (usb_host_interface_claim(client_hdl, dev_hdl, 0, 0) != ESP_OK) {
        LOG_E("Claim failed");
        return;
    }
    LOG_I("Interface claimed");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    read_device_id();
    
    uint8_t *payload_buf = (uint8_t*)heap_caps_aligned_alloc(64, MAX_LENGTH, MALLOC_CAP_DMA);
    if (!payload_buf) return;
    
    memset(payload_buf, 0, MAX_LENGTH);
    *(uint32_t*)payload_buf = MAX_LENGTH;
    uint32_t idx = 0x2a8;
    
    uint32_t *spray = (uint32_t*)&payload_buf[idx];
    for (uint32_t addr = RCM_PAYLOAD_ADDR; addr < INTERMEZZO_LOCATION; addr += 4) {
        *spray++ = INTERMEZZO_LOCATION;
        idx += 4;
    }
    
    memcpy(&payload_buf[idx], intermezzo_bin, sizeof(intermezzo_bin));
    idx += sizeof(intermezzo_bin);
    idx = (PAYLOAD_LOAD_BLOCK - RCM_PAYLOAD_ADDR) + 0x2a8;
    
    FILE *f = fopen("/data/payload.bin", "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        fread(&payload_buf[idx], 1, fsize, f);
        fclose(f);
        idx += fsize;
    }
    
    LOG_I("Payload: %d bytes", idx);
    send_payload(payload_buf, idx);
    heap_caps_free(payload_buf);
    
    smash_stack();
    
    LOG_I("=== DONE ===");
    usb_host_interface_release(client_hdl, dev_hdl, 0);
}

// ===== FreeRTOS Tasks =====
static void usb_host_task(void *arg) {
    while (1) usb_host_lib_handle_events(portMAX_DELAY, NULL);
}

static void injection_task(void *arg) {
    usb_host_client_config_t cfg = {
        .max_num_event_msg = 5,
        .async = {.client_event_callback = usb_event_cb, .callback_arg = NULL}
    };
    esp_err_t err = usb_host_client_register(&cfg, &client_hdl);
    if (err != ESP_OK) {
        LOG_E("USB client registration failed: %d", err);
        vTaskDelete(NULL);
        return;
    }
    LOG_I("USB Client registered, waiting for events...");
    
    while (1) {
        usb_host_client_handle_events(client_hdl, portMAX_DELAY);
        if (device_connected && !injection_done) {
            injection_done = true;
            inject_payload();
            while (device_connected) vTaskDelay(pdMS_TO_TICKS(100));
            injection_done = false;
        }
    }
}

// ===== Main Setup =====
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n================================");
    Serial.println("RCM Injector v6.0 - Web Edition");
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    if (psramFound()) {
        Serial.printf("PSRAM: %d bytes (%.1f MB)\n", ESP.getPsramSize(), ESP.getPsramSize() / (1024.0 * 1024.0));
    }
    Serial.println("================================");
    
    // Check partitions
    Serial.println("\n[STEP 1] Checking partitions...");
    check_partitions();
    
    // Initialize storage
    Serial.println("[STEP 2] Initializing storage...");
    esp_err_t storage_err = init_internal_storage();
    if (storage_err != ESP_OK) {
        Serial.printf("[FATAL] Storage init failed: %s\n", esp_err_to_name(storage_err));
        while(1) {
            delay(1000);
            Serial.print("X");
        }
    }
    Serial.println("[OK] Storage initialized");
    
    // Setup WiFi and web server
    Serial.println("\n[STEP 3] Starting WiFi and Web Server...");
    setup_wifi();
    
    // Check payload
    Serial.println("\n[STEP 4] Checking for payload...");
    bool payload_valid = false;
    long payload_size = 0;
    const long MIN_PAYLOAD_SIZE = 50000;  // 50KB minimum
    
    FILE *t = fopen("/data/payload.bin", "rb");
    if (t != NULL) {
        fseek(t, 0, SEEK_END);
        payload_size = ftell(t);
        fclose(t);
        
        Serial.printf("[INFO] Found payload.bin: %ld bytes (%.1f KB)\n", 
                     payload_size, payload_size / 1024.0);
        
        if (payload_size < MIN_PAYLOAD_SIZE) {
            Serial.printf("[WARNING] Payload too small (< %ld bytes)\n", MIN_PAYLOAD_SIZE);
            Serial.println("[INFO] This might be corrupt - please upload via web interface");
        } else {
            Serial.println("[OK] Payload size looks valid");
            payload_valid = true;
        }
    } else {
        Serial.println("[INFO] payload.bin NOT FOUND");
        Serial.println("[INFO] Please upload via web interface");
    }
    
    if (!payload_valid) {
        Serial.println("\n⚠️  NO VALID PAYLOAD - UPLOAD REQUIRED");
        Serial.println("📱 Use web interface to upload payload.bin");
        if (USE_WIFI_STATION && WiFi.status() == WL_CONNECTED) {
            Serial.printf("🌐 Open: http://%s\n", WiFi.localIP().toString().c_str());
        } else if (!USE_WIFI_STATION) {
            Serial.println("🌐 Open: http://192.168.4.1");
        }
        Serial.println("\nDevice will wait for web upload...");
        // Don't start USB host without payload
    } else {
        // Start USB host only if payload exists
        Serial.println("\n[STEP 5] Installing USB Host...");
        
        usb_host_config_t host_config = {
            .skip_phy_setup = false,
            .intr_flags = ESP_INTR_FLAG_LEVEL1
        };
        
        esp_err_t err = usb_host_install(&host_config);
        if (err != ESP_OK) {
            Serial.printf("[ERROR] USB Host install failed: %s (%d)\n", 
                         esp_err_to_name(err), err);
            while(1) delay(1000);
        }
        Serial.println("[OK] USB Host installed");
        
        Serial.println("\n[STEP 6] Creating tasks...");
        
        xTaskCreatePinnedToCore(usb_host_task, "usb_host", 4096, NULL, 20, NULL, 0);
        Serial.println("[OK] usb_host_task created");
        
        delay(100);
        
        xTaskCreatePinnedToCore(injection_task, "inject", 8192, NULL, 19, NULL, 0);
        Serial.println("[OK] injection_task created");
        
        Serial.println("\n================================");
        Serial.println("✅ SETUP COMPLETE");
        Serial.println("🔌 Waiting for Nintendo Switch in RCM mode...");
        Serial.println("================================\n");
    }
}

void loop() {
    // Handle web server requests
    server.handleClient();
    
    // Small delay to prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(10));
}
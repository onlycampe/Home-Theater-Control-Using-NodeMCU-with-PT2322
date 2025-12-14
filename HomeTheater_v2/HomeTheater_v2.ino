// =============================================================================
// HOME THEATER CONTROL - ESP8266 NodeMCU + PT2322
// =============================================================================
// Version 2.5 - Advanced Features
// Author: OnlyCampe
// Date: 12/13/2025
//
// IMPROVEMENTS IN THIS VERSION:
// ✅ Non-blocking delays (always responsive system)
// ✅ HTML moved to PROGMEM (saves ~3.5KB of RAM)
// ✅ Credentials in separate file (security)
// ✅ Clean and optimized code
// 🌟 OTA Updates (update firmware over WiFi without USB cable!)
// 🌟 Complete status/diagnostic page
// 🌟 Improved logging system
// 🌟 Responsive interface (mobile + desktop)
// 💡 Default values (temporary changes, reset on reboot)
// =============================================================================

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "PT2322.h"
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <SinricProSpeaker.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>      // 🌟 OTA Updates

// Include configuration files
#include "config.h"          // ⚠️ Your credentials (rename config.example.h)
#include "webpages.h"        // HTML stored in Flash (PROGMEM)

// =============================================================================
// OBJETOS GLOBAIS
// =============================================================================
IRrecv irrecv(RECV_PIN);
decode_results results;
SinricProSpeaker& speaker = SinricPro[SPEAKER_DEVICE_ID];
SinricProSwitch& switchDevice = SinricPro[SWITCH_DEVICE_ID];
ESP8266WebServer server(80);
PT2322 pt;

// =============================================================================
// STATE VARIABLES
// =============================================================================
bool isSystemOn = false;
bool isBluetoothOn = false;
bool isMuteOn = false;
bool isDddOn = true;
bool loggingEnabled = ENABLE_SERIAL_LOGGING;

// Test Channel State
bool testToneActive = false;
String activeTestChannel = "";

// ✅ NEW: Variables for non-blocking power control
enum PowerSequenceState {
    POWER_IDLE,
    POWER_ON_STEP1,
    POWER_OFF_STEP1
};
PowerSequenceState powerSequence = POWER_IDLE;
unsigned long powerStateChangeTime = 0;
bool targetPowerState = false;

// =============================================================================
// VOLUME AND AUDIO VARIABLES (DEFAULT VALUES)
// =============================================================================
// 💡 These values are restored on each reboot
int currentTotalVol = 53;
int currentCenterVol = 10;
int currentSubVol = 12;
int currentFrontVol = 10;
int currentRearVol = 10;

unsigned long lastVolumeUpdateTime = 0;
const unsigned long volumeUpdateInterval = 1000;
int lastVolumeSent = -1;

// =============================================================================
// 🌟 BUFFER POOL FOR MEMORY OPTIMIZATION
// =============================================================================
static char smallBuffer[128];
static char mediumBuffer[256];
static char largeBuffer[512];

// =============================================================================
// FUNCTION DECLARATIONS (prototypes)
// =============================================================================
bool onPowerState(const String &deviceId, bool &state);
bool onSetVolume(const String &deviceId, int volume);
bool onMute(const String &deviceId, bool &state);
void handlePowerSequence();
void setupOTA();
void logMessage(const char* level, const String& message);
inline String maskIP(const IPAddress& ip);

// =============================================================================
// OPTIMIZED HELPER FUNCTIONS
// =============================================================================
// IP mask for logs (avoids full exposure)
inline String maskIP(const IPAddress& ip) {
    snprintf_P(smallBuffer, sizeof(smallBuffer), 
               PSTR("%d.%d.xxx.xxx"), ip[0], ip[1]);
    return String(smallBuffer);
}

// =============================================================================
// OPTIMIZED LOGGING SYSTEM
// =============================================================================
void logMessage(const char* level, const String& message) {
    if (!loggingEnabled) return;
    
    unsigned long uptime = millis() / 1000;
    snprintf_P(smallBuffer, sizeof(smallBuffer), 
               PSTR("[%lus] [%s] "), uptime, level);
    Serial.print(smallBuffer);
    Serial.println(message);
}

// =============================================================================
// PIN CONTROL FUNCTIONS
// =============================================================================
void togglePin(int pin, bool state, const String &message) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state ? HIGH : LOW);
    if (loggingEnabled && !message.isEmpty()) {
        logMessage("INFO", message);
    }
}

// =============================================================================
// STATE UPDATE FUNCTIONS
// =============================================================================
void updatePowerState(bool state) {
    speaker.sendPowerStateEvent(state);
}

void updateVolume(int volume) {
    unsigned long currentTime = millis();
    lastVolumeSent = volume;
    lastVolumeUpdateTime = currentTime;
}

// =============================================================================
// ✅ NEW: NON-BLOCKING POWER CONTROL
// =============================================================================
// This function starts the on/off sequence without using delay()
// Processing happens in loop through handlePowerSequence()
bool setPowerState(bool &state) {
    targetPowerState = state;
    powerStateChangeTime = millis();
    
    if (state) {
        // Start POWER ON sequence
        togglePin(PSON_PIN, true, loggingEnabled ? "PSON turned on" : "");
        powerSequence = POWER_ON_STEP1;
    } else {
        // Start POWER OFF sequence
        togglePin(RELAY_PIN, false, loggingEnabled ? "Relay turned off" : "");
        powerSequence = POWER_OFF_STEP1;
    }
    return true;
}

// ✅ NEW: Process power sequence in non-blocking way
void handlePowerSequence() {
    if (powerSequence == POWER_IDLE) return;
    
    unsigned long currentTime = millis();
    
    switch(powerSequence) {
        case POWER_ON_STEP1:
            // Aguarda 2 segundos antes de ligar o relay
            if (currentTime - powerStateChangeTime >= 2000) {
                togglePin(RELAY_PIN, true, loggingEnabled ? "Relay turned on" : "");
                isSystemOn = true;
                updatePowerState(true);
                powerSequence = POWER_IDLE;
            }
            break;
            
        case POWER_OFF_STEP1:
            // Aguarda 2 segundos antes de desligar o PSON
            if (currentTime - powerStateChangeTime >= 2000) {
                togglePin(PSON_PIN, false, loggingEnabled ? "PSON turned off" : "");
                isSystemOn = false;
                updatePowerState(false);
                powerSequence = POWER_IDLE;
            }
            break;
            
        default:
            powerSequence = POWER_IDLE;
            break;
    }
}

// =============================================================================
// CALLBACKS DO SINRICPRO
// =============================================================================
bool onPowerState(const String &deviceId, bool &state) {
    if (loggingEnabled) {
        String source = deviceId.isEmpty() ? "WEB" : "SinricPro";
        logMessage("INFO", "[" + source + "] Power " + String(state ? "ON" : "OFF"));
    }
    return setPowerState(state);
}

bool onSetVolume(const String &deviceId, int volume) {
    currentTotalVol = constrain(volume, 0, 79);
    pt.setVol(currentTotalVol);
    updateVolume(currentTotalVol);
    if (loggingEnabled) {
        String source = deviceId.isEmpty() ? "WEB/IR" : "SinricPro";
        int percent = map(currentTotalVol, 0, 79, 0, 100);
        logMessage("INFO", "[" + source + "] Volume: " + String(currentTotalVol) + "/79 (" + String(percent) + "%)");
    }
    return true;
}

bool onMute(const String &deviceId, bool &mute) {
    isMuteOn = mute;
    int currentMute = mute ? 1 : 0;
    int currentDdd = isDddOn ? 1 : 0;
    pt.setFunc(currentMute, currentDdd, 0);
    
    if (loggingEnabled) {
        String source = deviceId.isEmpty() ? "WEB" : "SinricPro";
        logMessage("INFO", "[" + source + "] Mute " + String(mute ? "ON" : "OFF"));
    }
    return true;
}

bool onDdd(const String &deviceId, bool &state) {
    isDddOn = state;
    int currentMute = isMuteOn ? 1 : 0;
    int currentDdd = state ? 1 : 0;
    pt.setFunc(currentMute, currentDdd, 0);
    
    if (loggingEnabled) {
        String source = deviceId.isEmpty() ? "WEB" : "SinricPro";
        logMessage("INFO", "[" + source + "] DDD " + String(state ? "ON" : "OFF"));
    }
    return true;
}

// =============================================================================
// 🌟 HTML STATUS PAGE HANDLER (OPTIMIZED)
// =============================================================================
void handleStatusPage() {
    logMessage("WEB", "Status page: " + maskIP(server.client().remoteIP()));
    
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(STATUS_PAGE_HTML);
    
    // Card 1: System (using reusable buffer)
    server.sendContent_P(PSTR("<div class='card'><h2><i class='fa-solid fa-microchip'></i> System</h2><div class='info-grid'>"));
    server.sendContent_P(PSTR("<span class='label'>Version:</span><span class='value'>2.5</span>"));
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("<span class='label'>Uptime:</span><span class='value'>%lus (%lu min)</span>"),
               millis() / 1000, millis() / 60000);
    server.sendContent(largeBuffer);
    
    snprintf_P(mediumBuffer, sizeof(mediumBuffer),
               PSTR("<span class='label'>Chip ID:</span><span class='value'>%lx</span>"),
               ESP.getChipId());
    server.sendContent(mediumBuffer);
    
    snprintf_P(mediumBuffer, sizeof(mediumBuffer),
               PSTR("<span class='label'>SDK Version:</span><span class='value'>%s</span>"),
               ESP.getSdkVersion());
    server.sendContent(mediumBuffer);
    
    server.sendContent_P(PSTR("<span class='label'>Core Version:</span><span class='value'>"));
    server.sendContent(ESP.getCoreVersion());
    server.sendContent_P(PSTR("</span>"));
    
    snprintf_P(mediumBuffer, sizeof(mediumBuffer),
               PSTR("<span class='label'>Boot Version:</span><span class='value'>%d</span>"
                    "<span class='label'>Boot Mode:</span><span class='value'>%d</span>"),
               ESP.getBootVersion(), ESP.getBootMode());
    server.sendContent(mediumBuffer);
    
    server.sendContent_P(PSTR("<span class='label'>Reset Reason:</span><span class='value'>"));
    server.sendContent(ESP.getResetReason());
    server.sendContent_P(PSTR("</span></div></div>"));
    
    yield(); // Watchdog
    yield(); // Watchdog
    
    // Card 2: CPU & Flash
    const char* flashMode = "Unknown";
    switch(ESP.getFlashChipMode()) {
        case 0: flashMode = "QIO"; break;
        case 1: flashMode = "QOUT"; break;
        case 2: flashMode = "DIO"; break;
        case 3: flashMode = "DOUT"; break;
    }
    
    server.sendContent_P(PSTR("<div class='card'><h2><i class='fa-solid fa-bolt'></i> CPU & Flash</h2><div class='info-grid'>"));
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("<span class='label'>CPU Freq:</span><span class='value status-ok'>%d MHz</span>"
                    "<span class='label'>Flash Size:</span><span class='value'>%lu KB</span>"
                    "<span class='label'>Flash Speed:</span><span class='value'>%lu MHz</span>"
                    "<span class='label'>Flash Mode:</span><span class='value'>%s</span>"),
               ESP.getCpuFreqMHz(),
               ESP.getFlashChipSize() / 1024,
               ESP.getFlashChipSpeed() / 1000000,
               flashMode);
    server.sendContent(largeBuffer);
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("<span class='label'>Sketch Size:</span><span class='value'>%lu KB</span>"
                    "<span class='label'>Free Space:</span><span class='value'>%lu KB</span>"),
               ESP.getSketchSize() / 1024,
               ESP.getFreeSketchSpace() / 1024);
    server.sendContent(largeBuffer);
    
    server.sendContent_P(PSTR("<span class='label'>Sketch MD5:</span><span class='value' style='font-size:0.8em;'>"));
    server.sendContent(ESP.getSketchMD5());
    server.sendContent_P(PSTR("</span></div></div>"));
    
    yield();
    yield();
    
    // Card 3: Memory
    int freeHeap = ESP.getFreeHeap();
    int heapPercent = (freeHeap * 100) / 80192;
    const char* heapClass = heapPercent > 30 ? "status-ok" : "status-warn";
    
    server.sendContent_P(PSTR("<div class='card'><h2><i class='fa-solid fa-memory'></i> Memória RAM</h2><div class='info-grid'>"));
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("<span class='label'>Free RAM:</span><span class='value %s'>%d bytes (%d%%)</span>"
                    "<span class='label'>Max Free Block:</span><span class='value'>%d bytes</span>"
                    "<span class='label'>Fragmentation:</span><span class='value'>%d%%</span>"),
               heapClass, freeHeap, heapPercent,
               ESP.getMaxFreeBlockSize(),
               ESP.getHeapFragmentation());
    server.sendContent(largeBuffer);
    server.sendContent_P(PSTR("</div></div>"));
    
    yield();
    
    // Card 4: WiFi
    int rssi = WiFi.RSSI();
    const char* signalClass = rssi > -60 ? "status-ok" : "status-warn";
    
    server.sendContent_P(PSTR("<div class='card'><h2><i class='fa-solid fa-wifi'></i> WiFi</h2><div class='info-grid'>"));
    
    server.sendContent_P(PSTR("<span class='label'>SSID:</span><span class='value'>"));
    server.sendContent(WiFi.SSID());
    server.sendContent_P(PSTR("</span>"));
    
    snprintf_P(mediumBuffer, sizeof(mediumBuffer),
               PSTR("<span class='label'>Channel:</span><span class='value'>%d</span>"),
               WiFi.channel());
    server.sendContent(mediumBuffer);
    
    server.sendContent_P(PSTR("<span class='label'>IP:</span><span class='value'>"));
    server.sendContent(WiFi.localIP().toString());
    server.sendContent_P(PSTR("</span><span class='label'>MAC:</span><span class='value'>"));
    server.sendContent(WiFi.macAddress());
    server.sendContent_P(PSTR("</span>"));
    
    snprintf_P(mediumBuffer, sizeof(mediumBuffer),
               PSTR("<span class='label'>Signal:</span><span class='value %s'>%d dBm</span>"),
               signalClass, rssi);
    server.sendContent(mediumBuffer);
    server.sendContent_P(PSTR("</div></div>"));
    
    yield();
    
    // Card 5: Home Theater Status
    server.sendContent_P(PSTR("<div class='card'><h2><i class='fa-solid fa-volume-high'></i> Home Theater</h2><div class='info-grid'>"));
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("<span class='label'>System:</span><span class='value%s'>%s</span>"
                    "<span class='label'>Bluetooth:</span><span class='value%s'>%s</span>"
                    "<span class='label'>Mute:</span><span class='value%s'>%s</span>"
                    "<span class='label'>DDD:</span><span class='value%s'>%s</span>"),
               isSystemOn ? " status-ok" : "", isSystemOn ? "LIGADO" : "DESLIGADO",
               isBluetoothOn ? " status-ok" : "", isBluetoothOn ? "ON" : "OFF",
               isMuteOn ? " status-warn" : "", isMuteOn ? "ON" : "OFF",
               isDddOn ? " status-ok" : "", isDddOn ? "ON" : "OFF");
    server.sendContent(largeBuffer);
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("<span class='label'>Total Volume:</span><span class='value'>%d/79 (%d%%)</span>"
                    "<span class='label'>Center:</span><span class='value'>%d/15</span>"
                    "<span class='label'>Subwoofer:</span><span class='value'>%d/15</span>"
                    "<span class='label'>Front L/R:</span><span class='value'>%d/15</span>"
                    "<span class='label'>Rear L/R:</span><span class='value'>%d/15</span>"),
               currentTotalVol, map(currentTotalVol, 0, 79, 0, 100),
               currentCenterVol, currentSubVol, currentFrontVol, currentRearVol);
    server.sendContent(largeBuffer);
    server.sendContent_P(PSTR("</div></div>"));
    
    server.sendContent_P(STATUS_PAGE_FOOTER);
    server.sendContent("");
}

// =============================================================================
// 🌟 HANDLER DA API DE STATUS JSON (OTIMIZADO)
// =============================================================================
void handleStatus() {
    // Usa largeBuffer para construir JSON de forma eficiente
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("{\"version\":\"2.5\",\"uptime\":%lu,\"freeHeap\":%d,\"heapFragmentation\":%d,"
                    "\"maxFreeBlockSize\":%d,\"chipId\":\"%lx\",\"cpuFreqMHz\":%d,"),
               millis() / 1000, ESP.getFreeHeap(), ESP.getHeapFragmentation(),
               ESP.getMaxFreeBlockSize(), ESP.getChipId(), ESP.getCpuFreqMHz());
    
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", largeBuffer);
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("\"flashChipSize\":%lu,\"flashChipSpeed\":%lu,\"flashChipMode\":%d,"
                    "\"sketchSize\":%lu,\"freeSketchSpace\":%lu,"),
               ESP.getFlashChipSize(), ESP.getFlashChipSpeed() / 1000000,
               ESP.getFlashChipMode(), ESP.getSketchSize(), ESP.getFreeSketchSpace());
    server.sendContent(largeBuffer);
    
    server.sendContent_P(PSTR("\"sketchMD5\":\""));
    server.sendContent(ESP.getSketchMD5());
    server.sendContent_P(PSTR("\",\"sdkVersion\":\""));
    server.sendContent(ESP.getSdkVersion());
    server.sendContent_P(PSTR("\",\"coreVersion\":\""));
    server.sendContent(ESP.getCoreVersion());
    server.sendContent_P(PSTR("\","));
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("\"bootVersion\":%d,\"bootMode\":%d,\"resetReason\":\""),
               ESP.getBootVersion(), ESP.getBootMode());
    server.sendContent(largeBuffer);
    server.sendContent(ESP.getResetReason());
    server.sendContent_P(PSTR("\",\"resetInfo\":\""));
    server.sendContent(ESP.getResetInfo());
    server.sendContent_P(PSTR("\","));
    
    snprintf_P(largeBuffer, sizeof(largeBuffer),
               PSTR("\"wifiSignal\":%d,\"ip\":\"%s\",\"mac\":\"%s\",\"ssid\":\"%s\","
                    "\"wifiChannel\":%d,\"isSystemOn\":%s,\"isBluetoothOn\":%s,"
                    "\"isMuteOn\":%s,\"isDddOn\":%s,\"volume\":%d,\"centerVol\":%d,"
                    "\"subVol\":%d,\"frontVol\":%d,\"rearVol\":%d}"),
               WiFi.RSSI(), WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str(),
               WiFi.SSID().c_str(), WiFi.channel(),
               isSystemOn ? "true" : "false", isBluetoothOn ? "true" : "false",
               isMuteOn ? "true" : "false", isDddOn ? "true" : "false",
               currentTotalVol, currentCenterVol, currentSubVol, currentFrontVol, currentRearVol);
    server.sendContent(largeBuffer);
    server.sendContent("");
    
    logMessage("API", "Status: " + maskIP(server.client().remoteIP()));
}

// =============================================================================
// ✅ WEB PAGE HANDLER (OPTIMIZED)
// =============================================================================
void handleRoot() {
    logMessage("WEB", "Página: " + maskIP(server.client().remoteIP()));
    
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(HTML_HEADER);
    
    // Generate buttons using reusable buffer
    struct Button { const char* id; const char* icon; const char* iconClass; bool state; };
    const Button buttons[] = {
        {"system", "power-off", "fa-solid", isSystemOn},
        {"bluetooth", "bluetooth-b", "fa-brands", isBluetoothOn},
        {"mute", "volume-mute", "fa-solid", isMuteOn},
        {"ddd", "cube", "fa-solid", isDddOn},
        {"audioIn", "music", "fa-solid", false},
        {"audio51", "arrows-left-right-to-line", "fa-solid", false}
    };
    
    for (const Button& btn : buttons) {
        const char* btnClass = (strcmp(btn.id, "audioIn") == 0 || strcmp(btn.id, "audio51") == 0) 
                               ? "red" : (btn.state ? "green" : "red");
        
        snprintf_P(mediumBuffer, sizeof(mediumBuffer),
                   PSTR("<button id='%sButton' class='%s' onclick='toggleButton(\"%s\")'>"
                        "<i class='%s fa-%s'></i></button>"),
                   btn.id, btnClass, btn.id, btn.iconClass, btn.icon);
        server.sendContent(mediumBuffer);
    }
    
    // Generate sliders using reusable buffer
    struct Slider { const char* id; const char* label; int min; int max; int value; };
    const Slider sliders[] = {
        {"total", "Total Volume", 0, 79, currentTotalVol},
        {"center", "Center", 0, 15, currentCenterVol},
        {"sub", "Subwoofer", 0, 15, currentSubVol},
        {"front", "Front L/R", 0, 15, currentFrontVol},
        {"rear", "Surround L/R", 0, 15, currentRearVol}
    };
    
    server.sendContent_P(PSTR("<form action='/setVolume' method='POST'>"));
    
    for (const Slider& s : sliders) {
        snprintf_P(mediumBuffer, sizeof(mediumBuffer),
                   PSTR("<label class='form-text'>%s</label>"
                        "<input type='range' id='%s' name='%s' min='%d' max='%d' value='%d' "
                        "oninput='updateVolume(this)'><br>"),
                   s.label, s.id, s.id, s.min, s.max, s.value);
        server.sendContent(mediumBuffer);
    }
    
    server.sendContent_P(PSTR("</form>"));
    server.sendContent_P(HTML_FOOTER);
    server.sendContent("");
}

// =============================================================================
// HANDLERS HTTP (OTIMIZADOS)
// =============================================================================
void handleSetFunc() {
    String func = server.arg("func");
    bool state = server.arg("state").toInt() != 0;
    
    logMessage("WEB", "Função '" + func + "' de " + maskIP(server.client().remoteIP()));
    
    if (func == "audioIn") {
        digitalWrite(AUDIO_IN_PIN, HIGH);
        delay(100);
        digitalWrite(AUDIO_IN_PIN, LOW);
        if (loggingEnabled) Serial.println(F("Audio In pulse sent"));
    } else if (func == "bluetooth") {
        togglePin(BLUETOOTH_PIN, state, loggingEnabled ? (state ? "Bluetooth on" : "Bluetooth off") : "");
        isBluetoothOn = state;
    } else if (func == "system") {
        isSystemOn = state;
        setPowerState(state);
    } else if (func == "mute") {
        isMuteOn = state;
        int currentMute = state ? 1 : 0;
        int currentDdd = isDddOn ? 1 : 0;
        pt.setFunc(currentMute, currentDdd, 0);
        speaker.sendMuteEvent(state);
        if (loggingEnabled) {
            logMessage("INFO", "[WEB] Mute " + String(state ? "ON" : "OFF") + " → SinricPro");
        }
    } else if (func == "ddd") {
        isDddOn = state;
        int currentMute = isMuteOn ? 1 : 0;
        int currentDdd = state ? 1 : 0;
        pt.setFunc(currentMute, currentDdd, 0);
        if (loggingEnabled) {
            logMessage("INFO", "[WEB] DDD " + String(state ? "ON" : "OFF"));
        }
    } else if (func == "audio51") {
        togglePin(AUDIO_51_PIN, true, "");
        delay(100);
        togglePin(AUDIO_51_PIN, false, "");
        if (loggingEnabled) Serial.println(F("5.1 audio pulse sent"));
    }
    
    server.send(200, "text/plain", func + " updated");
}

void handleSetVolume() {
    if (server.hasArg("total")) {
        currentTotalVol = constrain(server.arg("total").toInt(), 0, 79);
        pt.setVol(currentTotalVol);
        updateVolume(currentTotalVol);
        if (loggingEnabled) {
            logMessage("WEB", "Volume " + String(currentTotalVol) + " from " + maskIP(server.client().remoteIP()));
        }
    }
    if (server.hasArg("center")) {
        currentCenterVol = server.arg("center").toInt();
        pt.setCenter_att(15 - currentCenterVol);
        if (loggingEnabled) logMessage("INFO", "Center: " + String(currentCenterVol));
    }
    if (server.hasArg("sub")) {
        currentSubVol = server.arg("sub").toInt();
        pt.setSub_att(15 - currentSubVol);
        if (loggingEnabled) logMessage("INFO", "Sub: " + String(currentSubVol));
    }
    if (server.hasArg("front")) {
        currentFrontVol = server.arg("front").toInt();
        pt.setFront_lk_att(15 - currentFrontVol);
        pt.setFront_rk_att(15 - currentFrontVol);
        if (loggingEnabled) logMessage("INFO", "Front: " + String(currentFrontVol));
    }
    if (server.hasArg("rear")) {
        currentRearVol = server.arg("rear").toInt();
        pt.setRear_lk_att(15 - currentRearVol);
        pt.setRear_rk_att(15 - currentRearVol);
        if (loggingEnabled) logMessage("INFO", "Rear: " + String(currentRearVol));
    }
}

// =============================================================================
// 🎵 TEST CHANNEL HANDLERS (CALIBRATION)
// =============================================================================
void handleTestTone() {
    String channel = server.arg("channel");
    
    testToneActive = true;
    activeTestChannel = channel;
    
    // Silence all channels first (15 = mute)
    pt.setCenter_att(15);
    pt.setSub_att(15);
    pt.setFront_lk_att(15);
    pt.setFront_rk_att(15);
    pt.setRear_lk_att(15);
    pt.setRear_rk_att(15);
    
    // Activate only the test channel (0 = normal volume)
    if (channel == "front_left") {
        pt.setFront_lk_att(15 - currentFrontVol);
    } else if (channel == "front_right") {
        pt.setFront_rk_att(15 - currentFrontVol);
    } else if (channel == "center") {
        pt.setCenter_att(15 - currentCenterVol);
    } else if (channel == "subwoofer") {
        pt.setSub_att(15 - currentSubVol);
    } else if (channel == "rear_left") {
        pt.setRear_lk_att(15 - currentRearVol);
    } else if (channel == "rear_right") {
        pt.setRear_rk_att(15 - currentRearVol);
    } else if (channel == "all") {
        // Restore all channels
        pt.setCenter_att(15 - currentCenterVol);
        pt.setSub_att(15 - currentSubVol);
        pt.setFront_lk_att(15 - currentFrontVol);
        pt.setFront_rk_att(15 - currentFrontVol);
        pt.setRear_lk_att(15 - currentRearVol);
        pt.setRear_rk_att(15 - currentRearVol);
    }
    
    logMessage("TEST", "Testing channel: " + channel);
    server.send(200, "text/plain", "Test started");
}

void handleStopTest() {
    testToneActive = false;
    activeTestChannel = "";
    
    // Restore original volumes
    pt.setVol(currentTotalVol);
    pt.setCenter_att(15 - currentCenterVol);
    pt.setSub_att(15 - currentSubVol);
    pt.setFront_lk_att(15 - currentFrontVol);
    pt.setFront_rk_att(15 - currentFrontVol);
    pt.setRear_lk_att(15 - currentRearVol);
    pt.setRear_rk_att(15 - currentRearVol);
    
    logMessage("TEST", "Teste finalizado");
    server.send(200, "text/plain", "Test stopped");
}

void handleTestPage() {
    logMessage("WEB", "Test page: " + maskIP(server.client().remoteIP()));
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(TEST_TONE_PAGE);
    server.sendContent("");
}

// =============================================================================
// PROCESSAMENTO DE COMANDOS IR
// =============================================================================
void processIRCommand(decode_results *results) {
    if (results->decode_type != UNKNOWN) {
        switch (results->value) {
            case 0xE0E0E01F:
                onSetVolume("", constrain(currentTotalVol + 1, 0, 79));
                break;
            case 0xE0E0D02F:
                onSetVolume("", constrain(currentTotalVol - 1, 0, 79));
                break;
            case 0xE0E0F00F:
                isSystemOn = !isSystemOn;
                onPowerState("", isSystemOn);
                break;
        }
    }
    irrecv.resume();
}

// =============================================================================
// 🌟 SETUP DO OTA (OVER-THE-AIR UPDATES)
// =============================================================================
void setupOTA() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        logMessage("OTA", "🔄 Starting OTA update: " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        logMessage("OTA", "✅ Atualização OTA concluída!");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 1000) {  // Atualiza a cada 1 segundo
            unsigned int percent = (progress / (total / 100));
            logMessage("OTA", "Progresso: " + String(percent) + "%");
            lastPrint = millis();
        }
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        String errorMsg = "Erro OTA [" + String(error) + "]: ";
        if (error == OTA_AUTH_ERROR) errorMsg += "Falha de autenticação";
        else if (error == OTA_BEGIN_ERROR) errorMsg += "Falha ao iniciar";
        else if (error == OTA_CONNECT_ERROR) errorMsg += "Falha de conexão";
        else if (error == OTA_RECEIVE_ERROR) errorMsg += "Falha ao receber";
        else if (error == OTA_END_ERROR) errorMsg += "Falha ao finalizar";
        logMessage("ERROR", errorMsg);
    });
    
    ArduinoOTA.begin();
    logMessage("INFO", "✅ OTA enabled! Password: HomeTheater@2025");
}

// =============================================================================
// SETUP FUNCTIONS
// =============================================================================
void setupMDNS() {
    // Main name
    if (!MDNS.begin(MDNS_NAME)) {
        logMessage("WARNING", "⚠️ mDNS failed (normal on Windows without Bonjour)");
        logMessage("INFO", "💡 Install Bonjour or use IP: http://" + WiFi.localIP().toString());
    } else {
        // Add HTTP service for discovery
        MDNS.addService("http", "tcp", 80);
        logMessage("INFO", "✅ mDNS active:");
        logMessage("INFO", "   • http://" + String(MDNS_NAME) + ".local");
        logMessage("INFO", "   • http://" + String(MDNS_ALIAS) + ".local (alias)");
    }
    
    logMessage("INFO", "💡 Sempre funciona: http://" + WiFi.localIP().toString());
}

void setupServer() {
    server.on("/", handleRoot);
    server.on("/setVolume", handleSetVolume);
    server.on("/setFunc", HTTP_POST, handleSetFunc);
    server.on("/status", handleStatus);
    server.on("/status-page", handleStatusPage);
    server.on("/test-tone.html", handleTestPage);     // 🎵 Test page
    server.on("/testTone", HTTP_POST, handleTestTone); // 🎵 Test API
    server.on("/stopTest", HTTP_POST, handleStopTest); // 🎵 Stop test
    server.begin();
    logMessage("INFO", "✅ HTTP server iniciado na porta 80");
}

void setupPins() {
    // ✅ Output pin configuration (removed duplicate code)
    pinMode(PSON_PIN, OUTPUT);
    pinMode(BLUETOOTH_PIN, OUTPUT);
    pinMode(AUDIO_IN_PIN, OUTPUT);
    pinMode(AUDIO_51_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    
    // Pin initialization
    digitalWrite(PSON_PIN, LOW);
    digitalWrite(BLUETOOTH_PIN, LOW);
    digitalWrite(AUDIO_IN_PIN, LOW);
    digitalWrite(AUDIO_51_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
    
    // IR receiver activation
    irrecv.enableIRIn();
    
    logMessage("INFO", "✅ Pinos configurados e IR ativado");
}

void applySettings() {
    pt.setVol(currentTotalVol);
    pt.setCenter_att(15 - currentCenterVol);
    pt.setSub_att(15 - currentSubVol);
    pt.setFront_lk_att(15 - currentFrontVol);
    pt.setFront_rk_att(15 - currentFrontVol);
    pt.setRear_lk_att(15 - currentRearVol);
    pt.setRear_rk_att(15 - currentRearVol);
    pt.setFunc(0, 1, 0);  // Mute OFF, DDD ON, Temb 0
    pt.setBass(0);
    pt.setMiddle(0);
    pt.setTreble(0);
    logMessage("INFO", "✅ PT2322 configurado");
}

// =============================================================================
// SETUP PRINCIPAL
// =============================================================================
void setup() {
    Serial.begin(115200);  // ✅ Otimizado para ESP8266
    delay(100);
    
    if (loggingEnabled) {
        Serial.println(F("\n\n==========================================="));
        Serial.println(F("  HOME THEATER CONTROL v2.5"));
        Serial.println(F("  OnlyCampe - 2025"));
        Serial.println(F("==========================================="));
    }
    
    logMessage("INFO", F("🌟 Starting system..."));
    
    // WiFi Manager Setup
    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(180);
    
    logMessage("INFO", F("Connecting to WiFi..."));
    if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
        logMessage("ERROR", F("❌ WiFi failed. Rebooting..."));
        delay(3000);
        ESP.restart();
    }
    
    logMessage("INFO", F("✅ WiFi connected!"));
    logMessage("INFO", "IP: " + WiFi.localIP().toString());
    logMessage("INFO", "MAC: " + WiFi.macAddress());
    snprintf_P(smallBuffer, sizeof(smallBuffer), PSTR("Signal: %d dBm"), WiFi.RSSI());
    logMessage("INFO", smallBuffer);
    
    setupMDNS();
    setupServer();
    setupPins();
    setupOTA();
    
    // Callbacks SinricPro
    speaker.onPowerState(onPowerState);
    speaker.onMute(onMute);
    speaker.onSetVolume([](const String &deviceId, int &volume) -> bool {
        return onSetVolume(deviceId, volume);
    });
    
    SinricPro.onConnected([]() { logMessage("INFO", F("✅ [SinricPro] Connected")); });
    SinricPro.onDisconnected([]() { logMessage("WARNING", F("⚠️ [SinricPro] Disconnected")); });
    SinricPro.begin(APP_KEY, APP_SECRET);
    
    applySettings();
    
    if (loggingEnabled) {
        Serial.println(F("\n==========================================="));
        Serial.println(F("  ✅ SYSTEM READY!"));
        Serial.println(F("\n  📱 ACCESS:"));
        Serial.print(F("     http://"));
        Serial.print(MDNS_NAME);
        Serial.println(F(".local"));
        Serial.print(F("     http://"));
        Serial.print(MDNS_ALIAS);
        Serial.println(F(".local"));
        Serial.print(F("     http://"));
        Serial.println(WiFi.localIP());
        Serial.println(F("\n  📊 Status: /status ou /status-page"));
        Serial.println(F("===========================================\n"));
    }
}

// =============================================================================
// ✅ MAIN LOOP (NON-BLOCKING + OTA)
// =============================================================================
void loop() {
    // Watchdog - feed the watchdog to avoid reset
    yield();
    
    ArduinoOTA.handle();    // 🌟 Processa OTA
    yield();
    
    server.handleClient();
    yield();
    
    MDNS.update();
    yield();
    
    SinricPro.handle();
    yield();
    
    // ✅ Process power sequence in non-blocking way
    handlePowerSequence();
    
    // Processa comandos de IR
    if (irrecv.decode(&results)) {
        if (loggingEnabled) {
            logMessage("IR", "Comando recebido: 0x" + String(results.value, HEX));
        }
        processIRCommand(&results);
        irrecv.resume();
    }
    
    // Send volume events at defined intervals
    unsigned long currentTime = millis();
    if (currentTime - lastVolumeUpdateTime >= volumeUpdateInterval) {
        if (lastVolumeSent != -1) {
            speaker.sendVolumeEvent(lastVolumeSent);
            lastVolumeSent = -1;
        }
        lastVolumeUpdateTime = currentTime;
    }
    
    delay(1);
}

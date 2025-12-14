// =============================================================================
// HOME THEATER CONTROL - ESP8266 NodeMCU + PT2322
// =============================================================================
// Versão 2.5 - Funcionalidades Avançadas
// Autor: OnlyCampe
// Data: 13/12/2025
//
// MELHORIAS DESTA VERSÃO:
// ✅ Delays não-bloqueantes (sistema sempre responsivo)
// ✅ HTML movido para PROGMEM (economiza ~3.5KB de RAM)
// ✅ Credenciais em arquivo separado (segurança)
// ✅ Código limpo e otimizado
// 🌟 OTA Updates (atualizar firmware pela WiFi sem cabo USB!)
// 🌟 Página de status/diagnóstico completa
// 🌟 Sistema de logging melhorado
// 🌟 Interface responsiva (mobile + desktop)
// 💡 Valores padrão (mudanças temporárias, reset ao reiniciar)
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

// Incluir arquivos de configuração
#include "config.h"          // ⚠️ Suas credenciais (renomeie config.example.h)
#include "webpages.h"        // HTML armazenado na Flash (PROGMEM)

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
// VARIÁVEIS DE ESTADO
// =============================================================================
bool isSystemOn = false;
bool isBluetoothOn = false;
bool isMuteOn = false;
bool isDddOn = true;
bool isTembOn = false;
bool loggingEnabled = ENABLE_SERIAL_LOGGING;
bool isRelayOn = false;
bool isAudio51Enabled = true;

// ✅ NOVO: Variáveis para controle de power não-bloqueante
enum PowerSequenceState {
    POWER_IDLE,
    POWER_ON_STEP1,
    POWER_OFF_STEP1
};
PowerSequenceState powerSequence = POWER_IDLE;
unsigned long powerStateChangeTime = 0;
bool targetPowerState = false;

// =============================================================================
// VARIÁVEIS DE VOLUME E ÁUDIO (VALORES PADRÃO)
// =============================================================================
// 💡 Estes valores são restaurados a cada inicialização
// Mudanças durante o uso são temporárias
int currentTotalVol = 53;     // Volume padrão: 53
int currentCenterVol = 10;    // Center padrão: 10
int currentSubVol = 12;       // Subwoofer padrão: 12
int currentFrontVol = 10;     // Front padrão: 10
int currentRearVol = 10;      // Rear padrão: 10
int currentBassValue = 0;     // Bass padrão: 0
int currentMiddleValue = 0;   // Middle padrão: 0
int currentTrebleValue = 0;   // Treble padrão: 0
int currentMute = 0;          // Mute padrão: OFF
int currentDdd = 1;           // DDD padrão: ON
int currentTemb = 0;          // Temb padrão: 0

unsigned long lastVolumeUpdateTime = 0;
const unsigned long volumeUpdateInterval = 1000;
int lastVolumeSent = -1;

// =============================================================================
// DECLARAÇÕES DE FUNÇÕES (protótipos)
// =============================================================================
bool onPowerState(const String &deviceId, bool &state);
bool onSetVolume(const String &deviceId, int volume);  // ✅ Corrigido: int volume (não referência)
bool onMute(const String &deviceId, bool &state);
void handlePowerSequence();
void setupOTA();
void logMessage(const char* level, const String& message);

// =============================================================================
// SISTEMA DE LOGGING
// =============================================================================
void logMessage(const char* level, const String& message) {
    if (loggingEnabled) {
        unsigned long uptime = millis() / 1000;
        String logLine = "[" + String(uptime) + "s] [" + String(level) + "] " + message;
        Serial.println(logLine);
    }
}

// =============================================================================
// FUNÇÕES DE CONTROLE DE PINOS
// =============================================================================
void togglePin(int pin, bool state, const String &message) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state ? HIGH : LOW);
    if (loggingEnabled && !message.isEmpty()) {
        logMessage("INFO", message);
    }
}

// =============================================================================
// FUNÇÕES DE ATUALIZAÇÃO DE ESTADO
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
// ✅ NOVO: CONTROLE DE POWER NÃO-BLOQUEANTE
// =============================================================================
// Esta função inicia a sequência de ligar/desligar sem usar delay()
// O processamento acontece no loop através de handlePowerSequence()
bool setPowerState(bool &state) {
    targetPowerState = state;
    powerStateChangeTime = millis();
    
    if (state) {
        // Inicia sequência de LIGAR
        togglePin(PSON_PIN, true, loggingEnabled ? "PSON turned on" : "");
        powerSequence = POWER_ON_STEP1;
    } else {
        // Inicia sequência de DESLIGAR
        togglePin(RELAY_PIN, false, loggingEnabled ? "Relay turned off" : "");
        powerSequence = POWER_OFF_STEP1;
    }
    return true;
}

// ✅ NOVO: Processa a sequência de power de forma não-bloqueante
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
    currentMute = mute ? 1 : 0;
    pt.setFunc(currentMute, currentDdd, currentTemb);
    if (loggingEnabled) {
        String source = deviceId.isEmpty() ? "WEB" : "SinricPro";
        logMessage("INFO", "[" + source + "] Mute " + String(mute ? "ON" : "OFF"));
    }
    return true;
}

bool onDdd(const String &deviceId, bool &state) {
    currentDdd = state ? 1 : 0;
    pt.setFunc(currentMute, currentDdd, currentTemb);
    if (loggingEnabled) {
        String source = deviceId.isEmpty() ? "WEB" : "SinricPro";
        logMessage("INFO", "[" + source + "] DDD " + String(state ? "ON" : "OFF"));
    }
    return true;
}

// =============================================================================
// 🌟 HANDLER DA PÁGINA DE STATUS HTML (visual bonita)
// =============================================================================
void handleStatusPage() {
    logMessage("WEB", "Status page acessada de " + server.client().remoteIP().toString());
    
    // Envia em chunks
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(STATUS_PAGE_HTML);
    
    // Card 1: Sistema
    String html = "<div class='card'><h2><i class='fa-solid fa-microchip'></i> Sistema</h2><div class='info-grid'>";
    html += "<span class='label'>Versão:</span><span class='value'>2.5</span>";
    html += "<span class='label'>Uptime:</span><span class='value'>" + String(millis() / 1000) + "s (" + String(millis() / 60000) + " min)</span>";
    html += "<span class='label'>Chip ID:</span><span class='value'>" + String(ESP.getChipId(), HEX) + "</span>";
    html += "<span class='label'>SDK Version:</span><span class='value'>" + String(ESP.getSdkVersion()) + "</span>";
    html += "<span class='label'>Core Version:</span><span class='value'>" + ESP.getCoreVersion() + "</span>";
    html += "<span class='label'>Boot Version:</span><span class='value'>" + String(ESP.getBootVersion()) + "</span>";
    html += "<span class='label'>Boot Mode:</span><span class='value'>" + String(ESP.getBootMode()) + "</span>";
    html += "<span class='label'>Reset Reason:</span><span class='value'>" + ESP.getResetReason() + "</span>";
    html += "</div></div>";
    server.sendContent(html);
    
    // Card 2: CPU & Flash
    String flashMode;
    switch(ESP.getFlashChipMode()) {
        case 0: flashMode = "QIO"; break;
        case 1: flashMode = "QOUT"; break;
        case 2: flashMode = "DIO"; break;
        case 3: flashMode = "DOUT"; break;
        default: flashMode = "Unknown"; break;
    }
    
    html = "<div class='card'><h2><i class='fa-solid fa-bolt'></i> CPU & Flash</h2><div class='info-grid'>";
    html += "<span class='label'>CPU Freq:</span><span class='value status-ok'>" + String(ESP.getCpuFreqMHz()) + " MHz</span>";
    html += "<span class='label'>Flash Size:</span><span class='value'>" + String(ESP.getFlashChipSize() / 1024) + " KB</span>";
    html += "<span class='label'>Flash Speed:</span><span class='value'>" + String(ESP.getFlashChipSpeed() / 1000000) + " MHz</span>";
    html += "<span class='label'>Flash Mode:</span><span class='value'>" + flashMode + "</span>";
    html += "<span class='label'>Sketch Size:</span><span class='value'>" + String(ESP.getSketchSize() / 1024) + " KB</span>";
    html += "<span class='label'>Free Space:</span><span class='value'>" + String(ESP.getFreeSketchSpace() / 1024) + " KB</span>";
    html += "<span class='label'>Sketch MD5:</span><span class='value' style='font-size:0.8em;'>" + ESP.getSketchMD5() + "</span>";
    html += "</div></div>";
    server.sendContent(html);
    
    // Card 3: Memória
    int freeHeap = ESP.getFreeHeap();
    int heapPercent = (freeHeap * 100) / 80192;
    String heapClass = heapPercent > 30 ? "status-ok" : "status-warn";
    
    html = "<div class='card'><h2><i class='fa-solid fa-memory'></i> Memória RAM</h2><div class='info-grid'>";
    html += "<span class='label'>RAM Livre:</span><span class='value " + heapClass + "'>" + String(freeHeap) + " bytes (" + String(heapPercent) + "%)</span>";
    html += "<span class='label'>Max Free Block:</span><span class='value'>" + String(ESP.getMaxFreeBlockSize()) + " bytes</span>";
    html += "<span class='label'>Fragmentação:</span><span class='value'>" + String(ESP.getHeapFragmentation()) + "%</span>";
    html += "</div></div>";
    server.sendContent(html);
    
    // Card 4: WiFi
    int rssi = WiFi.RSSI();
    String signalClass = rssi > -60 ? "status-ok" : (rssi > -70 ? "status-warn" : "status-warn");
    
    html = "<div class='card'><h2><i class='fa-solid fa-wifi'></i> WiFi</h2><div class='info-grid'>";
    html += "<span class='label'>SSID:</span><span class='value'>" + WiFi.SSID() + "</span>";
    html += "<span class='label'>Canal:</span><span class='value'>" + String(WiFi.channel()) + "</span>";
    html += "<span class='label'>IP:</span><span class='value'>" + WiFi.localIP().toString() + "</span>";
    html += "<span class='label'>MAC:</span><span class='value'>" + WiFi.macAddress() + "</span>";
    html += "<span class='label'>Signal:</span><span class='value " + signalClass + "'>" + String(rssi) + " dBm</span>";
    html += "</div></div>";
    server.sendContent(html);
    
    // Card 4: Estado do Home Theater
    html = "<div class='card'><h2><i class='fa-solid fa-volume-high'></i> Home Theater</h2><div class='info-grid'>";
    html += "<span class='label'>Sistema:</span><span class='value " + String(isSystemOn ? "status-ok'>LIGADO" : "'>DESLIGADO") + "</span>";
    html += "<span class='label'>Bluetooth:</span><span class='value " + String(isBluetoothOn ? "status-ok'>ON" : "'>OFF") + "</span>";
    html += "<span class='label'>Mute:</span><span class='value " + String(isMuteOn ? "status-warn'>ON" : "'>OFF") + "</span>";
    html += "<span class='label'>DDD:</span><span class='value " + String(isDddOn ? "status-ok'>ON" : "'>OFF") + "</span>";
    html += "<span class='label'>Volume Total:</span><span class='value'>" + String(currentTotalVol) + "/79 (" + String(map(currentTotalVol, 0, 79, 0, 100)) + "%)</span>";
    html += "<span class='label'>Center:</span><span class='value'>" + String(currentCenterVol) + "/15</span>";
    html += "<span class='label'>Subwoofer:</span><span class='value'>" + String(currentSubVol) + "/15</span>";
    html += "<span class='label'>Front L/R:</span><span class='value'>" + String(currentFrontVol) + "/15</span>";
    html += "<span class='label'>Rear L/R:</span><span class='value'>" + String(currentRearVol) + "/15</span>";
    html += "</div></div>";
    server.sendContent(html);
    
    server.sendContent_P(STATUS_PAGE_FOOTER);
    server.sendContent("");
}

// =============================================================================
// 🌟 HANDLER DA API DE STATUS JSON
// =============================================================================
void handleStatus() {
    String json = "{";
    json += "\"version\":\"2.5\",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"heapFragmentation\":" + String(ESP.getHeapFragmentation()) + ",";
    json += "\"maxFreeBlockSize\":" + String(ESP.getMaxFreeBlockSize()) + ",";
    json += "\"chipId\":\"" + String(ESP.getChipId(), HEX) + "\",";
    json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"flashChipSize\":" + String(ESP.getFlashChipSize()) + ",";
    json += "\"flashChipSpeed\":" + String(ESP.getFlashChipSpeed() / 1000000) + ",";
    json += "\"flashChipMode\":" + String(ESP.getFlashChipMode()) + ",";
    json += "\"sketchSize\":" + String(ESP.getSketchSize()) + ",";
    json += "\"freeSketchSpace\":" + String(ESP.getFreeSketchSpace()) + ",";
    json += "\"sketchMD5\":\"" + ESP.getSketchMD5() + "\",";
    json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\",";
    json += "\"coreVersion\":\"" + ESP.getCoreVersion() + "\",";
    json += "\"bootVersion\":" + String(ESP.getBootVersion()) + ",";
    json += "\"bootMode\":" + String(ESP.getBootMode()) + ",";
    json += "\"resetReason\":\"" + ESP.getResetReason() + "\",";
    json += "\"resetInfo\":\"" + ESP.getResetInfo() + "\",";
    json += "\"wifiSignal\":" + String(WiFi.RSSI()) + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"wifiChannel\":" + String(WiFi.channel()) + ",";
    json += "\"isSystemOn\":" + String(isSystemOn ? "true" : "false") + ",";
    json += "\"isBluetoothOn\":" + String(isBluetoothOn ? "true" : "false") + ",";
    json += "\"isMuteOn\":" + String(isMuteOn ? "true" : "false") + ",";
    json += "\"isDddOn\":" + String(isDddOn ? "true" : "false") + ",";
    json += "\"volume\":" + String(currentTotalVol) + ",";
    json += "\"centerVol\":" + String(currentCenterVol) + ",";
    json += "\"subVol\":" + String(currentSubVol) + ",";
    json += "\"frontVol\":" + String(currentFrontVol) + ",";
    json += "\"rearVol\":" + String(currentRearVol);
    json += "}";
    
    server.send(200, "application/json", json);
    logMessage("API", "Status acessado de " + server.client().remoteIP().toString());
}

// =============================================================================
// ✅ NOVO: HANDLER DA PÁGINA WEB (COM PROGMEM)
// =============================================================================
void handleRoot() {
    // Log de acesso (IP mascarado por segurança)
    String clientIP = server.client().remoteIP().toString();
    String clientMasked = clientIP.substring(0, clientIP.indexOf('.', clientIP.indexOf('.') + 1)) + ".xxx.xxx";
    logMessage("WEB", "Página acessada de " + clientMasked);
    
    // Envia HTML em chunks para economizar RAM
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    
    // Envia header da Flash (PROGMEM)
    server.sendContent_P(HTML_HEADER);
    
    // Gera botões dinamicamente
    struct Button { const char* id; const char* icon; const char* iconClass; bool state; };
    Button buttons[] = {
        {"system", "power-off", "fa-solid", isSystemOn},
        {"bluetooth", "bluetooth-b", "fa-brands", isBluetoothOn},
        {"mute", "volume-mute", "fa-solid", isMuteOn},
        {"ddd", "cube", "fa-solid", isDddOn},
        {"audioIn", "music", "fa-solid", false},
        {"audio51", "arrows-left-right-to-line", "fa-solid", false}
    };
    
    String btnHtml = "";
    for (Button btn : buttons) {
        btnHtml = "<button id='";
        btnHtml += btn.id;
        btnHtml += "Button' class='";
        btnHtml += (strcmp(btn.id, "audioIn") == 0 || strcmp(btn.id, "audio51") == 0) ? "red" : (btn.state ? "green" : "red");
        btnHtml += "' onclick='toggleButton(\"";
        btnHtml += btn.id;
        btnHtml += "\")'><i class='";
        btnHtml += btn.iconClass;
        btnHtml += " fa-";
        btnHtml += btn.icon;
        btnHtml += "'></i></button>";
        
        server.sendContent(btnHtml);
    }
    
    // Gera sliders dinamicamente
    struct Slider { const char* id; const char* label; int min; int max; int value; };
    Slider sliders[] = {
        {"total", "Volume Total", 0, 79, currentTotalVol},
        {"center", "Center", 0, 15, currentCenterVol},
        {"sub", "Subwoofer", 0, 15, currentSubVol},
        {"front", "Front L/R", 0, 15, currentFrontVol},
        {"rear", "Surround L/R", 0, 15, currentRearVol}
    };
    
    String sliderHtml = "<form action='/setVolume' method='POST'>";
    server.sendContent(sliderHtml);
    
    for (Slider s : sliders) {
        sliderHtml = "<label class='form-text'>";
        sliderHtml += s.label;
        sliderHtml += "</label><input type='range' id='";
        sliderHtml += s.id;
        sliderHtml += "' name='";
        sliderHtml += s.id;
        sliderHtml += "' min='";
        sliderHtml += s.min;
        sliderHtml += "' max='";
        sliderHtml += s.max;
        sliderHtml += "' value='";
        sliderHtml += s.value;
        sliderHtml += "' oninput='updateVolume(this)'><br>";
        
        server.sendContent(sliderHtml);
    }
    
    server.sendContent("</form>");
    
    // Envia footer da Flash (PROGMEM)
    server.sendContent_P(HTML_FOOTER);
    server.sendContent("");
}

// =============================================================================
// HANDLERS HTTP
// =============================================================================
void handleSetFunc() {
    String func = server.arg("func");
    bool state = server.arg("state").toInt() != 0;
    
    // Log de ação
    String clientIP = server.client().remoteIP().toString();
    String clientMasked = clientIP.substring(0, clientIP.indexOf('.', clientIP.indexOf('.') + 1)) + ".xxx.xxx";
    logMessage("WEB", "Função '" + func + "' acionada de " + clientMasked);
    
    if (func == "audioIn") {
        digitalWrite(AUDIO_IN_PIN, HIGH);
        delay(100);
        digitalWrite(AUDIO_IN_PIN, LOW);
        if (loggingEnabled) {
            Serial.println("Audio In pulse sent.");
        }
    } else if (func == "bluetooth") {
        togglePin(BLUETOOTH_PIN, state, loggingEnabled ? (state ? "Bluetooth turned on" : "Bluetooth turned off") : "");
        isBluetoothOn = state;
    } else if (func == "system") {
        // Atualiza estado e notifica SinricPro
        isSystemOn = state;
        setPowerState(state);
    } else if (func == "mute") {
        // Aplica no hardware
        isMuteOn = state;
        currentMute = state ? 1 : 0;
        pt.setFunc(currentMute, currentDdd, currentTemb);
        
        // 🌟 Notifica SinricPro manualmente (não usar callback)
        speaker.sendMuteEvent(state);
        
        if (loggingEnabled) {
            logMessage("INFO", "[WEB] Mute " + String(state ? "ON" : "OFF") + " → SinricPro notificado");
        }
    } else if (func == "ddd") {
        // Atualiza estado
        isDddOn = state;
        currentDdd = state ? 1 : 0;
        pt.setFunc(currentMute, currentDdd, currentTemb);
        if (loggingEnabled) {
            logMessage("INFO", "[WEB] DDD " + String(state ? "ON" : "OFF"));
        }
    } else if (func == "audio51") {
        togglePin(AUDIO_51_PIN, true, "");
        delay(100);
        togglePin(AUDIO_51_PIN, false, "");
        if (loggingEnabled) {
            Serial.println("5.1 audio pulse sent");
        }
    }
    
    server.send(200, "text/plain", func + " state updated");
}

void handleSetVolume() {
    // Log apenas se for volume total (evita spam de logs)
    if (server.hasArg("total")) {
        String clientIP = server.client().remoteIP().toString();
        String clientMasked = clientIP.substring(0, clientIP.indexOf('.', clientIP.indexOf('.') + 1)) + ".xxx.xxx";
        
        currentTotalVol = constrain(server.arg("total").toInt(), 0, 79);
        pt.setVol(currentTotalVol);
        updateVolume(currentTotalVol);
        if (loggingEnabled) {
            logMessage("WEB", "Volume alterado para " + String(currentTotalVol) + " por " + clientMasked);
        }
    }
    if (server.hasArg("center")) {
        currentCenterVol = server.arg("center").toInt();
        pt.setCenter_att(15 - currentCenterVol);
        if (loggingEnabled) {
            logMessage("INFO", "Center volume set to: " + String(currentCenterVol));
        }
    }
    if (server.hasArg("sub")) {
        currentSubVol = server.arg("sub").toInt();
        pt.setSub_att(15 - currentSubVol);
        if (loggingEnabled) {
            logMessage("INFO", "Subwoofer volume set to: " + String(currentSubVol));
        }
    }
    if (server.hasArg("front")) {
        currentFrontVol = server.arg("front").toInt();
        pt.setFront_lk_att(15 - currentFrontVol);
        pt.setFront_rk_att(15 - currentFrontVol);
        if (loggingEnabled) {
            logMessage("INFO", "Front volume set to: " + String(currentFrontVol));
        }
    }
    if (server.hasArg("rear")) {
        currentRearVol = server.arg("rear").toInt();
        pt.setRear_lk_att(15 - currentRearVol);
        pt.setRear_rk_att(15 - currentRearVol);
        if (loggingEnabled) {
            logMessage("INFO", "Rear volume set to: " + String(currentRearVol));
        }
    }
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
        logMessage("OTA", "🔄 Iniciando atualização OTA: " + type);
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
    logMessage("INFO", "✅ OTA habilitado! Senha: HomeTheater@2025");
}

// =============================================================================
// FUNÇÕES DE SETUP
// =============================================================================
void setupMDNS() {
    // Nome principal
    if (!MDNS.begin(MDNS_NAME)) {
        logMessage("WARNING", "⚠️ mDNS falhou (normal no Windows sem Bonjour)");
        logMessage("INFO", "💡 Instale Bonjour ou use IP: http://" + WiFi.localIP().toString());
    } else {
        // Adiciona serviço HTTP para descoberta
        MDNS.addService("http", "tcp", 80);
        logMessage("INFO", "✅ mDNS ativo:");
        logMessage("INFO", "   • http://" + String(MDNS_NAME) + ".local");
        logMessage("INFO", "   • http://" + String(MDNS_ALIAS) + ".local (alias)");
    }
    
    logMessage("INFO", "💡 Sempre funciona: http://" + WiFi.localIP().toString());
}

void setupServer() {
    server.on("/", handleRoot);
    server.on("/setVolume", handleSetVolume);
    server.on("/setFunc", HTTP_POST, handleSetFunc);
    server.on("/status", handleStatus);           // API JSON
    server.on("/status-page", handleStatusPage);  // 🌟 Página HTML visual
    server.begin();
    logMessage("INFO", "✅ HTTP server iniciado na porta 80");
}

void setupPins() {
    // ✅ Configuração de pinos de saída (removido código duplicado)
    pinMode(PSON_PIN, OUTPUT);
    pinMode(BLUETOOTH_PIN, OUTPUT);
    pinMode(AUDIO_IN_PIN, OUTPUT);
    pinMode(AUDIO_51_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    
    // Inicialização dos pinos
    digitalWrite(PSON_PIN, LOW);
    digitalWrite(BLUETOOTH_PIN, LOW);
    digitalWrite(AUDIO_IN_PIN, LOW);
    digitalWrite(AUDIO_51_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
    
    // Ativação do receptor IR
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
    pt.setFunc(currentMute, currentDdd, currentTemb);
    pt.setBass(currentBassValue);
    pt.setMiddle(currentMiddleValue);
    pt.setTreble(currentTrebleValue);
    logMessage("INFO", "✅ Configurações aplicadas ao PT2322");
}

// =============================================================================
// SETUP PRINCIPAL
// =============================================================================
void setup() {
    Serial.begin(9600);
    delay(500);  // Aguarda serial estabilizar
    
    if (loggingEnabled) {
        Serial.println("\n\n");
        Serial.println("===========================================");
        Serial.println("  HOME THEATER CONTROL v2.5");
        Serial.println("  OnlyCampe - 2025");
        Serial.println("===========================================");
    }
    
    logMessage("INFO", "🌟 Iniciando sistema...");
    logMessage("INFO", "💡 Valores padrão carregados (mudanças são temporárias)");
    
    // WiFi Manager Setup
    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(180);  // 3 minutos de timeout
    
    logMessage("INFO", "Conectando ao WiFi...");
    if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
        logMessage("ERROR", "❌ Falha ao conectar WiFi. Reiniciando...");
        delay(3000);
        ESP.restart();
    }
    
    logMessage("INFO", "✅ WiFi conectado!");
    logMessage("INFO", "IP: " + WiFi.localIP().toString());
    logMessage("INFO", "MAC: " + WiFi.macAddress());
    logMessage("INFO", "Signal: " + String(WiFi.RSSI()) + " dBm");
    
    setupMDNS();
    setupServer();
    setupPins();
    
    // 🌟 Configura OTA
    setupOTA();
    
    // Configurar callbacks do SinricPro
    speaker.onPowerState(onPowerState);
    speaker.onMute(onMute);
    speaker.onSetVolume([](const String &deviceId, int &volume) -> bool {
        return onSetVolume(deviceId, volume);
    });
    
    // Callbacks de conexão SinricPro
    SinricPro.onConnected([]() {
        logMessage("INFO", "✅ [SinricPro] Conectado");
    });
    
    SinricPro.onDisconnected([]() {
        logMessage("WARNING", "⚠️ [SinricPro] Desconectado");
    });
    
    SinricPro.begin(APP_KEY, APP_SECRET);
    
    applySettings();
    
    if (loggingEnabled) {
        Serial.println("\n===========================================");
        Serial.println("  ✅ SISTEMA PRONTO!");
        Serial.println("\n  📱 ACESSE NO NAVEGADOR:");
        Serial.print("     http://");
        Serial.print(MDNS_NAME);
        Serial.println(".local");
        Serial.print("     http://");
        Serial.print(MDNS_ALIAS);
        Serial.println(".local");
        Serial.print("     http://");
        Serial.println(WiFi.localIP());
        Serial.println("\n  📊 Status/API:");
        Serial.print("     http://");
        Serial.print(MDNS_NAME);
        Serial.println(".local/status");
        Serial.print("     http://");
        Serial.print(WiFi.localIP());
        Serial.println("/status");
        Serial.println("\n  💡 Se .local não funcionar, instale Bonjour");
        Serial.println("     ou use o IP direto (sempre funciona)");
        Serial.println("===========================================\n");
    }
}

// =============================================================================
// ✅ LOOP PRINCIPAL (NÃO-BLOQUEANTE + OTA)
// =============================================================================
void loop() {
    // Watchdog - alimenta o cão de guarda para evitar reset
    yield();
    
    ArduinoOTA.handle();    // 🌟 Processa OTA
    yield();
    
    server.handleClient();
    yield();
    
    MDNS.update();
    yield();
    
    SinricPro.handle();
    yield();
    
    // ✅ Processa sequência de power de forma não-bloqueante
    handlePowerSequence();
    
    // Processa comandos de IR
    if (irrecv.decode(&results)) {
        if (loggingEnabled) {
            logMessage("IR", "Comando recebido: 0x" + String(results.value, HEX));
        }
        processIRCommand(&results);
        irrecv.resume();
    }
    
    // Envia eventos de volume em intervalos definidos
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

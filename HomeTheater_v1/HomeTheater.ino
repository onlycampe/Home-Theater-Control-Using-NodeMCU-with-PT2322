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
#include <WiFiManager.h>  // Include the WiFiManager library
//
#define RECV_PIN D4          // Pin where the IR receiver is connected
#define PSON_PIN D8          // Power on/off pin for the Speaker
#define BLUETOOTH_PIN D6     // Bluetooth on/off pin (controls a BC547 relay)
#define AUDIO_IN_PIN D5      // Audio In control pin (selects audio inputs on a HD Audio Rush decoder, simulating a physical button press)
#define AUDIO_51_PIN D7  // 5.1/2.1 control pin (simulating a physical button press)
#define RELAY_PIN D3         // Control pin for relays using BC547
// Device settings - ⚠️ CREDENTIALS REMOVED FOR SECURITY
// Get your credentials at: https://sinric.pro/
#define SPEAKER_DEVICE_ID       "your_speaker_device_id_here"
#define SWITCH_DEVICE_ID        "your_switch_device_id_here"
#define APP_KEY                 "your_app_key_here"
#define APP_SECRET              "your_app_secret_here"
//
WiFiUDP udp; // Criar objeto WiFiUDP
IRrecv irrecv(RECV_PIN);      // Creates an instance of the IRrecv class for handling IR signals, using the pin defined by RECV_PIN
decode_results results;       // Variable to store the results of decoded IR signals
SinricProSpeaker& speaker = SinricPro[SPEAKER_DEVICE_ID];
SinricProSwitch& switchDevice = SinricPro[SWITCH_DEVICE_ID];
// Variáveis de controle
bool isSystemOn = false;
bool isBluetoothOn = false;
bool isMuteOn = false;
bool isDddOn = true;  
bool isTembOn = false;
bool loggingEnabled = true;
bool isRelayOn = false;            // Estado do relé do pino D3 começa desligado
bool isAudio51Enabled = true;     // Estado do controle 5.1 começa desligado
bool waitingForRelay = false;
bool relayRequestedState = false;
//callbacks
bool onPowerState(const String &deviceId, bool &state);
bool onSetVolume(const String &deviceId, int &volume, bool state);
bool onMute(const String &deviceId, bool &state);
//
unsigned long lastVolumeUpdateTime = 0;
const unsigned long volumeUpdateInterval = 1000; // Intervalo em milissegundos para enviar o evento após a última alteração
int lastVolumeSent = -1; // Armazena o último volume enviado
//
ESP8266WebServer server(80); // Defines the web server on port 80
PT2322 pt;                   // Creates an instance of the PT2322 class for audio control
// Global variables to store volume values
int currentTotalVol = 53;   // Initial value for Total Volume
int currentCenterVol = 10;  // Initial value for Center Volume
int currentSubVol = 12;     // Initial value for Subwoofer Volume
int currentFrontVol = 10;   // Initial value for Front Volume (Left + Right)
int currentRearVol = 10;    // Initial value for Rear Volume (Left + Right)
// Global variables to store audio adjustment values
int currentBassValue = 0;     // Initial value for Bass adjustment
int currentMiddleValue = 0;   // Initial value for Midrange adjustment
int currentTrebleValue = 0;   // Initial value for Treble adjustment
int currentMute = 0;          // Initial mute state
int currentDdd = 1;           // Initial DDD setting
int currentTemb = 0;          // Initial Temb setting

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><title>Control Panel</title>";
  html += "<meta name='theme-color' content='#222222'>";  // Altera a cor da barra de pesquisa do Chrome
  html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.7.2/css/all.min.css'>";
  html += "<style>@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap');";
  html += "body{font-family:'Roboto',sans-serif;margin:0;padding:0;background:#222;color:#fff;width:100vw;display:flex;justify-content:center;align-items:center;}";
  html += "* { user-select: none; }";
  html += ".container{width:100%;text-align:center;flex-direction:column;justify-content:flex-start;align-items:center;min-height:100vh;padding:90px 0;font-size:50px;}";
  html += "h1{font-size:1em;margin:20px 0;}p{font-size:0.7em;}";
  html += ".button-container{width:100%;display:flex;flex-wrap:wrap;justify-content:center;margin:90px 0;}";
  html += "button{font-size:1.5em;margin:10px;padding:15px;cursor:pointer;border-radius:50%;color:#fff;border:none;width:25vw;height:25vw;max-width:550px;max-height:550px;box-shadow:0 4px 6px rgba(0,0,0,0.3);user-select: none;}";
  html += ".red{background:#4c4f51;}.green{background:#2485c9;}";
  html += "@keyframes blink { 0%, 100% { background-color: #4c4f51; } 50% { background-color: #2485c9; } }";
  html += ".pulse { animation: blink 0.3s ease-in-out; }";
  html += "input[type=range]{-webkit-appearance:none;width:90%;height:30px;background:#4c4f51;border-radius:10px;cursor:pointer;margin-bottom:50px;margin-top:30px;}";
  html += "input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:40px;height:40px;border-radius:50%;background:#2485c9;cursor:pointer;}";
  html += "form{width:100%;flex-direction:column;align-items:center;justify-content:center;margin-top:5rem;}";
  html += "@media (max-width:768px){h1{font-size:2.5em;}button{font-size:1.2em;width:22vw;height:22vw;}}";
  html += "</style></head><body><div class='container'><h1>Home Theater Control</h1><div class='button-container'>";
  
  struct Button { String id, icon, iconClass; bool state; };
  Button buttons[] = {
    {"system", "power-off", "fa-solid", isSystemOn},
    {"bluetooth", "bluetooth-b", "fa-brands", isBluetoothOn},
    {"mute", "volume-mute", "fa-solid", isMuteOn},
    {"ddd", "cube", "fa-solid", isDddOn},
    {"audioIn", "music", "fa-solid", false},
    {"audio51", "arrows-left-right-to-line", "fa-solid", false}  // Novo ícone para o botão 2.0/5.1
  };

  for (Button btn : buttons) {
    html += "<button id='" + btn.id + "Button' class='" + 
            (btn.id == "audioIn" || btn.id == "audio51" ? "red" : (btn.state ? "green" : "red")) + 
            "' onclick='toggleButton(\"" + btn.id + "\")'>";

    // Aplica o 'fa-brands' apenas para o botão Bluetooth
    if (btn.id == "bluetooth") {
      html += "<i class='" + btn.iconClass + " fa-" + btn.icon + " fa-1x'></i>"; // Ícone para o Bluetooth
    } else {
      html += "<i class='" + btn.iconClass + " fa-" + btn.icon + " fa-1x'></i>"; // Ícones para os outros botões
    }

    html += "</button>";
  }
  
  struct Slider { String id, label; int min, max, value; } sliders[] = {
    {"total", "Volume Total", 0, 79, currentTotalVol},
    {"center", "Center", 0, 15, currentCenterVol},
    {"sub", "Subwoofer", 0, 15, currentSubVol},
    {"front", "Front L/R", 0, 15, currentFrontVol},
    {"rear", "Surround L/R", 0, 15, currentRearVol}
  };
  html += "<form action='/setVolume' method='POST'>";
  for (Slider s : sliders) {
    html += "<label class='form-text'>" + s.label + "</label><input type='range' id='" + s.id + "' name='" + s.id + "' min='" + s.min + "' max='" + s.max + "' value='" + s.value + "' oninput='updateVolume(this)'><br>";
  }
  html += "</form><p><a href='https://github.com/onlycampe' target='_blank' style='color:#4c4f51;text-decoration:none;'><i class='fa-brands fa-github-alt fa-1x'></i> OnlyCampe</a></p></div></div>";
  html += "<script>function updateVolume(el){fetch('/setVolume',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`${el.name}=${el.value}`});}";
  html += "function toggleButton(id){let btn=document.getElementById(id+'Button');";
  html += "if(id==='audioIn' || id==='audio51'){";
  html += "  btn.classList.add('pulse');";  // Adiciona a animação de pulsação
  html += "  setTimeout(()=>btn.classList.remove('pulse'),300);";  // Remove a pulsação após o efeito
  html += "  fetch('/setFunc',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`func=${id}&state=1`});";  // Envia o estado do áudio
  html += "}else{";
  html += "  fetch('/setFunc',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`func=${id}&state=${btn.classList.contains('green')?'0':'1'}`}).then(()=>{btn.classList.toggle('red');btn.classList.toggle('green');});";
  html += "}}";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}


void togglePin(int pin, bool state, const String &message) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, state ? HIGH : LOW);
  if (loggingEnabled && !message.isEmpty()) {
    Serial.println(message);
  }
}

bool setAudioFunction(bool &currentState, bool state, void (*setFunc)(int, int, int), int &param1, int &param2) {
  currentState = state ? 1 : 0;
  setFunc(currentMute, currentDdd, currentTemb);
  return true;
}
void updatePowerState(bool state) {
  speaker.sendPowerStateEvent(state);
}
void updateVolume(int volume) {
  unsigned long currentTime = millis();
  lastVolumeSent = volume;
  lastVolumeUpdateTime = currentTime;
}
bool setPowerState(bool &state) {
    if (state) {  // Se o estado atual é desligado (início ou quando desligado)
        // Liga o PSON e o RELAY
        togglePin(PSON_PIN, true, loggingEnabled ? "PSON turned on" : "");
        delay(2000);  // Aguarda para garantir que o PSON ligue primeiro
        togglePin(RELAY_PIN, true, loggingEnabled ? "Relay turned on" : "");
        
        // Atualiza o estado para indicar que agora está ligado
        state = true;
    } else {  // Se o estado atual é ligado
        // Desliga o RELAY e o PSON
        togglePin(RELAY_PIN, false, loggingEnabled ? "Relay turned off" : "");
        delay(2000);  // Aguarda para garantir que o RELAY desligue primeiro
        togglePin(PSON_PIN, false, loggingEnabled ? "PSON turned off" : "");
        
        // Atualiza o estado para indicar que agora está desligado
        state = false;
    }

    // Atualiza a variável global e chama a função de atualização do estado
    isSystemOn = state;
    updatePowerState(state);
    return true;
}


bool onPowerState(const String &deviceId, bool &state) {
    return setPowerState(state);
}
bool onSetVolume(const String &deviceId, int volume) {
  currentTotalVol = constrain(volume, 0, 79);
  pt.setVol(currentTotalVol);
  updateVolume(currentTotalVol);
  if (loggingEnabled) {
    Serial.printf("[Device: %s]: Volume set to %d\r\n", deviceId.c_str(), currentTotalVol);
  }
  return true;
}
bool onMute(const String &deviceId, bool &mute) {
  isMuteOn = mute;
  currentMute = mute ? 1 : 0;
  pt.setFunc(currentMute, currentDdd, currentTemb);  
  if (loggingEnabled) {
    Serial.printf("[Device: %s]: Mute state changed to %s\r\n", deviceId.c_str(), mute ? "on" : "off");
  }
  return true;
}
bool onDdd(const String &deviceId, bool &state) {
  currentDdd = state ? 1 : 0;
  pt.setFunc(currentMute, currentDdd, currentTemb);
  if (loggingEnabled) {
    Serial.println(state ? "DDD activated" : "DDD deactivated");
  }
  return true;
}
void handleSetFunc() {
  String func = server.arg("func");
  bool state = server.arg("state").toInt() != 0;
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
    setPowerState(state); // No return
  } else if (func == "mute") {
    onMute("", state);
    isMuteOn = state;
  } else if (func == "ddd") {
    onDdd("", state);
    isDddOn = state;
} else if (func == "audio51") {
    // Envia um pulso rápido no pino AUDIO_51_PIN
    togglePin(AUDIO_51_PIN, true, ""); // Ativa o pino sem log
    delay(100);  // Define a duração do pulso em milissegundos (ajuste conforme necessário)
    togglePin(AUDIO_51_PIN, false, ""); // Desativa o pino sem log
    // Registra um único log no final do pulso
    if (loggingEnabled) {
        Serial.println("5.1 audio pulse sent");
    }
  } else {
    updateAudioFunctions(func, state);
  }
  
  server.send(200, "text/plain", func + " state updated");
}
void updateAudioFunctions(const String &func, bool state) {
  if (func == "mute") {
    currentMute = state ? 1 : 0;
    pt.setFunc(currentMute, currentDdd, currentTemb);
  } else if (func == "ddd") {
    currentDdd = state ? 1 : 0;
    pt.setFunc(currentMute, currentDdd, currentTemb);
  }
  if (loggingEnabled) {
    Serial.printf("Function %s set to %s\n", func.c_str(), state ? "on" : "off");
  }
}
void handleSetAudioSettings() {
  if (server.hasArg("bass")) {
    currentBassValue = server.arg("bass").toInt();
    pt.setBass(currentBassValue);
    if (loggingEnabled) {
      Serial.println("Bass set to: " + String(currentBassValue));
    }
  }
  if (server.hasArg("middle")) {
    currentMiddleValue = server.arg("middle").toInt();
    pt.setMiddle(currentMiddleValue);
    if (loggingEnabled) {
      Serial.println("Middle set to: " + String(currentMiddleValue));
    }
  }
  if (server.hasArg("treble")) {
    currentTrebleValue = server.arg("treble").toInt();
    pt.setTreble(currentTrebleValue);
    if (loggingEnabled) {
      Serial.println("Treble set to: " + String(currentTrebleValue));
    }
  }
}
void handleSetVolume() {
  if (server.hasArg("total")) {
    currentTotalVol = constrain(server.arg("total").toInt(), 0, 79);
    pt.setVol(currentTotalVol);
    updateVolume(currentTotalVol);

    if (loggingEnabled) {
      Serial.print("Master volume set to: ");
      Serial.println(currentTotalVol);
    }
  }
  if (server.hasArg("center")) {
    currentCenterVol = server.arg("center").toInt();
    pt.setCenter_att(15 - currentCenterVol);
    if (loggingEnabled) {
      Serial.print("Center volume set to: ");
      Serial.println(currentCenterVol);
    }
  }
  if (server.hasArg("sub")) {
    currentSubVol = server.arg("sub").toInt();
    pt.setSub_att(15 - currentSubVol);
    if (loggingEnabled) {
      Serial.print("Subwoofer volume set to: ");
      Serial.println(currentSubVol);
    }
  }
  if (server.hasArg("front")) {
    currentFrontVol = server.arg("front").toInt();
    pt.setFront_lk_att(15 - currentFrontVol);
    pt.setFront_rk_att(15 - currentFrontVol);
    if (loggingEnabled) {
      Serial.print("Front volume set to: ");
      Serial.println(currentFrontVol);
    }
  }
  if (server.hasArg("rear")) {
    currentRearVol = server.arg("rear").toInt();
    pt.setRear_lk_att(15 - currentRearVol);
    pt.setRear_rk_att(15 - currentRearVol);
    if (loggingEnabled) {
      Serial.print("Rear volume set to: ");
      Serial.println(currentRearVol);
    }
  }
}
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
void setup() {
    Serial.begin(9600);
  
  // WiFi Manager Setup
  WiFiManager wifiManager;
  wifiManager.autoConnect("Home Theater", "12345678"); // Customize your AP SSID and password
  Serial.println("Connected to WiFi");

  setupMDNS();
  setupServer();
  setupPins();
  speaker.onPowerState(onPowerState);
  speaker.onMute(onMute);
  speaker.onSetVolume([](const String &deviceId, int &volume) -> bool {
    return onSetVolume(deviceId, volume);
  });
  // Add devices to SinricPro
  SinricPro.onConnected([]() {
    if (loggingEnabled) {
      Serial.println("[SinricPro]: Connected");
    }
  });
  SinricPro.onDisconnected([]() {
    if (loggingEnabled) {
      Serial.println("[SinricPro]: Disconnected");
    }
  });SinricPro.begin(APP_KEY, APP_SECRET);
  if (loggingEnabled) {
    Serial.println("System started and ready to receive IR commands.");
  }
  applySettings();
}
void setupMDNS() {
  if (!MDNS.begin("Home")) {
    if (loggingEnabled) {
      Serial.println("Error setting up MDNS responder!");
    }
    while (1) delay(1000);
  }
  if (loggingEnabled) {
    Serial.println("mDNS responder started");
  }
}
void setupServer() {
  server.on("/", handleRoot);
  server.on("/setVolume", handleSetVolume);
  server.on("/setFunc", HTTP_POST, handleSetFunc);
  server.begin();
  if (loggingEnabled) {
    Serial.println("HTTP server started");
  }
}
void setupPins() {
  // Configuração de pinos de saída
  pinMode(PSON_PIN, OUTPUT);
  pinMode(BLUETOOTH_PIN, OUTPUT);
  pinMode(AUDIO_IN_PIN, OUTPUT);
  pinMode(AUDIO_51_PIN, OUTPUT);  // Configuração do pino do 5.1
  pinMode(RELAY_PIN, OUTPUT); // Configura o pino do relé como saída
  
  // Inicialização dos pinos
  digitalWrite(PSON_PIN, LOW);
  digitalWrite(BLUETOOTH_PIN, LOW);
  digitalWrite(AUDIO_IN_PIN, LOW);
  digitalWrite(AUDIO_51_PIN, LOW);  // Inicializa o pino do 5.1 como LOW
  digitalWrite(RELAY_PIN, LOW); // Assegura que o relé comece desligado


  pinMode(D5, OUTPUT);
  digitalWrite(D5, LOW);

  

  // Ativação do receptor IR
  irrecv.enableIRIn();
  
  if (loggingEnabled) {
    Serial.println("IR Receiver activated");
  }
}
void loop() {
  server.handleClient();
  MDNS.update();
  SinricPro.handle();

  // Processa comandos de IR apenas quando há novos comandos
  if (irrecv.decode(&results)) {
    if (loggingEnabled) {
      Serial.println("IR command received: " + String(results.value, HEX));
    }
    processIRCommand(&results);
    irrecv.resume();  // Retoma o receptor de IR após processar o comando
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

  // Intervalo para aliviar o loop principal
  delay(1);
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
  if (loggingEnabled) {
    Serial.println("Configurações aplicadas ao PT2322.");
  }
}
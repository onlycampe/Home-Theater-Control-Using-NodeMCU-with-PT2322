#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "PT2322.h"
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <SinricPro.h>
#include <SinricProSpeaker.h>

// Wi-Fi Network Credentials
const char* ssid = "ssid";
const char* password = "password";

#define RECV_PIN D4          // Pin where the IR receiver is connected
#define PSON_PIN D8          // Power on/off pin for the Speaker
#define BLUETOOTH_PIN D6     // Bluetooth on/off pin (controls a BC547 relay)
#define AUDIO_IN_PIN D5      // Audio In control pin (selects audio inputs on a HD Audio Rush decoder, simulating a physical button press)
// Device settings
#define DEVICE_ID        "xxxxxxx"
#define APP_KEY           "xxxxxx"
#define APP_SECRET        "xxxxxxxxxxxxxxxxx"

IRrecv irrecv(RECV_PIN);      // Creates an instance of the IRrecv class for handling IR signals, using the pin defined by RECV_PIN
decode_results results;       // Variable to store the results of decoded IR signals
SinricProSpeaker& speaker = SinricPro[DEVICE_ID]; // Creates a reference to the SinricProSpeaker instance with the given SPEAKER_ID

// Variáveis de controle
bool isSystemOn = false;
bool isBluetoothOn = false;
bool isMuteOn = false;
bool isDddOn = true;  
bool isTembOn = false;
bool loggingEnabled = false;

// Declaração dos callbacks
bool onPowerState(const String &deviceId, bool &state);
bool onSetVolume(const String &deviceId, int &volume);
bool onAdjustVolume(const String &deviceId, int &volume, bool state);
bool onMute(const String &deviceId, bool &state);

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
  // Obtenha o estado atual dos botões
  String systemClass = isSystemOn ? "green" : "red";
  String bluetoothClass = isBluetoothOn ? "green" : "red";
  String muteClass = isMuteOn ? "green" : "red";
  String dddClass = isDddOn ? "green" : "red";

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<title>Control Panel</title>";
  html += "<style>";
  html += "@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap');";
  html += "body { font-family: 'Roboto', Arial, sans-serif; margin: 0; padding: 0; background-color: #222; color: #fff; height: 100vh; width: 100vw; display: flex; justify-content: center; align-items: center; }";
  html += ".container { width: 90%; max-width: 900px; height: 100%; margin: 0 auto; text-align: center; display: flex; flex-direction: column; justify-content: space-between; }";
  html += "h1 { font-size: 4em; text-align: center; font-family: 'Roboto', sans-serif; margin: 100px 0 30px; }";
  html += "p { font-size: 2em; text-align: center; }";
  html += ".button-container { width: 100%; display: flex; flex-wrap: wrap; justify-content: center; margin: 50px 0; }";
  html += ".button-container button { font-size: 2em; margin: 10px; padding: 15px 30px; cursor: pointer; border-radius: 50%; color: #fff; border: none; transition: background-color 0.2s ease-in-out; width: 25vw; height: 25vw; max-width: 300px; max-height: 300px; }";
  html += ".button-container button.red { background-color: #4c4f51; }";
  html += ".button-container button.green { background-color: #2485c9; }";
  html += ".button-container button#audioInButton { background-color: #4c4f51; }";
  html += ".range-container { flex: 1; width: 100%; display: flex; flex-direction: column; align-items: center; padding: 0 10px; margin-top: 10px; }";
  html += "form { width: 100%; max-width: 100%; display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 10px; border-radius: 5px; }";
  html += ".form-text { font-size: 2.5em; margin: 20px 0; }";
  html += "input[type='range']{-webkit-appearance:none;width:100%;height:20px;background:#4c4f51;border-radius:20px;cursor:pointer;margin-bottom:25px;}";
  html += "input[type='range']::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:60px;height:60px;border-radius:50%;background:#2485c9;cursor:pointer;}";
  html += "input[type='range']::-moz-range-thumb{width:40px;height:40px;border-radius:50%;background:#2485c9;cursor:pointer;}";
  html += "@media (max-width: 768px) { h1 { font-size: 2em; } .form-text { font-size: 2em; } input[type='range'] { width: calc(95% - 40px); } .button-container button { font-size: 1.5em; margin: 15px 5px; padding: 15px 30px; } }";
  html += "</style>";
  html += "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.6.0/css/all.min.css\">";
  html += "</head><body>";
  html += "<div class=\"container\">";
  html += "<h1>Home Theater Control</h1>";
  html += "<div class=\"button-container\">";
  html += "<button id=\"systemButton\" class=\"" + systemClass + "\" onclick=\"toggleButton('system')\"><i class=\"fas fa-power-off fa-2x\"></i></button>";
  html += "<button id=\"bluetoothButton\" class=\"" + bluetoothClass + "\" onclick=\"toggleButton('bluetooth')\"><i class=\"fa-brands fa-bluetooth-b fa-2x\"></i></button>";
  html += "<button id=\"muteButton\" class=\"" + muteClass + "\" onclick=\"toggleButton('mute')\"><i class=\"fas fa-volume-mute fa-2x\"></i></button>";
  html += "<button id=\"dddButton\" class=\"" + dddClass + "\" onclick=\"toggleButton('ddd')\"><i class=\"fa-solid fa-cube fa-2x\"></i></button>";
  html += "<button id=\"audioInButton\" class=\"" + audioInClass + "\" onclick=\"toggleButton('audioIn')\"><i class=\"fas fa-music fa-2x\"></i></button>";
  html += "</div>";
  html += "<div class=\"range-container\">";
  html += "<form action=\"/setVolume\" method=\"POST\">";
  html += "<span class=\"form-text\">Volume Total</span>";
  html += "<input type=\"range\" id=\"total\" name=\"total\" min=\"0\" max=\"79\" value=\"" + String(currentTotalVol) + "\" oninput=\"updateVolume(this)\"><br>";
  html += "<span class=\"form-text\">Center</span>";
  html += "<input type=\"range\" id=\"center\" name=\"center\" min=\"0\" max=\"15\" value=\"" + String(currentCenterVol) + "\" oninput=\"updateVolume(this)\"><br>";
  html += "<span class=\"form-text\">Subwoofer</span>";
  html += "<input type=\"range\" id=\"sub\" name=\"sub\" min=\"0\" max=\"15\" value=\"" + String(currentSubVol) + "\" oninput=\"updateVolume(this)\"><br>";
  html += "<span class=\"form-text\">Front L/R</span>";
  html += "<input type=\"range\" id=\"front\" name=\"front\" min=\"0\" max=\"15\" value=\"" + String(currentFrontVol) + "\" oninput=\"updateVolume(this)\"><br>";
  html += "<span class=\"form-text\">Surround L/R</span>";
  html += "<input type=\"range\" id=\"rear\" name=\"rear\" min=\"0\" max=\"15\" value=\"" + String(currentRearVol) + "\" oninput=\"updateVolume(this)\"><br>";
  html += "</form>";
  html += "<p><a href=\"https://github.com/onlycampe\" target=\"_blank\" style=\"color: #4c4f51; text-decoration: none;\"><i class=\"fa-brands fa-github-alt fa-1x\"></i> OnlyCampe</a></p>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "function updateVolume(input){";
  html += "var xhr=new XMLHttpRequest();";
  html += "xhr.open('POST','/setVolume',true);";
  html += "xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');";
  html += "xhr.send(input.name+'='+input.value);";
  html += "}";
  html += "function toggleButton(buttonId){";
  html += "var xhr=new XMLHttpRequest();";
  html += "xhr.onreadystatechange=function(){";
  html += "if(this.readyState==4&&this.status==200){";
  html += "var button=document.getElementById(buttonId+'Button');";
  html += "button.classList.toggle('red');";
  html += "button.classList.toggle('green');";
  html += "}};";
  html += "xhr.open('POST','/setFunc',true);";
  html += "xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');";
  html += "xhr.send('func='+buttonId+'&state='+(document.getElementById(buttonId+'Button').classList.contains('green')?'0':'1'));";
  html += "}";
  html += "</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}
// Declaração das funções
void applySettings();
void updateVolumeLog(int newVolume);

void togglePin(int pin, bool &state, const String &onMessage, const String &offMessage) {
  state = !state;
  digitalWrite(pin, state ? HIGH : LOW);
  if (loggingEnabled) {
    Serial.println(state ? onMessage : offMessage);
  }
}

bool setAudioFunction(bool &currentState, bool state, void (*setFunc)(int, int, int), int &param1, int &param2) {
  currentState = state ? 1 : 0;
  setFunc(currentMute, currentDdd, currentTemb);
  applySettings();
  return true;
}

bool onPowerState(const String &deviceId, bool &state) {
  togglePin(PSON_PIN, isSystemOn, "System turned on", "System turned off");
  return true;
}

bool onSetVolume(const String &deviceId, int &volume) {
  currentTotalVol = constrain(volume, 0, 79);
  pt.setVol(currentTotalVol);
  if (loggingEnabled) {
    Serial.println("Volume geral alterado");
  }
  return true;
}

bool onAdjustVolume(const String &deviceId, int &volume, bool state) {
  currentTotalVol = constrain(currentTotalVol + volume, 0, 79);
  pt.setVol(currentTotalVol);
  if (loggingEnabled) {
    Serial.println("Volume geral alterado");
  }
  return true;
}

bool onMute(const String &deviceId, bool &state) {
  currentMute = state ? 1 : 0;
  applySettings();
  if (loggingEnabled) {
    Serial.println(state ? "Mute activated" : "Mute deactivated");
  }
  return true;
}

bool onDdd(const String &deviceId, bool &state) {
  currentDdd = state ? 1 : 0;
  applySettings();
  if (loggingEnabled) {
    Serial.println(state ? "DDD activated" : "DDD deactivated");
  }
  return true;
}

void handleSetFunc() {
  String func = server.arg("func");
  bool state = server.arg("state").toInt() != 0;

  if (func == "bluetooth") {
    togglePin(BLUETOOTH_PIN, isBluetoothOn, "Bluetooth turned on", "Bluetooth turned off");
    isBluetoothOn = state;
  } else if (func == "system") {
    togglePin(PSON_PIN, isSystemOn, "System turned on", "System turned off");
    isSystemOn = state;
  } else if (func == "audioIn") {
    digitalWrite(AUDIO_IN_PIN, HIGH);
    delay(100);
    digitalWrite(AUDIO_IN_PIN, LOW);
    if (loggingEnabled) {
      Serial.println("Audio In pulse sent.");
    }
  } else if (func == "mute") {
    onMute("", state);
    isMuteOn = state;
  } else if (func == "ddd") {
    onDdd("", state);
    isDddOn = state;
  } else {
    updateAudioFunctions(func, state);
  }

  server.send(200, "text/plain", func + " state updated");
}

void updateAudioFunctions(const String &func, bool state) {
  if (func == "mute") {
    currentMute = state ? 1 : 0;
  } else if (func == "ddd") {
    currentDdd = state ? 1 : 0;
  }
  applySettings();
}

void handleSetAudioSettings() {
  if (server.hasArg("bass")) {
    int bassValue = server.arg("bass").toInt();
    pt.setBass(bassValue);
    if (loggingEnabled) {
      Serial.println("Bass set to: " + String(bassValue));
    }
  }
  if (server.hasArg("middle")) {
    int middleValue = server.arg("middle").toInt();
    pt.setMiddle(middleValue);
    if (loggingEnabled) {
      Serial.println("Middle set to: " + String(middleValue));
    }
  }
  if (server.hasArg("treble")) {
    int trebleValue = server.arg("treble").toInt();
    pt.setTreble(trebleValue);
    if (loggingEnabled) {
      Serial.println("Treble set to: " + String(trebleValue));
    }
  }
  applySettings();
}

void handleSetVolume() {
  if (server.hasArg("total")) {
    currentTotalVol = constrain(server.arg("total").toInt(), 0, 79);
    onSetVolume(DEVICE_ID, currentTotalVol);
  }
  if (server.hasArg("center")) {
    currentCenterVol = server.arg("center").toInt();
    if (loggingEnabled) {
      Serial.println("Volume central alterado");
    }
  }
  if (server.hasArg("sub")) {
    currentSubVol = server.arg("sub").toInt();
    if (loggingEnabled) {
      Serial.println("Volume subwoofer alterado");
    }
  }
  if (server.hasArg("front")) {
    currentFrontVol = server.arg("front").toInt();
    if (loggingEnabled) {
      Serial.println("Volume frontal alterado");
    }
  }
  if (server.hasArg("rear")) {
    currentRearVol = server.arg("rear").toInt();
    if (loggingEnabled) {
      Serial.println("Volume traseiro alterado");
    }
  }
  applySettings();
}

void processIRCommand(decode_results *results) {
  if (results->decode_type == UNKNOWN) return;

  switch (results->value) {
    case 0xE0E0E01F:
      if (currentTotalVol < 79) pt.setVol(++currentTotalVol);
      if (loggingEnabled) {
        Serial.println("Volume geral aumentado");
      }
      break;
    case 0xE0E0D02F:
      if (currentTotalVol > 0) pt.setVol(--currentTotalVol);
      if (loggingEnabled) {
        Serial.println("Volume geral diminuído");
      }
      break;
    case 0xE0E0F00F:
      togglePin(PSON_PIN, isSystemOn, "Speaker on", "Speaker off");
      break;
  }
  irrecv.resume();
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (loggingEnabled) {
      Serial.println("Connecting to WiFi...");
    }
  }
  if (loggingEnabled) {
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  speaker.onPowerState(onPowerState);
  speaker.onSetVolume(onSetVolume);
  speaker.onAdjustVolume(onAdjustVolume);
  speaker.onMute(onMute);

  SinricPro.begin(APP_KEY, APP_SECRET);

  setupMDNS();
  setupServer();
  setupPins();

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
  server.on("/setAudioSettings", handleSetAudioSettings);
  server.on("/setFunc", HTTP_POST, handleSetFunc);
  server.begin();
  if (loggingEnabled) {
    Serial.println("HTTP server started");
  }
}

void setupPins() {
  pinMode(PSON_PIN, OUTPUT);
  pinMode(BLUETOOTH_PIN, OUTPUT);
  pinMode(AUDIO_IN_PIN, OUTPUT);
  digitalWrite(PSON_PIN, LOW);
  digitalWrite(BLUETOOTH_PIN, LOW);
  digitalWrite(AUDIO_IN_PIN, LOW);
  
  pinMode(D5, OUTPUT);
  digitalWrite(D5, LOW);
  
  irrecv.enableIRIn();
  if (loggingEnabled) {
    Serial.println("IR Receiver activated");
  }
}

void loop() {
  server.handleClient();
  MDNS.update();
  SinricPro.handle();

  if (irrecv.decode(&results)) {
    if (loggingEnabled) {
      Serial.println("IR command received: " + String(results.value, HEX));
    }
    processIRCommand(&results);
    applySettings();
  }
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
    Serial.println("Configurações aplicadas ao PT2322:");
  }
}

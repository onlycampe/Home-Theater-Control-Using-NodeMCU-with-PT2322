#ifndef CONFIG_H
#define CONFIG_H

// ⚠️ INSTRUÇÕES IMPORTANTES:
// 1. Copie este arquivo e renomeie para "config.h"
// 2. Preencha com suas credenciais REAIS do SinricPro
// 3. NUNCA faça commit do arquivo "config.h" no GitHub
// 4. Apenas o "config.example.h" deve ir para o Git

// =============================================================================
// CREDENCIAIS SINRIC PRO
// =============================================================================
// Obtenha suas credenciais em: https://sinric.pro/
#define SPEAKER_DEVICE_ID       "cole_seu_speaker_device_id_aqui"
#define SWITCH_DEVICE_ID        "cole_seu_switch_device_id_aqui"
#define APP_KEY                 "cole_sua_app_key_aqui"
#define APP_SECRET              "cole_seu_app_secret_aqui"

// =============================================================================
// CONFIGURAÇÕES DO WIFI MANAGER
// =============================================================================
#define WIFI_AP_NAME            "Home Theater"
#define WIFI_AP_PASSWORD        "SuaSenhaForte@2025"  // ⚠️ TROQUE por senha forte!

// =============================================================================
// CONFIGURAÇÕES OTA (Over-The-Air Updates)
// =============================================================================
#define OTA_HOSTNAME            "hometheater"         // Nome do dispositivo na rede
#define OTA_PASSWORD            "SuaSenhaOTA@2025"    // ⚠️ TROQUE por senha forte!

// =============================================================================
// CONFIGURAÇÕES mDNS
// =============================================================================
#define MDNS_NAME               "hometheater"         // http://hometheater.local
#define MDNS_ALIAS              "home"                // http://home.local (alias)

// =============================================================================
// CONFIGURAÇÕES DE DEBUG
// =============================================================================
#define ENABLE_SERIAL_LOGGING   true

// =============================================================================
// CONFIGURAÇÃO DE HARDWARE (PINOS)
// =============================================================================
#define RECV_PIN        D4   // Pin onde o receptor IR está conectado
#define PSON_PIN        D8   // Power on/off pin para o Speaker
#define BLUETOOTH_PIN   D6   // Bluetooth on/off pin (controla relé BC547)
#define AUDIO_IN_PIN    D5   // Controle de entrada de áudio
#define AUDIO_51_PIN    D7   // Controle 5.1/2.1
#define RELAY_PIN       D3   // Pino de controle para relés

#endif


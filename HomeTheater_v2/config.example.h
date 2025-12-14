#ifndef CONFIG_H
#define CONFIG_H

// ⚠️ IMPORTANT INSTRUCTIONS:
// 1. Copy this file and rename it to "config.h"
// 2. Fill in with your REAL SinricPro credentials
// 3. NEVER commit the "config.h" file to GitHub
// 4. Only "config.example.h" should go to Git

// =============================================================================
// SINRIC PRO CREDENTIALS
// =============================================================================
// Get your credentials at: https://sinric.pro/
#define SPEAKER_DEVICE_ID       "paste_your_speaker_device_id_here"
#define SWITCH_DEVICE_ID        "paste_your_switch_device_id_here"
#define APP_KEY                 "paste_your_app_key_here"
#define APP_SECRET              "paste_your_app_secret_here"

// =============================================================================
// WIFI MANAGER SETTINGS
// =============================================================================
#define WIFI_AP_NAME            "Home Theater"
#define WIFI_AP_PASSWORD        "YourStrongPassword@2025"  // ⚠️ CHANGE to strong password!

// =============================================================================
// OTA (Over-The-Air Updates) SETTINGS
// =============================================================================
#define OTA_HOSTNAME            "hometheater"              // Device name on network
#define OTA_PASSWORD            "YourOTAPassword@2025"     // ⚠️ CHANGE to strong password!

// =============================================================================
// mDNS SETTINGS
// =============================================================================
#define MDNS_NAME               "hometheater"              // http://hometheater.local
#define MDNS_ALIAS              "home"                     // http://home.local (alias)

// =============================================================================
// DEBUG SETTINGS
// =============================================================================
#define ENABLE_SERIAL_LOGGING   true

// =============================================================================
// HARDWARE CONFIGURATION (PINS)
// =============================================================================
#define RECV_PIN        D4   // Pin where IR receiver is connected
#define PSON_PIN        D8   // Power on/off pin for Speaker
#define BLUETOOTH_PIN   D6   // Bluetooth on/off pin (controls BC547 relay)
#define AUDIO_IN_PIN    D5   // Audio input control
#define AUDIO_51_PIN    D7   // 5.1/2.1 control
#define RELAY_PIN       D3   // Control pin for relays

#endif


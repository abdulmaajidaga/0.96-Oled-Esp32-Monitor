#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN CONFIGURATION - ESP32-S3
// ============================================

// I2C OLED Display Pins
#define OLED_SDA_PIN        8
#define OLED_SCL_PIN        9
#define OLED_ADDRESS        0x3C

// Button Pins (Active LOW with internal pull-up)
#define BTN_UP_PIN          4
#define BTN_DOWN_PIN        5
#define BTN_ENTER_PIN       6

// ============================================
// DISPLAY SETTINGS
// ============================================
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64

// ============================================
// WIFI SETTINGS - UPDATE THESE!
// ============================================
#define WIFI_SSID           "YourWiFiSSID"
#define WIFI_PASSWORD       "YourWiFiPassword"

// Laptop telemetry server address
// Run the Python script on your laptop, then update this IP
#define TELEMETRY_HOST      "YourLaptopIP"  // e.g. "192.168.0.145"
#define TELEMETRY_PORT      5000
#define TELEMETRY_ENDPOINT  "/telemetry"

// ============================================
// TIMING SETTINGS
// ============================================
#define BUTTON_DEBOUNCE_MS      50
#define DATA_REFRESH_MS         2000    // Fetch telemetry every 2 seconds (less blocking)
#define MENU_ANIMATION_MS       16      // ~60fps for smooth scrolling
#define WIFI_CONNECT_TIMEOUT_MS 10000

// ============================================
// MENU SETTINGS
// ============================================
#define MENU_ITEMS_VISIBLE      3       // Show 3 items on screen
#define MENU_ITEM_HEIGHT        17      // Menu item height
#define SCROLL_SPEED            4       // Pixels per frame for smooth scroll

#endif // CONFIG_H

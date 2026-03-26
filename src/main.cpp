#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "icons.h"
#include "buttons.h"
#include "telemetry.h"
#include "menu.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);

ButtonHandler buttons;
TelemetryData telemetryData;
MenuSystem menu;

// Timing
unsigned long lastDataFetch = 0;
unsigned long lastRender = 0;

// WiFi failure tracking
int wifiFailCount = 0;
const int MAX_WIFI_FAILS = 5;

// ============================================
// NICE BOOT SCREEN
// ============================================

void showBootScreen() {
    display.clearBuffer();

    // Double border frame
    display.drawRFrame(0, 0, 128, 64, 4);
    display.drawRFrame(3, 3, 122, 58, 3);

    // Big title
    display.setFont(u8g2_font_helvB14_tf);
    display.drawStr(8, 28, "TELEMETRY");

    // Subtitle
    display.setFont(u8g2_font_helvB10_tf);
    display.drawStr(25, 45, "MONITOR");

    // Version/branding line
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(45, 58, "v1.0");

    display.sendBuffer();
    delay(1500);
}

// ============================================
// WIFI CONNECTION WITH NICE UI
// ============================================

void showConnecting(int progress) {
    display.clearBuffer();

    // Header
    display.drawBox(0, 0, 128, 14);
    display.setDrawColor(0);
    display.setFont(u8g2_font_7x14B_tf);
    display.drawStr(20, 12, "CONNECTING");
    display.setDrawColor(1);

    // WiFi icon area
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(10, 32, "Network:");
    display.setFont(u8g2_font_7x14B_tf);
    display.drawStr(10, 48, WIFI_SSID);

    // Progress bar
    display.drawRFrame(10, 52, 108, 8, 2);
    if (progress > 0) {
        int fillWidth = (progress * 104) / 100;
        display.drawRBox(12, 54, fillWidth, 4, 1);
    }

    display.sendBuffer();
}

void showConnected() {
    display.clearBuffer();

    // Header - success style
    display.drawBox(0, 0, 128, 14);
    display.setDrawColor(0);
    display.setFont(u8g2_font_7x14B_tf);
    display.drawStr(22, 12, "CONNECTED!");
    display.setDrawColor(1);

    // Checkmark area
    display.setFont(u8g2_font_9x15B_tf);
    display.drawStr(56, 35, "OK");

    // IP Address
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(10, 50, "IP:");
    display.setFont(u8g2_font_7x14B_tf);
    String ip = WiFi.localIP().toString();
    display.drawStr(30, 50, ip.c_str());

    // Ready message
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(30, 62, "Starting menu...");

    display.sendBuffer();
    delay(1500);
}

void showConnectionFailed() {
    display.clearBuffer();

    // Header - error style
    display.drawBox(0, 0, 128, 14);
    display.setDrawColor(0);
    display.setFont(u8g2_font_7x14B_tf);
    display.drawStr(10, 12, "WIFI FAILED!");
    display.setDrawColor(1);

    // Error message
    display.setFont(u8g2_font_7x14B_tf);
    display.drawStr(10, 35, "No Connection");

    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(10, 50, "Check WiFi settings");
    display.drawStr(10, 62, "Retrying...");

    display.sendBuffer();
    delay(2000);
}

bool connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startTime = millis();
    int progress = 0;

    while (WiFi.status() != WL_CONNECTED) {
        // Update progress bar
        unsigned long elapsed = millis() - startTime;
        progress = (elapsed * 100) / WIFI_CONNECT_TIMEOUT_MS;
        if (progress > 95) progress = 95;  // Don't show 100% until connected

        showConnecting(progress);
        delay(200);

        if (millis() - startTime > WIFI_CONNECT_TIMEOUT_MS) {
            return false;
        }
    }

    showConnected();
    return true;
}

// ============================================
// WIFI TELEMETRY FETCH
// ============================================

bool fetchWiFiTelemetry() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String url = String("http://") + TELEMETRY_HOST + ":" + TELEMETRY_PORT + TELEMETRY_ENDPOINT;
    http.begin(url);
    http.setTimeout(300);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;

        if (!deserializeJson(doc, payload)) {
            telemetryData.cpu.usagePercent = doc["cpu"]["usage"] | 0.0f;
            telemetryData.cpu.frequencyGHz = doc["cpu"]["freq"] | 0.0f;
            telemetryData.cpu.coreCount = doc["cpu"]["cores"] | 0;

            telemetryData.ram.usagePercent = doc["ram"]["usage"] | 0.0f;
            telemetryData.ram.usedGB = doc["ram"]["used"] | 0.0f;
            telemetryData.ram.totalGB = doc["ram"]["total"] | 0.0f;

            telemetryData.gpu.usagePercent = doc["gpu"]["usage"] | 0.0f;
            telemetryData.gpu.temperature = doc["gpu"]["temp"] | 0.0f;
            telemetryData.gpu.vramUsedGB = doc["gpu"]["vram_used"] | 0.0f;
            telemetryData.gpu.vramTotalGB = doc["gpu"]["vram_total"] | 0.0f;

            telemetryData.network.uploadMBps = doc["network"]["upload"] | 0.0f;
            telemetryData.network.downloadMBps = doc["network"]["download"] | 0.0f;

            telemetryData.disk.readMBps = doc["disk"]["read"] | 0.0f;
            telemetryData.disk.writeMBps = doc["disk"]["write"] | 0.0f;
            telemetryData.disk.usagePercent = doc["disk"]["active"] | 0.0f;  // Active time like Task Manager

            telemetryData.lastUpdate = millis();
            telemetryData.isValid = true;
            http.end();
            wifiFailCount = 0;
            return true;
        }
    }

    http.end();
    wifiFailCount++;
    return false;
}

// ============================================
// SETUP
// ============================================

void setup() {
    Serial.begin(115200);

    display.begin();
    display.setContrast(255);

    showBootScreen();

    buttons.begin();
    memset(&telemetryData, 0, sizeof(telemetryData));
    menu.begin(&display, &telemetryData);

    // Connect to WiFi
    while (!connectWiFi()) {
        showConnectionFailed();
        // Keep retrying
    }
}

// ============================================
// LOOP
// ============================================

void loop() {
    unsigned long now = millis();

    // Handle buttons
    ButtonEvent event = buttons.update();
    switch (event) {
        case BTN_UP_PRESSED:
            menu.navigateUp();
            break;
        case BTN_DOWN_PRESSED:
            menu.navigateDown();
            break;
        case BTN_ENTER_PRESSED:
            // Short press: select in menu, go back in sub-screens
            if (menu.isMainMenu()) {
                menu.select();
            } else {
                menu.back();
            }
            break;
        case BTN_ENTER_LONG_PRESS:
            menu.back();
            break;
        default:
            break;
    }

    // Fetch telemetry data
    if (now - lastDataFetch >= DATA_REFRESH_MS) {
        lastDataFetch = now;

        if (wifiFailCount >= MAX_WIFI_FAILS) {
            // Too many failures - try reconnecting
            telemetryData.isValid = false;
            if (WiFi.status() != WL_CONNECTED) {
                WiFi.reconnect();
            }
            wifiFailCount = 0;
        } else {
            fetchWiFiTelemetry();
        }
    }

    // Update and render menu
    menu.update();
    if (now - lastRender >= MENU_ANIMATION_MS) {
        lastRender = now;
        menu.render();
    }

    yield();
}

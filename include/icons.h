#ifndef ICONS_H
#define ICONS_H

#include <Arduino.h>

// ============================================
// FLIPPER ZERO STYLE ICONS (12x12 pixels)
// Created with Flipper aesthetic - clean, minimal, rounded
// ============================================

// CPU Icon - chip with pins
const uint8_t icon_cpu[] PROGMEM = {
    0b00100100,
    0b00100100,
    0b11111111,
    0b10000001,
    0b10111101,
    0b10100101,
    0b10100101,
    0b10111101,
    0b10000001,
    0b11111111,
    0b00100100,
    0b00100100
};
#define ICON_CPU_WIDTH 8
#define ICON_CPU_HEIGHT 12

// RAM Icon - memory stick
const uint8_t icon_ram[] PROGMEM = {
    0b11111111,
    0b10000001,
    0b10101011,
    0b10101011,
    0b10101011,
    0b10000001,
    0b11111111,
    0b01010101,
    0b01010101,
    0b01010101,
    0b01010101,
    0b00000000
};
#define ICON_RAM_WIDTH 8
#define ICON_RAM_HEIGHT 12

// GPU Icon - graphics card
const uint8_t icon_gpu[] PROGMEM = {
    0b11111111,
    0b10000001,
    0b10111101,
    0b10100001,
    0b10111101,
    0b10000001,
    0b11111111,
    0b11100111,
    0b00000000,
    0b11111111,
    0b01010101,
    0b00000000
};
#define ICON_GPU_WIDTH 8
#define ICON_GPU_HEIGHT 12

// Network Icon - up/down arrows
const uint8_t icon_net[] PROGMEM = {
    0b00010000,
    0b00111000,
    0b01010100,
    0b00010000,
    0b00010000,
    0b00000000,
    0b00001000,
    0b00001000,
    0b00101010,
    0b00011100,
    0b00001000,
    0b00000000
};
#define ICON_NET_WIDTH 8
#define ICON_NET_HEIGHT 12

// Disk Icon - hard drive
const uint8_t icon_disk[] PROGMEM = {
    0b01111110,
    0b11111111,
    0b10000001,
    0b10000001,
    0b10000001,
    0b11111111,
    0b11111111,
    0b10000011,
    0b10000011,
    0b10000001,
    0b11111111,
    0b01111110
};
#define ICON_DISK_WIDTH 8
#define ICON_DISK_HEIGHT 12

// Dashboard Icon - grid of elements
const uint8_t icon_dash[] PROGMEM = {
    0b11111111,
    0b10000001,
    0b10110101,
    0b10110101,
    0b10000001,
    0b10101101,
    0b10101101,
    0b10000001,
    0b10010011,
    0b10010011,
    0b10000001,
    0b11111111
};
#define ICON_DASH_WIDTH 8
#define ICON_DASH_HEIGHT 12

// Settings Icon - gear
const uint8_t icon_settings[] PROGMEM = {
    0b00011000,
    0b00111100,
    0b11100111,
    0b11000011,
    0b11100111,
    0b01111110,
    0b01111110,
    0b11100111,
    0b11000011,
    0b11100111,
    0b00111100,
    0b00011000
};
#define ICON_SETTINGS_WIDTH 8
#define ICON_SETTINGS_HEIGHT 12

// WiFi Icon - signal waves
const uint8_t icon_wifi[] PROGMEM = {
    0b00000000,
    0b01111110,
    0b11000011,
    0b00111100,
    0b01000010,
    0b00011000,
    0b00100100,
    0b00000000,
    0b00011000,
    0b00011000,
    0b00000000,
    0b00000000
};
#define ICON_WIFI_WIDTH 8
#define ICON_WIFI_HEIGHT 12

// No WiFi Icon - X mark
const uint8_t icon_no_wifi[] PROGMEM = {
    0b10000001,
    0b01000010,
    0b00100100,
    0b00011000,
    0b00011000,
    0b00100100,
    0b01000010,
    0b10000001,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
};
#define ICON_NO_WIFI_WIDTH 8
#define ICON_NO_WIFI_HEIGHT 12

// Back Arrow Icon
const uint8_t icon_back[] PROGMEM = {
    0b00000000,
    0b00100000,
    0b01100000,
    0b11111111,
    0b11111111,
    0b01100000,
    0b00100000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
};
#define ICON_BACK_WIDTH 8
#define ICON_BACK_HEIGHT 12

// Selection Arrow (right pointing)
const uint8_t icon_select[] PROGMEM = {
    0b00000010,
    0b00000110,
    0b00001110,
    0b00011110,
    0b00111110,
    0b00011110,
    0b00001110,
    0b00000110,
    0b00000010,
    0b00000000,
    0b00000000,
    0b00000000
};
#define ICON_SELECT_WIDTH 8
#define ICON_SELECT_HEIGHT 12

#endif // ICONS_H

#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"
#include "icons.h"
#include "telemetry.h"

enum MenuScreen {
    SCREEN_MAIN_MENU = 0,
    SCREEN_CPU,
    SCREEN_RAM,
    SCREEN_GPU,
    SCREEN_NETWORK,
    SCREEN_DISK,
    SCREEN_DASHBOARD,
    SCREEN_SETTINGS
};

struct MenuItem {
    const char* label;
    const uint8_t* icon;
    uint8_t iconWidth;
    uint8_t iconHeight;
    MenuScreen targetScreen;
};

class MenuSystem {
private:
    U8G2* display;
    TelemetryData* telemetry;

    MenuScreen currentScreen;
    int8_t selectedIndex;
    int8_t targetScrollOffset;

    static const uint8_t MENU_ITEM_COUNT = 7;
    MenuItem menuItems[MENU_ITEM_COUNT];

    float scrollPosition;

public:
    void begin(U8G2* disp, TelemetryData* tele) {
        display = disp;
        telemetry = tele;
        currentScreen = SCREEN_MAIN_MENU;
        selectedIndex = 0;
        targetScrollOffset = 0;
        scrollPosition = 0;

        menuItems[0] = {"CPU", icon_cpu, ICON_CPU_WIDTH, ICON_CPU_HEIGHT, SCREEN_CPU};
        menuItems[1] = {"Memory", icon_ram, ICON_RAM_WIDTH, ICON_RAM_HEIGHT, SCREEN_RAM};
        menuItems[2] = {"GPU", icon_gpu, ICON_GPU_WIDTH, ICON_GPU_HEIGHT, SCREEN_GPU};
        menuItems[3] = {"Network", icon_net, ICON_NET_WIDTH, ICON_NET_HEIGHT, SCREEN_NETWORK};
        menuItems[4] = {"Disk", icon_disk, ICON_DISK_WIDTH, ICON_DISK_HEIGHT, SCREEN_DISK};
        menuItems[5] = {"Dashboard", icon_dash, ICON_DASH_WIDTH, ICON_DASH_HEIGHT, SCREEN_DASHBOARD};
        menuItems[6] = {"Settings", icon_settings, ICON_SETTINGS_WIDTH, ICON_SETTINGS_HEIGHT, SCREEN_SETTINGS};
    }

    void navigateUp() {
        if (currentScreen == SCREEN_MAIN_MENU && selectedIndex > 0) {
            selectedIndex--;
            updateScrollTarget();
        }
    }

    void navigateDown() {
        if (currentScreen == SCREEN_MAIN_MENU && selectedIndex < MENU_ITEM_COUNT - 1) {
            selectedIndex++;
            updateScrollTarget();
        }
    }

    void select() {
        if (currentScreen == SCREEN_MAIN_MENU) {
            currentScreen = menuItems[selectedIndex].targetScreen;
        }
    }

    void back() {
        currentScreen = SCREEN_MAIN_MENU;
    }

    MenuScreen getCurrentScreen() {
        return currentScreen;
    }

    bool isMainMenu() {
        return currentScreen == SCREEN_MAIN_MENU;
    }

    void update() {
        scrollPosition = targetScrollOffset * MENU_ITEM_HEIGHT;
    }

    void render() {
        display->clearBuffer();

        switch (currentScreen) {
            case SCREEN_MAIN_MENU:  renderMainMenu(); break;
            case SCREEN_CPU:        renderCPUScreen(); break;
            case SCREEN_RAM:        renderRAMScreen(); break;
            case SCREEN_GPU:        renderGPUScreen(); break;
            case SCREEN_NETWORK:    renderNetworkScreen(); break;
            case SCREEN_DISK:       renderDiskScreen(); break;
            case SCREEN_DASHBOARD:  renderDashboard(); break;
            case SCREEN_SETTINGS:   renderSettings(); break;
            default:                renderMainMenu(); break;
        }

        display->sendBuffer();
    }

private:
    void updateScrollTarget() {
        if (selectedIndex < targetScrollOffset) {
            targetScrollOffset = selectedIndex;
        } else if (selectedIndex >= targetScrollOffset + MENU_ITEMS_VISIBLE) {
            targetScrollOffset = selectedIndex - MENU_ITEMS_VISIBLE + 1;
        }
        if (targetScrollOffset < 0) targetScrollOffset = 0;
        if (targetScrollOffset > MENU_ITEM_COUNT - MENU_ITEMS_VISIBLE) {
            targetScrollOffset = MENU_ITEM_COUNT - MENU_ITEMS_VISIBLE;
        }
    }

    void renderMainMenu() {
        // Header
        display->drawBox(0, 0, 128, 12);
        display->setDrawColor(0);
        display->setFont(u8g2_font_7x14B_tf);
        display->drawStr(3, 11, "TELEMETRY");
        display->setDrawColor(1);

        // Menu items
        int yStart = 14;

        for (int i = 0; i < MENU_ITEM_COUNT; i++) {
            int displayIndex = i - targetScrollOffset;
            if (displayIndex < 0 || displayIndex >= MENU_ITEMS_VISIBLE) continue;

            int y = yStart + (displayIndex * MENU_ITEM_HEIGHT);

            // Selected item - filled rounded box
            if (i == selectedIndex) {
                display->drawRBox(2, y, 122, MENU_ITEM_HEIGHT - 2, 3);
                display->setDrawColor(0);
            }

            // Icon
            display->drawXBMP(8, y + 2, menuItems[i].iconWidth, menuItems[i].iconHeight, menuItems[i].icon);

            // Label
            display->setFont(u8g2_font_7x14B_tf);
            display->drawStr(26, y + 13, menuItems[i].label);

            // Arrow
            if (i == selectedIndex) {
                display->drawStr(112, y + 13, ">");
            }

            display->setDrawColor(1);
        }

        // Scrollbar
        if (MENU_ITEM_COUNT > MENU_ITEMS_VISIBLE) {
            int barHeight = 50 * MENU_ITEMS_VISIBLE / MENU_ITEM_COUNT;
            int maxScroll = MENU_ITEM_COUNT - MENU_ITEMS_VISIBLE;
            int barY = 14 + (50 - barHeight) * targetScrollOffset / maxScroll;
            display->drawBox(126, barY, 2, barHeight);
        }
    }

    void renderHeader(const char* title) {
        display->drawBox(0, 0, 128, 14);
        display->setDrawColor(0);
        display->setFont(u8g2_font_7x14B_tf);
        display->drawStr(4, 12, title);
        display->setDrawColor(1);
    }

    void renderProgressBar(int x, int y, int width, int height, float percent) {
        // Draw frame first
        display->drawRFrame(x, y, width, height, 2);
        // Draw fill - minimum 3 pixels to look decent
        int fillWidth = (int)((width - 4) * (percent / 100.0));
        if (fillWidth >= 3) {
            display->drawBox(x + 2, y + 2, fillWidth, height - 4);
        }
    }

    void renderCPUScreen() {
        renderHeader("CPU");

        display->setFont(u8g2_font_7x14B_tf);
        char buf[32];

        // Usage
        snprintf(buf, sizeof(buf), "%.0f%%", telemetry->cpu.usagePercent);
        display->drawStr(4, 30, "Usage:");
        display->drawStr(56, 30, buf);
        renderProgressBar(4, 36, 120, 14, telemetry->cpu.usagePercent);

        // Cores info
        snprintf(buf, sizeof(buf), "%d Cores", telemetry->cpu.coreCount);
        display->drawStr(4, 62, buf);
    }

    void renderRAMScreen() {
        renderHeader("MEMORY");

        display->setFont(u8g2_font_7x14B_tf);
        char buf[32];

        snprintf(buf, sizeof(buf), "%.0f%%", telemetry->ram.usagePercent);
        display->drawStr(4, 30, "Used:");
        display->drawStr(50, 30, buf);
        renderProgressBar(4, 34, 120, 10, telemetry->ram.usagePercent);

        snprintf(buf, sizeof(buf), "%.1f / %.1f GB", telemetry->ram.usedGB, telemetry->ram.totalGB);
        display->drawStr(4, 58, buf);
    }

    void renderGPUScreen() {
        renderHeader("GPU");

        display->setFont(u8g2_font_7x14B_tf);
        char buf[32];

        snprintf(buf, sizeof(buf), "%.0f%%", telemetry->gpu.usagePercent);
        display->drawStr(4, 30, "Load:");
        display->drawStr(50, 30, buf);
        renderProgressBar(4, 34, 120, 10, telemetry->gpu.usagePercent);

        snprintf(buf, sizeof(buf), "%.0fC  %.1f/%.0fGB", telemetry->gpu.temperature, telemetry->gpu.vramUsedGB, telemetry->gpu.vramTotalGB);
        display->drawStr(4, 58, buf);
    }

    void renderNetworkScreen() {
        renderHeader("NETWORK");

        display->setFont(u8g2_font_7x14B_tf);
        char buf[32];

        snprintf(buf, sizeof(buf), "%.2f MB/s", telemetry->network.uploadMBps);
        display->drawStr(4, 35, "UP:");
        display->drawStr(40, 35, buf);

        snprintf(buf, sizeof(buf), "%.2f MB/s", telemetry->network.downloadMBps);
        display->drawStr(4, 55, "DN:");
        display->drawStr(40, 55, buf);
    }

    void renderDiskScreen() {
        renderHeader("DISK");

        display->setFont(u8g2_font_7x14B_tf);
        char buf[32];

        snprintf(buf, sizeof(buf), "%.0f%%", telemetry->disk.usagePercent);
        display->drawStr(4, 30, "Active:");
        display->drawStr(60, 30, buf);
        renderProgressBar(4, 34, 120, 10, telemetry->disk.usagePercent);

        snprintf(buf, sizeof(buf), "R:%.1f  W:%.1f MB/s", telemetry->disk.readMBps, telemetry->disk.writeMBps);
        display->drawStr(4, 58, buf);
    }

    void renderDashboard() {
        // Simple vertical layout - no overlap possible
        char buf[20];
        display->setFont(u8g2_font_7x14B_tf);

        // CPU row
        snprintf(buf, sizeof(buf), "CPU %3.0f%%", telemetry->cpu.usagePercent);
        display->drawStr(4, 14, buf);
        renderProgressBar(70, 4, 54, 10, telemetry->cpu.usagePercent);

        // RAM row
        snprintf(buf, sizeof(buf), "RAM %3.0f%%", telemetry->ram.usagePercent);
        display->drawStr(4, 30, buf);
        renderProgressBar(70, 20, 54, 10, telemetry->ram.usagePercent);

        // GPU row
        snprintf(buf, sizeof(buf), "GPU %3.0f%%", telemetry->gpu.usagePercent);
        display->drawStr(4, 46, buf);
        renderProgressBar(70, 36, 54, 10, telemetry->gpu.usagePercent);

        // DSK row
        snprintf(buf, sizeof(buf), "DSK %3.0f%%", telemetry->disk.usagePercent);
        display->drawStr(4, 62, buf);
        renderProgressBar(70, 52, 54, 10, telemetry->disk.usagePercent);
    }

    void renderSettings() {
        renderHeader("SETTINGS");

        display->setFont(u8g2_font_7x14B_tf);

        // WiFi status
        display->drawStr(4, 32, "WiFi:");
        if (telemetry->isValid) {
            display->drawDisc(55, 27, 4);
            display->drawStr(65, 32, "OK");
        } else {
            display->drawCircle(55, 27, 4);
            display->drawStr(65, 32, "---");
        }

        // Back instruction
        display->setFont(u8g2_font_6x10_tf);
        display->drawStr(4, 50, "ENTER: Back");
    }
};

#endif // MENU_H

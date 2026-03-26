#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

// ============================================
// TELEMETRY DATA STRUCTURE
// ============================================

struct CPUData {
    float usagePercent;
    float temperature;
    float frequencyGHz;
    uint8_t coreCount;
};

struct RAMData {
    float usagePercent;
    float usedGB;
    float totalGB;
    float swapUsedGB;
    float swapTotalGB;
};

struct GPUData {
    float usagePercent;
    float temperature;
    float vramUsedGB;
    float vramTotalGB;
    char name[32];
};

struct NetworkData {
    float uploadMBps;
    float downloadMBps;
    float totalUploadGB;
    float totalDownloadGB;
};

struct DiskData {
    float readMBps;
    float writeMBps;
    float usagePercent;
    float usedGB;
    float totalGB;
};

struct TelemetryData {
    CPUData cpu;
    RAMData ram;
    GPUData gpu;
    NetworkData network;
    DiskData disk;
    unsigned long lastUpdate;
    bool isValid;
};

#endif // TELEMETRY_H

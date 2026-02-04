#include "GPS.h"
#include <string.h>
#include <stdio.h>

GPS::GPS(HardwareSerial &uart) : _uart(uart) {
    data.valid = false;
}

void GPS::begin(uint32_t baud) {
    // ESP32 UART2: RX=18, TX=17
    _uart.begin(baud, SERIAL_8N1, 18, 17);
}

void GPS::convertToDMS(float ddmm, int &deg, int &min, float &sec) {
    deg = (int)(ddmm / 100);
    float m = ddmm - deg * 100;
    min = (int)m;
    sec = (m - min) * 60.0f;
}

bool GPS::parseRMC(char *line) {
    char status;
    float lat, lon;
    char ns, ew;

    // Bắt cả GPRMC và GNRMC
    if (sscanf(line,
        "$%*2cRMC,%*[^,],%c,%f,%c,%f,%c",
        &status, &lat, &ns, &lon, &ew) == 5)
    {
        if (status == 'A') {
            convertToDMS(lat, data.lat_deg, data.lat_min, data.lat_sec);
            convertToDMS(lon, data.lon_deg, data.lon_min, data.lon_sec);
            data.lat_dir = ns;
            data.lon_dir = ew;
            data.valid = true;
            return true;
        }
    }
    data.valid = false;
    return false;
}

GPS::Data GPS::read() {
    static char buffer[128];
    static uint8_t idx = 0;

    while (_uart.available()) {
        char c = _uart.read();

        if (c == '\n') {
            buffer[idx] = '\0';
            idx = 0;

            if (strstr(buffer, "RMC")) {
                parseRMC(buffer);
                return data;
            }
        }
        else if (idx < sizeof(buffer) - 1) {
            buffer[idx++] = c;
        }
    }
    return data;
}

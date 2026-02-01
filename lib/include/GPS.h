#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

class GPS {
public:
    struct Data {
        int lat_deg;
        int lat_min;
        float lat_sec;
        char lat_dir;

        int lon_deg;
        int lon_min;
        float lon_sec;
        char lon_dir;

        bool valid;
    };

    GPS(HardwareSerial &uart);
    void begin(uint32_t baud);
    Data read();

private:
    HardwareSerial &_uart;
    Data data;

    void convertToDMS(float ddmm, int &deg, int &min, float &sec);
    bool parseRMC(char *line);
};

#endif

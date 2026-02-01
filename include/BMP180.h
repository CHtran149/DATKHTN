#ifndef BMP180_H
#define BMP180_H

#include <Arduino.h>
#include <Wire.h>

class BMP180 {
public:
    struct Data {
        float pressure;     // Áp suất (hPa)
        float temperature;  // Nhiệt độ (°C)
        float altitude;     // Độ cao (m)
        bool valid;         // Cờ báo đọc thành công
    };

private:
    uint8_t _address;      // Địa chỉ I2C (thường là 0x77)
    Data result;

    bool _readRawData();   // Đọc dữ liệu thô từ cảm biến

public:
    BMP180(uint8_t address = 0x77);
    bool begin();          // Khởi tạo cảm biến
    Data read();           // Đọc cảm biến
};

#endif

#ifndef DHT11_H
#define DHT11_H

#include <Arduino.h>

class DHT11 {
public:
    struct Data {
        float temperature;  // Nhiệt độ (°C)
        float humidity;     // Độ ẩm (%)
        bool valid;         // Cờ báo đọc thành công
    };

private:
    int _pin;
    Data result;
    bool _readRawData(uint8_t data[5]);

public:
    DHT11(int pin);
    Data read();  // Đọc cảm biến
};

#endif

#include "BMP180.h"
#include <Adafruit_BMP085.h>

// Đối tượng thư viện Adafruit (BMP180 dùng chung BMP085)
static Adafruit_BMP085 bmp;

// --- Khởi tạo ---
BMP180::BMP180(uint8_t address) {
    _address = address;
    result = {0.0f, 0.0f, 0.0f, false};
}

// --- Khởi tạo cảm biến ---
bool BMP180::begin() {
    // I2C (Wire.begin) phải được gọi ở main
    if (!bmp.begin()) {
        return false;
    }
    return true;
}

// --- Đọc dữ liệu thô từ cảm biến ---
bool BMP180::_readRawData() {
    // Đọc áp suất (Pa → hPa)
    result.pressure = bmp.readPressure() / 100.0f;

    // Đọc nhiệt độ (°C)
    result.temperature = bmp.readTemperature();

    // Đọc độ cao (m) – mặc định theo mực nước biển
    result.altitude = bmp.readAltitude();

    return true;
}

// --- Hàm read(): đọc và trả về kết quả ---
BMP180::Data BMP180::read() {
    if (_readRawData()) {
        result.valid = true;
    } else {
        result.valid = false;
    }
    return result;
}

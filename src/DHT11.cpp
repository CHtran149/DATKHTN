#include "DHT11.h"

// --- Khởi tạo ---
DHT11::DHT11(int pin) {
    _pin = pin;
    pinMode(_pin, INPUT_PULLUP);
    result = {0, 0, false};
}

// --- Đọc dữ liệu thô từ cảm biến ---
bool DHT11::_readRawData(uint8_t data[5]) {
    uint8_t cnt = 7, idx = 0;
    for (int i = 0; i < 5; i++) data[i] = 0;

    // Gửi tín hiệu bắt đầu tới cảm biến
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delay(20); // giữ mức thấp ít nhất 18ms
    digitalWrite(_pin, HIGH);
    delayMicroseconds(30);
    pinMode(_pin, INPUT);

    // Chờ cảm biến phản hồi
    unsigned long start = micros();
    while (digitalRead(_pin) == HIGH) {
        if (micros() - start > 90) return false; // không phản hồi
    }

    // DHT kéo LOW ~80us
    start = micros();
    while (digitalRead(_pin) == LOW) {
        if (micros() - start > 90) return false;
    }

    // DHT kéo HIGH ~80us
    start = micros();
    while (digitalRead(_pin) == HIGH) {
        if (micros() - start > 90) return false;
    }

    // Đọc 40 bit dữ liệu
    for (int i = 0; i < 40; i++) {
        // Chờ tín hiệu LOW bắt đầu bit
        start = micros();
        while (digitalRead(_pin) == LOW) {
            if (micros() - start > 80) return false;
        }

        // Đo độ dài mức HIGH để phân biệt 0 hoặc 1
        unsigned long t = micros();
        while (digitalRead(_pin) == HIGH) {
            if (micros() - t > 90) break;
        }

        if ((micros() - t) > 40)
            data[idx] |= (1 << cnt);

        if (cnt == 0) {
            cnt = 7;
            idx++;
        } else cnt--;
    }

    // Kiểm tra checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    return (sum == data[4]);
}

// --- Hàm read(): đọc và trả về kết quả ---
DHT11::Data DHT11::read() {
    uint8_t data[5];
    if (_readRawData(data)) {
        result.humidity = data[0];
        result.temperature = data[2];
        result.valid = true;
    } else {
        result.valid = false;
    }
    return result;
}

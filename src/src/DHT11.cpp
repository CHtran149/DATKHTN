#include "DHT11.h"

// --- Khởi tạo ---
DHT11::DHT11(int pin) { // khởi tạo đối tượng DHT11 với chân dữ liệu được chỉ định, thiết lập chân này là INPUT_PULLUP và khởi tạo kết quả mặc định
    _pin = pin; // Chân dữ liệu của DHT11
    pinMode(_pin, INPUT_PULLUP); //  DHT11 yêu cầu chân dữ liệu phải có điện trở kéo lên (pull-up), nên ta dùng INPUT_PULLUP để kích hoạt điện trở nội của ESP32
    result = {0, 0, false}; // khởi tạo giá trị mặc định cho struct Data (nhiệt độ, độ ẩm, valid)                   
}

// --- Đọc dữ liệu thô từ cảm biến ---
bool DHT11::_readRawData(uint8_t data[5]) { // hàm đọc trực tiếp 5 byte dữ liệu tho từ DHT11
    uint8_t cnt = 7, idx = 0; // biến cnt dùng để ghi bit vào từng byte, bắt đầu từ bit 7 (MSB)
                                // idx dùng để chỉ byte hiện tại trong mảng data (0-4)
    for (int i = 0; i < 5; i++) data[i] = 0;

    // Gửi tín hiệu bắt đầu tới cảm biến
    pinMode(_pin, OUTPUT); // chuyển chân dữ liệu sang OUTPUT để gửi tín hiệu bắt đầu
    digitalWrite(_pin, LOW);// kéo chân dữ liệu xuống LOW để bắt đầu giao tiếp
    delay(20); // giữ mức thấp ít nhất 18ms
    digitalWrite(_pin, HIGH);// sau đó kéo lên HIGH để kết thúc tín hiệu bắt đầu
    delayMicroseconds(30);//đợi khoảng 20-40us để DHT11 chuẩn bị phản hồi
    pinMode(_pin, INPUT); // Chuyển chân dữ liệu về INPUT để chờ DHT11 phản hồi, lúc này chân sẽ được kéo lên bởi điện trở pull-up

    // Chờ cảm biến phản hồi
    unsigned long start = micros(); // bắt đầu đếm thời gian chờ phản hồi từ DHT11
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

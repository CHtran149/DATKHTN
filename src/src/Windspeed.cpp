#include "WindSpeed.h"

WindSpeed* WindSpeed::_instance = nullptr;

// ===== Biến debounce =====
static volatile uint32_t _lastTick = 0;   // thời điểm xung hợp lệ gần nhất (ms)

// --- Constructor ---
WindSpeed::WindSpeed(uint8_t chan_cam_bien, float duong_kinh_canh)
    : _chan_cam_bien(chan_cam_bien),
      _duong_kinh_canh(duong_kinh_canh),
      _dem_xung(0)
{
    _instance = this;
}

// --- Hàm ngắt Hall ---
void IRAM_ATTR WindSpeed::_ngatCamBien() {
    if (!_instance) return;

    uint32_t now = millis();

    // Debounce 10 ms: 1 vòng quay = 1 xung
    if (now - _lastTick > 10) {
        _instance->_dem_xung++;
        _lastTick = now;
    }
}

// --- Khởi tạo ---
void WindSpeed::begin() {
    pinMode(_chan_cam_bien, INPUT_PULLUP);
    attachInterrupt(
        digitalPinToInterrupt(_chan_cam_bien),
        _ngatCamBien,
        FALLING
    );
}

WindSpeed::Data WindSpeed::read(uint32_t chu_ky_do_ms) {
    static uint32_t lastTime = 0;
    Data kq;
    kq.valid = false;

    if (millis() - lastTime >= chu_ky_do_ms) {
        noInterrupts();
        uint32_t so_xung = _dem_xung;
        _dem_xung = 0;
        interrupts();

        float thoi_gian_s = chu_ky_do_ms / 1000.0f;
        float chu_vi = PI * _duong_kinh_canh;
        float quang_duong = so_xung * chu_vi;

        kq.toc_do_gio = quang_duong / thoi_gian_s;
        kq.so_xung = so_xung;
        kq.valid = true;

        lastTime = millis();
    }

    return kq;
}

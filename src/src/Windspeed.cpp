#include "WindSpeed.h"

WindSpeed* WindSpeed::_instance = nullptr;

// --- Constructor ---
WindSpeed::WindSpeed(uint8_t chan_cam_bien, float duong_kinh_canh)
    : _chan_cam_bien(chan_cam_bien),
      _duong_kinh_canh(duong_kinh_canh),
      _dem_xung(0)
{
    _instance = this;
}

// --- Hàm ngắt ---
void IRAM_ATTR WindSpeed::_ngatCamBien() {
    if (_instance) {
        _instance->_dem_xung++;
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

// --- Đọc và tính tốc độ gió ---
WindSpeed::Data WindSpeed::read(uint32_t thoi_gian_do_ms) {
    Data ket_qua;
    _dem_xung = 0;

    delay(thoi_gian_do_ms);

    noInterrupts();
    uint32_t so_xung = _dem_xung;
    interrupts();

    float thoi_gian_s = thoi_gian_do_ms / 1000.0f;
    float chu_vi = PI * _duong_kinh_canh;
    float quang_duong = so_xung * chu_vi;

    ket_qua.toc_do_gio = quang_duong / thoi_gian_s;
    ket_qua.so_xung = so_xung;
    ket_qua.valid = true;

    return ket_qua;
}

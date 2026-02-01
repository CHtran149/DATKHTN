#include "Rainfall.h"

volatile unsigned long Rainfall::_demXung = 0;

// ===== Ngắt Hall =====
void IRAM_ATTR Rainfall::ngatDemXung() {
    _demXung++;
}

// ===== Constructor =====
Rainfall::Rainfall(int chanHall,
                   float duongKinhPhieu_mm,
                   float theTichGau_mm3,
                   unsigned long thoiGianDo_ms) {
    _chanHall = chanHall;
    _duongKinhPhieu = duongKinhPhieu_mm;
    _theTichGau = theTichGau_mm3;
    _thoiGianDo = thoiGianDo_ms;
    _thoiGianTruoc = 0;
}

// ===== Begin =====
void Rainfall::begin() {
    pinMode(_chanHall, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_chanHall), ngatDemXung, FALLING);
}

// ===== Tính diện tích phễu =====
float Rainfall::tinhDienTichPhieu() {
    return 3.1415926 * (_duongKinhPhieu / 2.0) * (_duongKinhPhieu / 2.0);
}

// ===== Tính hệ số K =====
float Rainfall::tinhHeSoK(float dienTich) {
    return _theTichGau / dienTich;
}

// ===== Read =====
Rainfall::Data Rainfall::read() {
    Data kq;
    kq.valid = false;

    if (millis() - _thoiGianTruoc >= _thoiGianDo) {
        noInterrupts();
        unsigned long xung = _demXung;
        _demXung = 0;
        interrupts();

        float S = tinhDienTichPhieu();
        float K = tinhHeSoK(S);

        kq.soXung = xung;
        kq.dienTichPhieu = S;
        kq.heSoK = K;
        kq.luongMua = xung * K;
        kq.cuongDoMua = kq.luongMua * (3600000.0 / _thoiGianDo);
        kq.valid = true;

        _thoiGianTruoc = millis();
    }

    return kq;
}

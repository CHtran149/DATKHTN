#ifndef RAINFALL_H
#define RAINFALL_H

#include <Arduino.h>

class Rainfall {
public:
    struct Data {
        unsigned long soXung;
        float dienTichPhieu;   // mm2
        float heSoK;           // mm/xung
        float luongMua;        // mm
        float cuongDoMua;      // mm/h
        bool valid;
    };

    Rainfall(int chanHall,
             float duongKinhPhieu_mm,
             float theTichGau_mm3,
             unsigned long thoiGianDo_ms);

    void begin();
    Data read();

private:
    int _chanHall;
    float _duongKinhPhieu;
    float _theTichGau;
    unsigned long _thoiGianDo;

    float tinhDienTichPhieu();
    float tinhHeSoK(float dienTich);

    static volatile unsigned long _demXung;
    static void IRAM_ATTR ngatDemXung();

    unsigned long _thoiGianTruoc;
};

#endif

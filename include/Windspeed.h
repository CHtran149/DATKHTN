#ifndef WIND_SPEED_H
#define WIND_SPEED_H

#include <Arduino.h>

class WindSpeed {
public:
    struct Data {
        float toc_do_gio;     // m/s
        uint32_t so_xung;     // số xung đo được
        bool valid;
    };

    WindSpeed(uint8_t chan_cam_bien, float duong_kinh_canh);

    void begin();
    Data read(uint32_t thoi_gian_do_ms);

private:
    uint8_t _chan_cam_bien;
    float _duong_kinh_canh;      // mét
    volatile uint32_t _dem_xung;

    static void IRAM_ATTR _ngatCamBien();
    static WindSpeed* _instance;
};

#endif

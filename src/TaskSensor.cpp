#include "TaskSensor.h"

// BẮT BUỘC: Dùng extern để báo cho compiler biết các Object này 
// đã được tạo (khởi tạo) ở file main.cpp
#include "DHT11.h"
#include "BMP180.h"
#include "WindSpeed.h"
#include "Rainfall.h"
#include "GPS.h"

extern DHT11 dht;
extern BMP180 bmp;
extern WindSpeed wind;
extern Rainfall rain;
extern GPS gps;
extern QueueHandle_t Queue_SensorRaw;

void Task_Sensor(void *pvParameters) {
    Serial.println("[Task_Sensor] Started");

    while (1) {
        // CỰC KỲ QUAN TRỌNG: Khởi tạo struct bằng {0} để xóa sạch RAM rác
        SensorRaw_t data = {0}; 

        /* ---- Đọc DHT11 ---- */
        auto dhtData = dht.read();
        if (dhtData.valid) {
            data.temperature = dhtData.temperature;
            data.humidity    = dhtData.humidity;
        } else {
            // Gán giá trị an toàn để Task_Processing không bị chia cho 0 hoặc NaN
            data.temperature = 25.0f; 
            data.humidity = 50.0f;
        }

        /* ---- Đọc BMP180 ---- */
        auto bmpData = bmp.read();
        if (bmpData.valid) {
            data.pressure = bmpData.pressure;
            data.altitude = bmpData.altitude;
        } else {
            data.pressure = 1013.25f;
        }

        /* ---- Đọc Wind & Rain ---- */
        auto windData = wind.read(1000);
        data.wind_speed = windData.valid ? windData.toc_do_gio : 0.0f;

        auto rainData = rain.read();
        data.rainfall = rainData.valid ? rainData.luongMua : 0.0f;

        /* ---- Đọc GPS ---- */
        auto gpsData = gps.read();
        if (gpsData.valid) {
            data.lat_deg = gpsData.lat_deg;
            data.lat_min = gpsData.lat_min;
            data.lat_sec = gpsData.lat_sec;
            data.lat_dir = gpsData.lat_dir;
            data.lon_deg = gpsData.lon_deg;
            data.lon_min = gpsData.lon_min;
            data.lon_sec = gpsData.lon_sec;
            data.lon_dir = gpsData.lon_dir;

            
            Serial.print("Lat: ");
            Serial.print(gpsData.lat_deg);
            Serial.print("°");
            Serial.print(gpsData.lat_min);
            Serial.print("'");
            Serial.print(gpsData.lat_sec);
            Serial.print(gpsData.lat_dir);

            Serial.print(" | Lon: ");
            Serial.print(gpsData.lon_deg);
            Serial.print("°");
            Serial.print(gpsData.lon_min);
            Serial.print("'");
            Serial.print(gpsData.lon_sec);
            Serial.println(gpsData.lon_dir);
        }

        data.timestamp = millis();

        // Gửi vào Queue
        if (Queue_SensorRaw != NULL) {
            xQueueSend(Queue_SensorRaw, &data, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
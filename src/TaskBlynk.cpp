
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL62V_f1A6O"
#define BLYNK_TEMPLATE_NAME "Weather"
#define BLYNK_DEVICE_NAME "Weather"
#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "TaskBlynk.h"
#include "SensorTypes.h"
#include "../include/secrets.h"

// WiFi & Blynk credentials - replace with your real values


char BLYNK_AUTH[] = "quJvZhMQWTpMf1iH46-NKmk87DaGqUf6";

// Virtual pin mapping (adjust in Blynk app)
#define VPIN_T_AVG  V1 // Temperature average virtual pin
#define VPIN_H_AVG  V2 // Humidity average virtual pin
#define VPIN_P_AVG  V3 // Pressure average virtual pin
#define VPIN_W_AVG  V4 // Wind average virtual pin
#define VPIN_R_AVG  V5 // Rain average virtual pin
#define VPIN_GPS_TXT V6   // hien thi chung vi do + kinh do



void Task_Blynk(void *pvParameters) {
    ProcessedSensor_t data;

    Serial.println("[Blynk] Connecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println("\n[Blynk] WiFi connected");

    // Dùng begin để tự động reconnect
    Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PASS);

    while (1) {
        Blynk.run();
         String gpsText = "Vi do: " + String(data.latitude, 6) +
                                 " | Kinh do: " + String(data.longitude, 6);
        if (Queue_Data_Blynk != NULL) {
            if (xQueueReceive(Queue_Data_Blynk, &data, 0) == pdTRUE) {
                // Gửi dữ liệu đồng bộ lên V1–V5
                Blynk.virtualWrite(VPIN_T_AVG, data.t_avg);
                Blynk.virtualWrite(VPIN_H_AVG, data.h_avg);
                Blynk.virtualWrite(VPIN_P_AVG, data.p_avg);
                Blynk.virtualWrite(VPIN_W_AVG, data.w_avg);
                Blynk.virtualWrite(VPIN_R_AVG, data.r_avg);
                Blynk.virtualWrite(VPIN_GPS_TXT, gpsText);
              //  Serial.printf("[Blynk] Sent T=%.2f H=%.2f P=%.2f W=%.2f R=%.2f\n",
                           //   data.t_avg, data.h_avg, data.p_avg, data.w_avg, data.r_avg);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // delay vừa phải
    }
}
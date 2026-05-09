
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

// TEMP_WARN

BLYNK_WRITE(V10) {
    float val = param.asFloat();
    Config_t cfgMsg;

    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            cfgMsg = g_config;          // copy an toàn
            cfgMsg.temp_warn = val;
            xSemaphoreGive(Config_Mutex);
        }
    }

    if (Queue_Config != NULL) {
        xQueueSend(Queue_Config, &cfgMsg, 0);
        Serial.printf("[Blynk] Updated TEMP_WARN=%.1f\n", val);
    }
}

// TEMP_DANGER
BLYNK_WRITE(V11) {
    float val = param.asFloat();
    Config_t cfgMsg;

    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            cfgMsg = g_config;          // copy an toàn
            cfgMsg.temp_danger = val;
            xSemaphoreGive(Config_Mutex);
        }
    }

    if (Queue_Config != NULL) {
        xQueueSend(Queue_Config, &cfgMsg, 0);
        Serial.printf("[Blynk] Updated TEMP_DANGER=%.1f\n", val);
    }
}

// WIND_DANGER
BLYNK_WRITE(V12) {
    float val = param.asFloat();
    Config_t cfgMsg;

    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            cfgMsg = g_config;          // copy an toàn
            cfgMsg.wind_danger = val;
            xSemaphoreGive(Config_Mutex);
        }
    }

    if (Queue_Config != NULL) {
        xQueueSend(Queue_Config, &cfgMsg, 0);
        Serial.printf("[Blynk] Updated WIND_DANGER=%.1f\n", val);
    }
}

// RAIN_DANGER
BLYNK_WRITE(V13) {
    float val = param.asFloat();
    Config_t cfgMsg;

    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            cfgMsg = g_config;          // copy an toàn
            cfgMsg.rain_danger = val;
            xSemaphoreGive(Config_Mutex);
        }
    }

    if (Queue_Config != NULL) {
        xQueueSend(Queue_Config, &cfgMsg, 0);
        Serial.printf("[Blynk] Updated RAIN_DANGER=%.1f\n", val);
    }
}

// SAMPLE_INTERVAL
BLYNK_WRITE(V14) {
    int val = param.asInt();
    if (val < 100) val = 100; // đảm bảo không bằng 0

    Config_t cfgMsg;

    // Bảo vệ khi đọc g_config bằng mutex
    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            cfgMsg = g_config; // copy an toàn
            xSemaphoreGive(Config_Mutex);
        }
    }

    // Cập nhật giá trị mới
    cfgMsg.sample_interval_ms = val;

    if (Queue_Config != NULL) {
        xQueueSend(Queue_Config, &cfgMsg, 0);
        Serial.printf("[Blynk] Updated SAMPLE_INTERVAL=%d ms\n", val);
    }
}



void Task_Blynk(void *pvParameters) {
    ProcessedSensor_t data;
    unsigned long lastSync = 0;

    Serial.println("[Blynk] Connecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println("\n[Blynk] WiFi connected");

    // Dùng begin để tự động reconnect
    Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PASS);

    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            Blynk.virtualWrite(V10, g_config.temp_warn);
            Blynk.virtualWrite(V11, g_config.temp_danger);
            Blynk.virtualWrite(V12, g_config.wind_danger);
            Blynk.virtualWrite(V13, g_config.rain_danger);
            Blynk.virtualWrite(V14, g_config.sample_interval_ms);
            xSemaphoreGive(Config_Mutex);
        }
    }

    while (1) {
        Blynk.run();
        if (Queue_Data_Blynk != NULL) {
            if (xQueueReceive(Queue_Data_Blynk, &data, 0) == pdTRUE) {

                // String gpsText = "Vi do: " + String(data.latitude, 6) +
                //                  " | Kinh do: " + String(data.longitude, 6);
                String gpsLink = "https://www.google.com/maps?q=" + 
                 String(data.latitude, 6) + "," + 
                 String(data.longitude, 6);
                // Gửi dữ liệu đồng bộ lên V1–V5
                Blynk.virtualWrite(VPIN_T_AVG, data.t_avg);
                Blynk.virtualWrite(VPIN_H_AVG, data.h_avg);
                Blynk.virtualWrite(VPIN_P_AVG, data.p_avg);
                Blynk.virtualWrite(VPIN_W_AVG, data.w_avg);
                Blynk.virtualWrite(VPIN_R_AVG, data.r_avg);
                Blynk.virtualWrite(VPIN_GPS_TXT, gpsLink);
              //  Serial.printf("[Blynk] Sent T=%.2f H=%.2f P=%.2f W=%.2f R=%.2f\n",
                           //   data.t_avg, data.h_avg, data.p_avg, data.w_avg, data.r_avg);
            }
        }

                // Đồng bộ slider mỗi 5 giây
        if (millis() - lastSync > 5000) {
            if (Config_Mutex != NULL) {
                if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    Blynk.virtualWrite(V10, g_config.temp_warn);
                    Blynk.virtualWrite(V11, g_config.temp_danger);
                    Blynk.virtualWrite(V12, g_config.wind_danger);
                    Blynk.virtualWrite(V13, g_config.rain_danger);
                    Blynk.virtualWrite(V14, g_config.sample_interval_ms);
                    xSemaphoreGive(Config_Mutex);
                }
            }
            lastSync = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // delay vừa phải
    }
}
#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "TaskBlynk.h"
#include "SensorTypes.h"

// WiFi & Blynk credentials - replace with your real values
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASS";
char BLYNK_AUTH[] = "YOUR_BLYNK_AUTH_TOKEN";

// Virtual pin mapping (adjust in Blynk app)
#define VPIN_T_AVG  V1
#define VPIN_H_AVG  V2
#define VPIN_P_AVG  V3
#define VPIN_W_AVG  V4
#define VPIN_R_AVG  V5
#define VPIN_STATE  V10

void Task_Blynk(void *pvParameters) {
    ProcessedSensor_t data;

    Serial.println("[Blynk] Connecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries++ < 40) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) Serial.println("[Blynk] WiFi connected");

    Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PASS);
    Serial.println("[Blynk] Started");

    while (1) {
        Blynk.run();

        // Receive processed data and write to app (dedicated Blynk queue)
        if (Queue_Data_Blynk != NULL) {
            if (xQueueReceive(Queue_Data_Blynk, &data, pdMS_TO_TICKS(500)) == pdTRUE) {
                Blynk.virtualWrite(VPIN_T_AVG, data.t_avg);
                Blynk.virtualWrite(VPIN_H_AVG, data.h_avg);
                Blynk.virtualWrite(VPIN_P_AVG, data.p_avg);
                Blynk.virtualWrite(VPIN_W_AVG, data.w_avg);
                Blynk.virtualWrite(VPIN_R_AVG, data.r_avg);
            }
        }

        // Read event bits (non-blocking) and update status widget
        if (Event_Weather != NULL) {
            EventBits_t bits = xEventGroupGetBits(Event_Weather);
            if (bits & (1 << 1)) Blynk.virtualWrite(VPIN_STATE, "DANGER");
            else if (bits & (1 << 0)) Blynk.virtualWrite(VPIN_STATE, "WARNING");
            else if (bits & (1 << 2)) Blynk.virtualWrite(VPIN_STATE, "ERROR");
            else Blynk.virtualWrite(VPIN_STATE, "NORMAL");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// Blynk handlers to receive configuration from app and forward to Queue_Config
// Map widget V20..V24 to config fields in Config_t
BLYNK_WRITE(V20) { // sample interval (ms)
    if (Queue_Config == NULL) return;
    Config_t cfg;
    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            cfg = g_config; // copy
            xSemaphoreGive(Config_Mutex);
        } else return;
    } else return;

    cfg.sample_interval_ms = param.asInt();
    xQueueSend(Queue_Config, &cfg, 0);
}

BLYNK_WRITE(V21) { // temp_warn
    if (Queue_Config == NULL) return;
    Config_t cfg;
    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            cfg = g_config;
            xSemaphoreGive(Config_Mutex);
        } else return;
    } else return;

    cfg.temp_warn = param.asFloat();
    xQueueSend(Queue_Config, &cfg, 0);
}

BLYNK_WRITE(V22) { // temp_danger
    if (Queue_Config == NULL) return;
    Config_t cfg;
    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            cfg = g_config;
            xSemaphoreGive(Config_Mutex);
        } else return;
    } else return;

    cfg.temp_danger = param.asFloat();
    xQueueSend(Queue_Config, &cfg, 0);
}

BLYNK_WRITE(V23) { // humi_warn
    if (Queue_Config == NULL) return;
    Config_t cfg;
    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            cfg = g_config;
            xSemaphoreGive(Config_Mutex);
        } else return;
    } else return;

    cfg.humi_warn = param.asFloat();
    xQueueSend(Queue_Config, &cfg, 0);
}

BLYNK_WRITE(V24) { // humi_danger
    if (Queue_Config == NULL) return;
    Config_t cfg;
    if (Config_Mutex != NULL) {
        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            cfg = g_config;
            xSemaphoreGive(Config_Mutex);
        } else return;
    } else return;

    cfg.humi_danger = param.asFloat();
    xQueueSend(Queue_Config, &cfg, 0);
}

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "TaskCloud.h"
#include "SensorTypes.h"

// WiFi and server configuration - replace with real values
#include "../include/secrets.h"
const char* SERVER_URL = "https://script.google.com/macros/s/AKfycbzvZ9byc_0WE4hj_wJGp2FeTVgOQZnaqoZ0LWTfRrdlezDByqva4cQ0qPsMccSVdQC9/exec";

void Task_Cloud(void *pvParameters) {
    ProcessedSensor_t data;

    Serial.println("[Cloud] Connecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries++ < 60) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) Serial.println("[Cloud] WiFi connected");

    while (1) {
        if (Queue_Data_Cloud != NULL) {
            if (xQueueReceive(Queue_Data_Cloud, &data, portMAX_DELAY) == pdTRUE) {
                HTTPClient http;
                http.begin(SERVER_URL);
                http.addHeader("Content-Type", "application/json");

                String payload = "{";
                payload += "\"temperature\":" + String(data.t_avg, 2) + ",";
                payload += "\"humidity\":" + String(data.h_avg, 2) + ",";
                payload += "\"pressure\":" + String(data.p_avg, 2) + ",";
                payload += "\"wind\":" + String(data.w_avg, 2) + ",";
                payload += "\"rain\":" + String(data.r_avg, 2) + ",";
                payload += "\"heat_index\":" + String(data.heat_index_c, 2) + ",";
                payload += "\"timestamp\":" + String(data.timestamp);
                payload += "}";

                // Debug: print JSON payload before POSTing
                Serial.print("[Cloud] Payload: ");
                Serial.println(payload);

                int httpResponseCode = http.POST(payload);
                if (httpResponseCode > 0) {
                    Serial.printf("[Cloud] Data uploaded, response: %d\n", httpResponseCode);
                } else {
                    Serial.printf("[Cloud] Upload failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
                }
                http.end();
            }
        }
    }
}

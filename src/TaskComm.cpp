#include <Arduino.h>
#include "TaskComm.h"
#include "SensorTypes.h"

// Simple retry parameters
static const int MAX_RETRY = 3;
static const TickType_t RETRY_DELAY = pdMS_TO_TICKS(3000);

void sendAlertViaSMS(const Alert_t &alert) {
    // Placeholder: replace with real GSM send if available
    Serial.print("[Comm] Sending SMS alert: ");
    Serial.println(alert.message);
}

void Task_Comm(void *pvParameters) {
    Alert_t alert;

    while (1) {
        // Prioritize queued alerts
        if (xQueueReceive(Queue_Alert, &alert, pdMS_TO_TICKS(1000)) == pdTRUE) {
            int attempts = 0;
            bool ok = false;
            while (attempts < MAX_RETRY && !ok) {
                attempts++;
                sendAlertViaSMS(alert);
                // In this placeholder assume send always OK; in real code check modem response
                ok = true;
                if (!ok) vTaskDelay(RETRY_DELAY);
            }
            if (!ok) {
                Serial.println("[Comm] Failed to send alert after retries");
            }
            continue;
        }

        // Otherwise, check event bits for real-time handling
        if (Event_Weather != NULL) {
            EventBits_t bits = xEventGroupWaitBits(Event_Weather, 0x07, pdTRUE, pdFALSE, pdMS_TO_TICKS(500));
            if (bits & (1 << 1)) {
                Serial.println("[Comm] Real-time: DANGER event detected");
            } else if (bits & (1 << 0)) {
                Serial.println("[Comm] Real-time: WARNING event detected");
            } else if (bits & (1 << 2)) {
                Serial.println("[Comm] Real-time: ERROR event detected");
            }
        }
    }
}

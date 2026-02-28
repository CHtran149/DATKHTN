#include <Arduino.h>
#include "TaskComm.h"
#include "SensorTypes.h"
#include "Modem.h"

// List of phone numbers to notify (edit these)
// Thêm số điện thoại thực tế ở đây (định dạng E.164)
static const char *alertPhones[] = {
    "+84901234567",
    "+84987654321"
};
static const int alertPhoneCount = sizeof(alertPhones) / sizeof(alertPhones[0]);

// Simple retry parameters
static const int MAX_RETRY = 3;
static const TickType_t RETRY_DELAY = pdMS_TO_TICKS(3000);

// Gửi SMS tới tất cả số trong danh sách. Trả về true nếu tất cả gửi thành công.
bool sendAlertViaSMS(const Alert_t &alert) {
    Serial.print("[Comm] Sending SMS alert: ");
    Serial.println(alert.message);

    bool all_ok = true;
    for (int i = 0; i < alertPhoneCount; ++i) {
        const char *phone = alertPhones[i];
        bool ok = modem.sendSMS(phone, alert.message);
        if (!ok) {
            Serial.print("[Comm] sendSMS failed for "); Serial.println(phone);
            all_ok = false;
        }
        // small delay between sends
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return all_ok;
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
                ok = sendAlertViaSMS(alert);
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

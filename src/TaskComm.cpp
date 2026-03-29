#include <Arduino.h>
#include "TaskComm.h"
#include "SensorTypes.h"
#include "Modem.h"

// Danh sách số điện thoại nhận cảnh báo
static const char *alertPhones[] = {
    "+84901234567",
    "+84987654321"
};
static const int alertPhoneCount = sizeof(alertPhones) / sizeof(alertPhones[0]);

static const int MAX_RETRY = 3;
static const TickType_t RETRY_DELAY = pdMS_TO_TICKS(3000);

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
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return all_ok;
}

// Send SMS with retries (helper modeled after user's sample sendWithRetries)
static bool sendWithRetries(const char *phone, const char *msg, int maxAttempts) {
    int attempts = 0;
    bool ok = false;
    while (attempts < maxAttempts && !ok) {
        attempts++;
        ok = modem.sendSMS(phone, msg);
        if (!ok) {
            Serial.printf("[Comm] sendWithRetries attempt %d failed for %s\n", attempts, phone);
            vTaskDelay(RETRY_DELAY);
        }
    }
    return ok;
}

void Task_Comm(void *pvParameters) {
    Alert_t alert;
    ProcessedSensor_t latest; // dữ liệu mới nhất để trả lời SMS

    while (1) {
        // 1. Ưu tiên xử lý cảnh báo từ Queue_Alert
        if (xQueueReceive(Queue_Alert, &alert, pdMS_TO_TICKS(500)) == pdTRUE) {
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

        // 2. Kiểm tra SMS đến (yêu cầu dữ liệu)
        String sender, content;
        while (modem.readSMS(sender, content)) {
            Serial.printf("[Comm] SMS received from %s: %s\n", sender.c_str(), content.c_str());

            if (content.equalsIgnoreCase("REQUEST")) {
                char msgbuf[160];

                if (Queue_Data_Blynk == NULL) {
                    Serial.println("[Comm] Queue_Data_Blynk is NULL, cannot build report");
                    snprintf(msgbuf, sizeof(msgbuf), "No data available");
                } else if (xQueuePeek(Queue_Data_Blynk, &latest, pdMS_TO_TICKS(200)) != pdTRUE) {
                    Serial.println("[Comm] No data available in Queue_Data_Blynk to reply");
                    snprintf(msgbuf, sizeof(msgbuf), "No data available");
                } else {
                    EventBits_t bits = xEventGroupGetBits(Event_Weather);
                    const char *state = "NORMAL";
                    if (bits & (1 << 1)) state = "DANGER";
                    else if (bits & (1 << 0)) state = "WARNING";
                    else if (bits & (1 << 2)) state = "ERROR";

                    snprintf(msgbuf, sizeof(msgbuf),
                        "T=%.1fC H=%.1f%% P=%.1fhPa W=%.1fm/s R=%.1fmm State=%s",
                        latest.t_avg, latest.h_avg, latest.p_avg,
                        latest.w_avg, latest.r_avg, state);
                }

                Serial.printf("[Comm] Sending report to %s\n", sender.c_str());
                bool s = sendWithRetries(sender.c_str(), msgbuf, MAX_RETRY);
                if (s) Serial.println("[Comm] Report sent");
                else Serial.println("[Comm] Failed to send report after retries");
            } else {
                Serial.println("[Comm] SMS ignored: not REQUEST");
            }
        }

        // 3. Nếu không có SMS, kiểm tra Event_Weather realtime
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
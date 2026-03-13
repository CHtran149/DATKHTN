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
        if (modem.readSMS(sender, content)) {
            Serial.printf("[Comm] SMS received from %s: %s\n", sender.c_str(), content.c_str());
            if (content.indexOf("REQ") >= 0) {
                // Lấy dữ liệu mới nhất từ Queue_Data_Blynk (peek)
                if (Queue_Data_Blynk != NULL) {
                    if (xQueuePeek(Queue_Data_Blynk, &latest, 0) == pdTRUE) {
                        // Lấy trạng thái hệ thống từ Event_Weather
                        EventBits_t bits = xEventGroupGetBits(Event_Weather);
                        const char *state = "NORMAL";
                        if (bits & (1 << 1)) state = "DANGER";
                        else if (bits & (1 << 0)) state = "WARNING";
                        else if (bits & (1 << 2)) state = "ERROR";

                        char reply[160];
                        snprintf(reply, sizeof(reply),
                            "T=%.1fC H=%.1f%% P=%.1fhPa W=%.1fm/s R=%.1fmm State=%s",
                            latest.t_avg, latest.h_avg, latest.p_avg,
                            latest.w_avg, latest.r_avg, state);

                        bool success = modem.sendSMS(sender.c_str(), reply);
                        if (success) {
                            Serial.println("[Comm] Replied with weather data");
                        } else {
                            Serial.println("[Comm] Failed to reply SMS");
                        }
                    }
                }
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
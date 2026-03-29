#include <Arduino.h>
#include "TaskComm.h"
#include "SensorTypes.h"
#include "Modem.h"

// ===== CONFIG =====
static const char *alertPhones[] = {
    "+84327161236",
    "+84977435825"
};
static const int alertPhoneCount = sizeof(alertPhones) / sizeof(alertPhones[0]);

static const int MAX_RETRY = 3;
static const TickType_t RETRY_DELAY = pdMS_TO_TICKS(3000);

// ===== RETRY SEND SMS =====
static bool sendWithRetries(const char *phone, const char *msg, int maxAttempts)
{
    int attempts = 0;
    bool ok = false;

    while (attempts < maxAttempts && !ok)
    {
        attempts++;
        Serial.printf("[Comm] Sending SMS to %s (try %d)\n", phone, attempts);

        ok = modem.sendSMS(phone, msg);

        if (!ok)
        {
            Serial.println("[Comm] Send failed -> retry...");
            vTaskDelay(RETRY_DELAY);
        }
    }

    return ok;
}

// ===== TASK =====
void Task_Comm(void *pvParameters)
{
    (void)pvParameters;

    Serial.println("[Comm] Task started");

    unsigned long lastAlert = 0;
    unsigned long lastPeriodicSend = 0;


    Alert_t alert;
    ProcessedSensor_t latest = {0};

    bool hasData = false;  

    char msgbuf[180];

    for (;;)
    {
        unsigned long now = millis();

        // =====================================================
        // 1. NHẬN DATA TỪ PROCESSING
        // =====================================================
        if (Queue_Data_Comm != NULL)
        {
            if (xQueueReceive(Queue_Data_Comm, &latest, pdMS_TO_TICKS(1000)) == pdTRUE)
            {
                hasData = true;

                Serial.printf("[Comm] Data Updated: T=%.1f H=%.1f\n",
                              latest.t_avg, latest.h_avg);
            }
        }
        else
        {
            Serial.println("[Comm] ERROR: Queue_Data_Comm NULL");
        }

        // =====================================================
        // 2. ALERT SMS
        // =====================================================
        if (xQueueReceive(Queue_Alert, &alert, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            Serial.println("[Comm] ALERT RECEIVED");

            if (now - lastAlert >= 30000)
            {
                Serial.println("[Comm] Sending ALERT SMS...");

                for (int i = 0; i < alertPhoneCount; i++)
                {
                    bool ok = sendWithRetries(alertPhones[i], alert.message, MAX_RETRY);

                    if (ok) Serial.println("[Comm] Alert OK");
                    else Serial.println("[Comm] Alert FAIL");

                    vTaskDelay(pdMS_TO_TICKS(500));
                }

                lastAlert = now;
            }
            else
            {
                Serial.println("[Comm] Alert cooldown...");
            }
        }

        // =====================================================
        // 3. NHẬN SMS REQUEST
        // =====================================================
        String sender, content;

        Serial.println("[Comm] Checking SMS...");

        while (modem.readSMS(sender, content))
        {
            Serial.println("====== SMS RECEIVED ======");
            Serial.println(sender);
            Serial.println(content);

            // ⭐ FIX QUAN TRỌNG
            content.trim();

            Serial.printf("[Comm] After trim: '%s'\n", content.c_str());

            if (content.equalsIgnoreCase("REQUEST") || content.equalsIgnoreCase("REQ"))
            {
                Serial.println("[Comm] REQUEST detected");

                // ===== TẠO NỘI DUNG =====
                if (!hasData)
                {
                    snprintf(msgbuf, sizeof(msgbuf), "No sensor data");
                }
                else
                {
                    snprintf(msgbuf, sizeof(msgbuf),
                             "T=%.1fC H=%.1f%% P=%.1fhPa W=%.1fm/s R=%.1fmm Lat=%.5f Lon=%.5f",
                             latest.t_avg,
                             latest.h_avg,
                             latest.p_avg,
                             latest.w_avg,
                             latest.r_avg,
                             latest.latitude,
                             latest.longitude);
                }

                Serial.printf("[Comm] Reply to %s\n", sender.c_str());

                bool ok = sendWithRetries(sender.c_str(), msgbuf, MAX_RETRY);

                if (ok) Serial.println("[Comm] Reply OK");
                else Serial.println("[Comm] Reply FAIL");
            }
            else
            {
                Serial.println("[Comm] SMS ignored");
            }
        }

        // =====================================================
        // 4. GỬI DỮ LIỆU ĐỊNH KỲ 5 PHÚT
        // =====================================================
        if (hasData && (now - lastPeriodicSend >= 300000)) { // 300000 ms = 5 phút
            snprintf(msgbuf, sizeof(msgbuf),
                    "T=%.1fC H=%.1f%% P=%.1fhPa W=%.1fm/s R=%.1fmm Lat=%.5f Lon=%.5f",
                    latest.t_avg,
                    latest.h_avg,
                    latest.p_avg,
                    latest.w_avg,
                    latest.r_avg,
                    latest.latitude,
                    latest.longitude);

            Serial.println("[Comm] Sending periodic data SMS...");

            for (int i = 0; i < alertPhoneCount; i++) {
                bool ok = sendWithRetries(alertPhones[i], msgbuf, MAX_RETRY);
                if (ok) Serial.println("[Comm] Periodic SMS OK");
                else Serial.println("[Comm] Periodic SMS FAIL");
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            lastPeriodicSend = now;
        }

        Serial.println("[Comm] Loop done\n");

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
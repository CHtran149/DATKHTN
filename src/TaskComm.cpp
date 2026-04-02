#include <Arduino.h>
#include "TaskComm.h"
#include "SensorTypes.h"
#include "Modem.h"

// ===== CONFIG =====
static const char *alertPhones[] = {
    "+84327161236",
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

// ===== TASK CHÍNH =====
void Task_Comm(void *pvParameters)
{
    (void)pvParameters;
    Serial.println("[Comm] Task started");

    // --- BƯỚC KHỞI TẠO QUAN TRỌNG (ĐÃ TEST THÀNH CÔNG) ---
    // Đợi module SIM ổn định và bắt Baudrate 115200
    vTaskDelay(pdMS_TO_TICKS(5000)); 
    Serial.println("[Comm] Initializing Modem settings...");
    for(int i = 0; i < 5; i++) {
        Serial2.println("AT"); 
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    Serial2.println("ATZ");  // Reset về mặc định
    vTaskDelay(pdMS_TO_TICKS(1000));
    Serial2.println("ATE1"); // Bật Echo để quan sát phản hồi
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Gửi tin nhắn thông báo hệ thống trực tuyến (Tùy chọn)
    //sendWithRetries(alertPhones[0], "TRAM QUAN TRAC PTIT: He thong da Online!", 2);

    unsigned long lastAlert = 0;
    unsigned long lastPeriodicSend = 0;

    Alert_t alert;
    ProcessedSensor_t latest = {0};
    bool hasData = false;  
    char msgbuf[400];

    for (;;)
    {
        unsigned long now = millis();

        // =====================================================
        // 1. NHẬN DATA TỪ PROCESSING (Để gửi khi có REQ)
        // =====================================================
        if (Queue_Data_Comm != NULL)
        {
            if (xQueueReceive(Queue_Data_Comm, &latest, 0) == pdTRUE) // Đọc không đợi để loop mượt
            {
                hasData = true;
                Serial.printf("[Comm] Data Updated: T=%.1f H=%.1f P=%.1f\n",
                              latest.t_avg, latest.h_avg, latest.p_avg);
            }
        }

        // =====================================================
        // 2. ALERT SMS (Xử lý tin nhắn khẩn cấp)
        // =====================================================
        if (xQueueReceive(Queue_Alert, &alert, 0) == pdTRUE)
        {
            Serial.println("[Comm] ALERT RECEIVED FROM QUEUE");
            if (now - lastAlert >= 30000) // Cooldown 30s
            {
                for (int i = 0; i < alertPhoneCount; i++)
                {
                    sendWithRetries(alertPhones[i], alert.message, MAX_RETRY);
                }
                lastAlert = now;
            }
            else {
                Serial.println("[Comm] Alert cooldown active...");
            }
        }

        // =====================================================
        // 3. NHẬN SMS REQUEST (PHẢN HỒI 2 CHIỀU)
        // =====================================================
        String sender, content;
        // Kiểm tra xem có tin nhắn đến không
        while (modem.readSMS(sender, content))
        {
            Serial.println("====== SMS RECEIVED ======");
            Serial.println("From: " + sender);
            Serial.println("Content: " + content);

            content.trim();
            if (content.equalsIgnoreCase("REQUEST") || content.equalsIgnoreCase("REQ"))
            {
                Serial.println("[Comm] REQUEST detected -> Preparing Response");

                if (!hasData) {
                    snprintf(msgbuf, sizeof(msgbuf),
                        "No sensor data | Config Tw=%.1f Td=%.1f Hw=%.1f Hd=%.1f Wd=%.1f Rd=%.1f Int=%lu ms",
                        g_config.temp_warn, g_config.temp_danger,
                        g_config.humi_warn, g_config.humi_danger,
                        g_config.wind_danger, g_config.rain_danger,
                        g_config.sample_interval_ms);
                }
                else {
                    // Tổng hợp thông số trạm quan trắc + ngưỡng hiện tại
                    snprintf(msgbuf, sizeof(msgbuf),
                            "PTIT: T=%.1fC, H=%.1f%%, P=%.1fhPa, W=%.1fm/s, R=%.1fmm. GPS:%.5f,%.5f |"
                            "Config: Tw=%.1f Td=%.1f Hw=%.1f Hd=%.1f Wd=%.1f Rd=%.1f Int=%lu ms",
                            latest.t_avg, latest.h_avg, latest.p_avg,
                            latest.w_avg, latest.r_avg,
                            latest.latitude, latest.longitude,
                            g_config.temp_warn, g_config.temp_danger,
                            g_config.humi_warn, g_config.humi_danger,
                            g_config.wind_danger, g_config.rain_danger,
                            g_config.sample_interval_ms);
                }

                Serial.printf("[Comm] Replying to %s\n", sender.c_str());
                bool ok = sendWithRetries(sender.c_str(), msgbuf, MAX_RETRY);
                if (ok) {
                    Serial.println("[Comm] Reply SUCCESS");
                } else {
                    Serial.println("[Comm] Reply FAILED -> trying shorter message");
                    snprintf(msgbuf, sizeof(msgbuf),
                            "Reply failed. Config Tw=%.1f Td=%.1f Hw=%.1f Hd=%.1f",
                            g_config.temp_warn, g_config.temp_danger,
                            g_config.humi_warn, g_config.humi_danger);
                    sendWithRetries(sender.c_str(), msgbuf, 1);
                }
            }
            // --- SET CONFIG ---
            else if (content.startsWith("SET"))
            {
                Config_t cfgMsg = g_config; // copy cấu hình hiện tại
                bool updated = false;

                if (content.indexOf("TEMP_WARN=") >= 0) {
                    cfgMsg.temp_warn = content.substring(content.indexOf("=") + 1).toFloat();
                    updated = true;
                }
                else if (content.indexOf("TEMP_DANGER=") >= 0) {
                    cfgMsg.temp_danger = content.substring(content.indexOf("=") + 1).toFloat();
                    updated = true;
                }
                else if (content.indexOf("HUMI_WARN=") >= 0) {
                    cfgMsg.humi_warn = content.substring(content.indexOf("=") + 1).toFloat();
                    updated = true;
                }
                else if (content.indexOf("HUMI_DANGER=") >= 0) {
                    cfgMsg.humi_danger = content.substring(content.indexOf("=") + 1).toFloat();
                    updated = true;
                }
                else if (content.indexOf("WIND_DANGER=") >= 0) {
                    cfgMsg.wind_danger = content.substring(content.indexOf("=") + 1).toFloat();
                    updated = true;
                }
                else if (content.indexOf("RAIN_DANGER=") >= 0) {
                    cfgMsg.rain_danger = content.substring(content.indexOf("=") + 1).toFloat();
                    updated = true;
                }
                else if (content.indexOf("SAMPLE_INTERVAL=") >= 0) {
                    cfgMsg.sample_interval_ms = content.substring(content.indexOf("=") + 1).toInt();
                    updated = true;
                }

                if (updated && Queue_Config != NULL)
                {
                    xQueueSend(Queue_Config, &cfgMsg, 0);
                    Serial.println("[Comm] Config updated via SMS");

                    // Gửi SMS xác nhận
                    snprintf(msgbuf, sizeof(msgbuf), "Config updated OK: %s", content.c_str());
                    sendWithRetries(sender.c_str(), msgbuf, 1);
                }
                else
                {
                    sendWithRetries(sender.c_str(), "Config update failed: invalid format", 1);
                }

            }
            else {
                Serial.println("[Comm] SMS content not recognized, ignoring...");
            }

        }

        // =====================================================
        // 4. GỬI DỮ LIỆU ĐỊNH KỲ (Mỗi 5 phút)
        // =====================================================
//         if (hasData && (now - lastPeriodicSend >= 300000)) 
//         {
//             snprintf(msgbuf, sizeof(msgbuf),
//                      "DINH KY: T=%.1fC, H=%.1f%%, P=%.1f, W=%.1f, R=%.1f. GPS:%.5f,%.5f",
//                      latest.t_avg, latest.h_avg, latest.p_avg, 
//                      latest.w_avg, latest.r_avg,
//                      latest.latitude, latest.longitude);

//             Serial.println("[Comm] Sending periodic report...");
//             for (int i = 0; i < alertPhoneCount; i++) {
//                 sendWithRetries(alertPhones[i], msgbuf, 1); // Định kỳ chỉ thử 1 lần
//             }
//             lastPeriodicSend = now;
//         }

//         vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1s để nhường CPU cho các task khác
    }
}

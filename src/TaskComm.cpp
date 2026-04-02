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

// ===== Simple internal SMS queue (non-blocking state-machine in Task_Comm) =====
typedef struct {
    char phone[32];
    char msg[384];
    uint8_t maxAttempts;
} SMSReq_t;

static const int SMS_Q_SIZE = 8;
static SMSReq_t smsQueue[SMS_Q_SIZE];
static int sms_q_head = 0;
static int sms_q_tail = 0;

static bool enqueueSMS(const char *phone, const char *msg, uint8_t maxAttempts) {
    int next = (sms_q_tail + 1) % SMS_Q_SIZE;
    if (next == sms_q_head) return false; // full
    strncpy(smsQueue[sms_q_tail].phone, phone, sizeof(smsQueue[sms_q_tail].phone)-1);
    smsQueue[sms_q_tail].phone[sizeof(smsQueue[sms_q_tail].phone)-1] = '\0';
    strncpy(smsQueue[sms_q_tail].msg, msg, sizeof(smsQueue[sms_q_tail].msg)-1);
    smsQueue[sms_q_tail].msg[sizeof(smsQueue[sms_q_tail].msg)-1] = '\0';
    smsQueue[sms_q_tail].maxAttempts = maxAttempts;
    sms_q_tail = next;
    return true;
}

static bool dequeueSMS(SMSReq_t &out) {
    if (sms_q_head == sms_q_tail) return false;
    out = smsQueue[sms_q_head];
    sms_q_head = (sms_q_head + 1) % SMS_Q_SIZE;
    return true;
}

// State for current send
typedef struct {
    bool active;
    SMSReq_t req;
    uint8_t attempts;
    uint8_t state; // 0 idle,1 sent CMGF wait OK,2 sent CMGS wait '>',3 sent payload wait result
    uint32_t ts;
} SMSState_t;

static SMSState_t smsState = {0};

static void processSmsState() {
    // This function must be called frequently from the main loop
    if (!smsState.active) {
        SMSReq_t next;
        if (dequeueSMS(next)) {
            smsState.active = true;
            smsState.req = next;
            smsState.attempts = 0;
            smsState.state = 1;
            smsState.ts = millis();
            Serial.printf("[Comm] SMS queued -> starting send to %s\n", smsState.req.phone);
            // send CMGF to ensure text mode
            Serial2.println("AT+CMGF=1");
            return;
        }
        return;
    }

    // active send flow
    if (smsState.state == 1) {
        // wait for OK from CMGF
        if (modem.scanFor("OK")) {
            // start CMGS
            char buf[64];
            snprintf(buf, sizeof(buf), "AT+CMGS=\"%s\"", smsState.req.phone);
            Serial2.println(buf);
            smsState.state = 2;
            smsState.ts = millis();
            return;
        }
        if (millis() - smsState.ts > 2000) {
            // retry sending CMGF
            Serial2.println("AT+CMGF=1");
            smsState.ts = millis();
        }
    }
    else if (smsState.state == 2) {
        // wait for '>' prompt
        if (modem.scanFor(">")) {
            // send payload
            Serial2.print(smsState.req.msg);
            delay(100);
            Serial2.write(26); // Ctrl+Z
            smsState.state = 3;
            smsState.ts = millis();
            return;
        }
        if (millis() - smsState.ts > 5000) {
            // timeout, retry whole send
            smsState.attempts++;
            if (smsState.attempts >= smsState.req.maxAttempts) {
                Serial.printf("[Comm] SMS send to %s failed after attempts\n", smsState.req.phone);
                smsState.active = false;
            } else {
                Serial.println("[Comm] Prompt timeout, retrying CMGS...");
                char buf[64];
                snprintf(buf, sizeof(buf), "AT+CMGS=\"%s\"", smsState.req.phone);
                Serial2.println(buf);
                smsState.ts = millis();
            }
        }
    }
    else if (smsState.state == 3) {
        // wait for +CMGS or OK or ERROR
        if (modem.scanFor("+CMGS") || modem.scanFor("OK")) {
            Serial.printf("[Comm] SMS to %s sent OK\n", smsState.req.phone);
            smsState.active = false;
            return;
        }
        if (modem.scanFor("ERROR")) {
            Serial.printf("[Comm] SMS to %s reported ERROR\n", smsState.req.phone);
            smsState.attempts++;
            if (smsState.attempts >= smsState.req.maxAttempts) {
                Serial.printf("[Comm] SMS to %s failed after retries\n", smsState.req.phone);
                smsState.active = false;
            } else {
                // retry: send CMGF and then CMGS again
                Serial2.println("AT+CMGF=1");
                smsState.state = 1;
                smsState.ts = millis();
            }
            return;
        }
        if (millis() - smsState.ts > 15000) {
            Serial.printf("[Comm] SMS to %s timeout\n", smsState.req.phone);
            smsState.attempts++;
            if (smsState.attempts >= smsState.req.maxAttempts) {
                smsState.active = false;
            } else {
                Serial2.println("AT+CMGF=1");
                smsState.state = 1;
                smsState.ts = millis();
            }
        }
    }
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

        // Progress SMS sending state-machine (non-blocking)
        processSmsState();

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
                    if (!enqueueSMS(alertPhones[i], alert.message, MAX_RETRY)) {
                        Serial.println("[Comm] Warning: SMS queue full, alert not enqueued");
                    }
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
                if (!enqueueSMS(sender.c_str(), msgbuf, MAX_RETRY)) {
                    Serial.println("[Comm] Warning: SMS queue full, reply not enqueued");
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

                if (updated)
                {
                    bool updated_now = false;
                    // Try to update global config directly under mutex for immediate effect
                    if (Config_Mutex != NULL) {
                        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                            g_config = cfgMsg;
                            xSemaphoreGive(Config_Mutex);
                            updated_now = true;
                            Serial.println("[Comm] g_config updated directly via SMS");
                        } else {
                            Serial.println("[Comm] Failed to take Config_Mutex to update g_config");
                        }
                    }

                    // Also try to notify Processing via Queue_Config (best-effort)
                    if (Queue_Config != NULL) {
                        if (xQueueSend(Queue_Config, &cfgMsg, pdMS_TO_TICKS(100)) != pdTRUE) {
                            Serial.println("[Comm] Warning: Queue_Config full, notification not sent");
                        }
                    }

                    // Gửi SMS xác nhận (dù queue có đầy, g_config đã được cập nhật nếu mutex thành công)
                    if (updated_now) {
                        snprintf(msgbuf, sizeof(msgbuf), "Config updated OK: %s", content.c_str());
                        if (!enqueueSMS(sender.c_str(), msgbuf, 1)) Serial.println("[Comm] Warning: SMS queue full, confirm not enqueued");
                    } else {
                        if (!enqueueSMS(sender.c_str(), "Config received but mutex unavailable", 1)) Serial.println("[Comm] Warning: SMS queue full, confirm not enqueued");
                    }
                }
                else
                {
                    if (!enqueueSMS(sender.c_str(), "Config update failed: invalid format", 1)) Serial.println("[Comm] Warning: SMS queue full, error not enqueued");
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

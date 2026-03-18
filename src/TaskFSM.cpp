#include <Arduino.h>
#include "TaskFSM.h"

// Event bits
#define EVENT_WARNING_BIT (1 << 0)
#define EVENT_DANGER_BIT  (1 << 1)
#define EVENT_ERROR_BIT   (1 << 2)

// Thresholds will be read from shared g_config (protected by Config_Mutex)

void Task_FSM(void *pvParameters) {
    ProcessedSensor_t ps;
    Alert_t alert;

    while (1) {
        if (xQueueReceive(Queue_FSM_Input, &ps, portMAX_DELAY) == pdTRUE) {
            AlertLevel_t level = ALERT_INFO;

            // Simple FSM checks: escalate to highest triggered level
            float temp_warn = 35.0f, temp_danger = 40.0f, humi_warn = 80.0f, humi_danger = 90.0f, wind_danger = 20.0f, rain_danger = 10.0f;
            if (Config_Mutex != NULL) {
                if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                    temp_warn = g_config.temp_warn;
                    temp_danger = g_config.temp_danger;
                    humi_warn = g_config.humi_warn;
                    humi_danger = g_config.humi_danger;
                    wind_danger = g_config.wind_danger;
                    rain_danger = g_config.rain_danger;
                    xSemaphoreGive(Config_Mutex);
                }
            }

            if (ps.t_avg >= temp_danger || ps.h_avg >= humi_danger || ps.w_avg >= wind_danger || ps.r_avg >= rain_danger) {
                level = ALERT_DANGER;
            } else if (ps.t_avg >= temp_warn || ps.h_avg >= humi_warn) {
                level = ALERT_WARNING;
            }

            // If data seems invalid (NaN) mark as ERROR
            if (isnan(ps.t_avg) || isnan(ps.h_avg)) {
                level = ALERT_ERROR;
            }

            // Prepare alert if not INFO
            if (level != ALERT_INFO) {
                alert.level = level;
                alert.timestamp = ps.timestamp;
                if (level == ALERT_WARNING) {
                   // snprintf(alert.message, sizeof(alert.message), "WARNING: T=%.1f H=%.1f", ps.t_avg, ps.h_avg);
                } else if (level == ALERT_DANGER) {
                    snprintf(alert.message, sizeof(alert.message), "DANGER: T=%.1f H=%.1f W=%.1f R=%.2f", ps.t_avg, ps.h_avg, ps.w_avg, ps.r_avg);
                } else {
                    snprintf(alert.message, sizeof(alert.message), "ERROR: invalid sensor data");
                }

                // send alert (non-blocking)
                if (Queue_Alert != NULL) {
                    xQueueSend(Queue_Alert, &alert, 0);
                }
            }

            // Set event bits for real-time tasks
            if (Event_Weather != NULL) {
                if (level == ALERT_WARNING) xEventGroupSetBits(Event_Weather, EVENT_WARNING_BIT);
                else if (level == ALERT_DANGER) xEventGroupSetBits(Event_Weather, EVENT_DANGER_BIT);
                else if (level == ALERT_ERROR) xEventGroupSetBits(Event_Weather, EVENT_ERROR_BIT);
            }
        }
    }
}

#include <Arduino.h>
#include "TaskFSM.h"

// Event bits
#define EVENT_WARNING_BIT (1 << 0)
#define EVENT_DANGER_BIT  (1 << 1)
#define EVENT_ERROR_BIT   (1 << 2)

// Thresholds (example values — tune as needed)
static const float TEMP_WARN = 35.0f; // °C
static const float TEMP_DANGER = 40.0f;
static const float HUMI_WARN = 80.0f; // %
static const float HUMI_DANGER = 90.0f;
static const float WIND_DANGER = 20.0f; // m/s
static const float RAIN_DANGER = 10.0f; // mm per sample

void Task_FSM(void *pvParameters) {
    ProcessedSensor_t ps;
    Alert_t alert;

    while (1) {
        if (xQueueReceive(Queue_FSM_Input, &ps, portMAX_DELAY) == pdTRUE) {
            AlertLevel_t level = ALERT_INFO;

            // Simple FSM checks: escalate to highest triggered level
            if (ps.t_avg >= TEMP_DANGER || ps.h_avg >= HUMI_DANGER || ps.w_avg >= WIND_DANGER || ps.r_avg >= RAIN_DANGER) {
                level = ALERT_DANGER;
            } else if (ps.t_avg >= TEMP_WARN || ps.h_avg >= HUMI_WARN) {
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
                    snprintf(alert.message, sizeof(alert.message), "WARNING: T=%.1f H=%.1f", ps.t_avg, ps.h_avg);
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

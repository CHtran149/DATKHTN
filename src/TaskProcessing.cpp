#include <Arduino.h>
#include "TaskProcessing.h"

// Simple moving average buffer size
#define MA_SIZE 5

static float buf_temp[MA_SIZE];
static float buf_humi[MA_SIZE];
static float buf_press[MA_SIZE];
static float buf_wind[MA_SIZE];
static float buf_rain[MA_SIZE];
static int buf_idx = 0;
static int buf_count = 0;

static float ma_avg(float *buf, int count) {
    float s = 0.0f;
    for (int i = 0; i < count; ++i) s += buf[i];
    return count ? s / count : 0.0f;
}

static float heatIndexC(float T_c, float RH) {
    // Convert C to F
    float T = T_c * 1.8f + 32.0f;
    float R = RH;
     //NOAA formula
    float HI = -42.379f + 2.04901523f * T + 10.14333127f * R - 0.22475541f * T * R
               - 6.83783e-3f * T * T - 5.481717e-2f * R * R
               + 1.22874e-3f * T * T * R + 8.5282e-4f * T * R * R - 1.99e-6f * T * T * R * R;
   //  convert back to C
    return (HI - 32.0f) * 1.8f;
}

void Task_Processing(void *pvParameters) {
    SensorRaw_t raw;
    Config_t cfgMsg;

    while (1) {
        if (xQueueReceive(Queue_SensorRaw, &raw, portMAX_DELAY) == pdTRUE) {
            // check for configuration updates from Blynk (non-blocking)
            if (Queue_Config != NULL) {
                if (xQueueReceive(Queue_Config, &cfgMsg, 0) == pdTRUE) {
                    if (Config_Mutex != NULL) {
                        if (xSemaphoreTake(Config_Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                            g_config = cfgMsg;
                            xSemaphoreGive(Config_Mutex);
                            Serial.println("[Processing] Updated g_config from Queue_Config");
                            Serial.printf("[Processing] New sample_interval_ms=%lu, temp_warn=%.1f, humi_warn=%.1f...\n",
                                g_config.sample_interval_ms, g_config.temp_warn, g_config.humi_warn);
                        }
                    }
                }
            }
            // push into buffers
            buf_temp[buf_idx] = raw.temperature;
            buf_humi[buf_idx] = raw.humidity;
            buf_press[buf_idx] = raw.pressure;
            buf_wind[buf_idx] = raw.wind_speed;
            buf_rain[buf_idx] = raw.rainfall;

            buf_idx = (buf_idx + 1) % MA_SIZE;
            if (buf_count < MA_SIZE) buf_count++;

            float T_avg = ma_avg(buf_temp, buf_count);
            float H_avg = ma_avg(buf_humi, buf_count);
            float P_avg = ma_avg(buf_press, buf_count);
            float W_avg = ma_avg(buf_wind, buf_count);
            float R_avg = ma_avg(buf_rain, buf_count);

            float heat_c = heatIndexC(T_avg, H_avg);

            // Simple index mappings (0-100)
            int RainIndex = (int)constrain(R_avg * 10.0f, 0, 100);
            int WindIndex = (int)constrain(W_avg * 10.0f, 0, 100);

             // ===== DOI GPS SANG DANG THAP PHAN (AN TOAN) =====
             float lat = 0.0f;
            float lon = 0.0f;

    // Chỉ tính toán nếu dữ liệu GPS không bị trống (ví dụ độ độ không phải là 0)
    // Hoặc tốt nhất là thêm cờ valid vào SensorRaw_t
    if (raw.lat_deg != 0 || raw.lon_deg != 0) {
             lat = (float)raw.lat_deg + (float)raw.lat_min / 60.0f + (float)raw.lat_sec / 3600.0f;
            lon = (float)raw.lon_deg + (float)raw.lon_min / 60.0f + (float)raw.lon_sec / 3600.0f;
    
            if (raw.lat_dir == 'S') lat = -lat;
             if (raw.lon_dir == 'W') lon = -lon;
    }

            // Build processed message to send to FSM
            ProcessedSensor_t ps;
            ps.t_avg = T_avg;
            ps.h_avg = H_avg;
            ps.p_avg = P_avg;
            ps.w_avg = W_avg;
            ps.r_avg = R_avg;
            ps.heat_index_c = heat_c;
            ps.rain_index = RainIndex;
            ps.wind_index = WindIndex;
            ps.latitude = lat;
            ps.longitude = lon;
            ps.timestamp = raw.timestamp;

            // Send processed data to FSM input queue (non-blocking)
            if (Queue_FSM_Input != NULL) {
                xQueueSend(Queue_FSM_Input, &ps, 0);
            }

            // Also send processed data to both Blynk and Cloud queues
            if (Queue_Data_Blynk != NULL) {
                xQueueSend(Queue_Data_Blynk, &ps,0);
            }
            if (Queue_Data_Cloud != NULL) {
                xQueueSend(Queue_Data_Cloud, &ps,0);
            }
            if (Queue_Data_Comm != NULL) {
               xQueueSend(Queue_Data_Comm, &ps, 0);
            }

            // Output processed values
            Serial.println("---- Processed Sensor Data ----");
            Serial.print("T_avg: "); Serial.print(T_avg); Serial.print(" C | ");
            Serial.print("H_avg: "); Serial.print(H_avg); Serial.print(" %\n");

            Serial.print("Pressure_avg: "); Serial.print(P_avg); Serial.print(" hPa | ");
            Serial.print("Alt (raw): "); Serial.print(raw.altitude); Serial.print(" m\n");

            Serial.print("Wind_avg: "); Serial.print(W_avg); Serial.print(" m/s | ");
            Serial.print("Rain_avg: "); Serial.print(R_avg); Serial.print(" mm\n");

            Serial.print("HeatIndex (C): "); Serial.print(heat_c); Serial.print(" | ");
            Serial.print("RainIndex: "); Serial.print(RainIndex); Serial.print(" | ");
            Serial.print("WindIndex: "); Serial.println(WindIndex);

            Serial.print("Vi do: "); Serial.print(lat, 6);
            Serial.print(" | Kinh do: "); Serial.println(lon, 6);
            Serial.println("-------------------------------\n");
        }
    }
}

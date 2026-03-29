#include <Arduino.h>
#include <Wire.h>

/* ===== Include module ===== */
#include "all_header.h"
/* ===== PIN DEFINE ===== */
#define PIN_DHT        10
#define PIN_WIND       7
#define PIN_RAIN       5

#define SDA_PIN        13
#define SCL_PIN        12

#define GPS_RX         18
#define GPS_TX         17

#define GSM_RX         44
#define GSM_TX         43

/* ===== Object ===== */
DHT11 dht(PIN_DHT);
BMP180 bmp;
WindSpeed wind(PIN_WIND, 0.1f);
Rainfall rain(PIN_RAIN, 150.0f, 12000.0f, 10000);
GPS gps(Serial1);
// GSM modem instance (uses Serial2 by default) -- configure pins in setup()
GSM modem(Serial2, 115200);

/* ===== Queues & Events ===== */
QueueHandle_t Queue_SensorRaw;
QueueHandle_t Queue_FSM_Input;
QueueHandle_t Queue_Alert;
EventGroupHandle_t Event_Weather;
// additional
QueueHandle_t Queue_Data_Blynk;
QueueHandle_t Queue_Data_Cloud;
QueueHandle_t Queue_Config;
SemaphoreHandle_t Config_Mutex;
QueueHandle_t Queue_Data_Comm;
// define global config
Config_t g_config;

void setup() {
    Serial.begin(115200);
    Serial.println("boot ok");
    delay(2000);
    Serial.println("\n--- He thong bat dau khoi dong ---");
    Wire.begin(SDA_PIN, SCL_PIN);

    bmp.begin();
    wind.begin();
    rain.begin();
    gps.begin(115200);
    modem.begin(GSM_RX, GSM_TX);
    // Serial2.println("AT+CMGF=1");
    // delay(500);

    // Serial2.println("AT+CNMI=2,2,0,0,0");
    // delay(500);

    Queue_SensorRaw = xQueueCreate(5, sizeof(SensorRaw_t));
    Queue_FSM_Input = xQueueCreate(5, sizeof(ProcessedSensor_t));
    Queue_Alert = xQueueCreate(5, sizeof(Alert_t));
    Queue_Data_Comm = xQueueCreate(10, sizeof(ProcessedSensor_t));
    Queue_Data_Blynk = xQueueCreate(5, sizeof(ProcessedSensor_t));
    Queue_Data_Cloud = xQueueCreate(5, sizeof(ProcessedSensor_t));
    Queue_Config = xQueueCreate(2, sizeof(Config_t));
    Config_Mutex = xSemaphoreCreateMutex();
    Event_Weather = xEventGroupCreate();

    // initialize default config
    g_config.sample_interval_ms = 3000;
    g_config.temp_warn = 35.0f;
    g_config.temp_danger = 40.0f;
    g_config.humi_warn = 80.0f;
    g_config.humi_danger = 90.0f;
    g_config.wind_danger = 20.0f;
    g_config.rain_danger = 10.0f;

    xTaskCreatePinnedToCore(Task_Sensor,"Task_Sensor",8192,NULL,2,NULL,1);
    Serial.println("Task_Sensor created");

    xTaskCreatePinnedToCore(Task_Processing,"Task_Processing",8192,NULL,2,NULL,1);
    Serial.println("Task_Processing created");

    xTaskCreatePinnedToCore(Task_FSM,"Task_FSM",8192,NULL,2,NULL,1);
    Serial.println("Task_FSM created");

    xTaskCreatePinnedToCore(Task_Comm,"Task_Comm",8192,NULL,1,NULL,1);
    Serial.println("Task_Comm created");

    xTaskCreatePinnedToCore(Task_Blynk,"Task_Blynk",8192,NULL,1,NULL,0);
    Serial.println("Task_Blynk created");

   // xTaskCreatePinnedToCore(Task_Cloud,"Task_Cloud",8192,NULL,1,NULL,0);
    //Serial.println("Task_Cloud created");
    
    Serial.println("All tasks started");
}
void loop() {
}
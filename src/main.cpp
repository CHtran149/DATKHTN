#include <all_header.h>

/* ===== KHAI BÁO OBJECT (CHỈ 1 LẦN) ===== */
DHT11 dht(33);                          // GPIO 33
BMP180 bmp;                             // I2C
WindSpeed wind(35, 0.09f);              // GPIO 35
Rainfall rain(34, 160.0f, 0.279f, 60000);// GPIO 34

QueueHandle_t Queue_SensorRaw;

/* ===== STRUCT DATA ===== */
typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float windSpeed;
    float rainfall;
    uint32_t timestamp;
} SensorData_t;

/* ===== TASK SENSOR ===== */
void taskSensor(void *param) {
    SensorData_t sensorData;
    TickType_t lastWake = xTaskGetTickCount();

    while (1) {
        // DHT11
        DHT11::Data dhtData = dht.read();
        sensorData.temperature = dhtData.valid ? dhtData.temperature : -999;
        sensorData.humidity    = dhtData.valid ? dhtData.humidity    : -999;

        // BMP180
        BMP180::Data bmpData = bmp.read();
        sensorData.pressure = bmpData.valid ? bmpData.pressure : -999;

        // Wind
        WindSpeed::Data windData = wind.read(1000);
        sensorData.windSpeed = windData.valid ? windData.toc_do_gio : -999;

        // Rain
        Rainfall::Data rainData = rain.read();
        sensorData.rainfall = rainData.valid ? rainData.luongMua : 0;

        sensorData.timestamp = millis();

        xQueueSend(Queue_SensorRaw, &sensorData, 0);

        Serial.printf(
            "[Sensor] T=%.1fC H=%.1f%% P=%.1fhPa W=%.2fm/s R=%.2fmm\n",
            sensorData.temperature,
            sensorData.humidity,
            sensorData.pressure,
            sensorData.windSpeed,
            sensorData.rainfall
        );

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1000));
    }
}

/* ===== SETUP ===== */
void setup() {
    Serial.begin(9600);
    delay(2000);

    Wire.begin();

    dht.read();     // kick lần đầu
    bmp.begin();
    wind.begin();
    rain.begin();

    Queue_SensorRaw = xQueueCreate(10, sizeof(SensorData_t));

    xTaskCreatePinnedToCore(
        taskSensor,
        "Task_Sensor",
        4096,
        NULL,
        2,
        NULL,
        1
    );

    Serial.println("System ready");
}

void loop() {}

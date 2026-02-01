#include <Arduino.h>
#include "GSM.h"

#define MODEM_RX 16
#define MODEM_TX 17

GSM gsm(Serial2, 115200);

void setup()
{
    Serial.begin(9600);
    delay(3000);

    gsm.begin(MODEM_RX, MODEM_TX);

    Serial.println("Sending SMS...");
    gsm.sendSMS("+84327161236", "HELLO CHIEN TRAN");
}

void loop()
{
    while (Serial2.available())
    {
        Serial.write(Serial2.read());
    }
}

#include "GSM.h"

// --- Constructor ---
GSM::GSM(HardwareSerial &serial, uint32_t baudrate)
    : modem(serial), baud(baudrate) {}

// --- Khởi động modem ---
void GSM::begin(int rxPin, int txPin)
{
    modem.begin(baud, SERIAL_8N1, rxPin, txPin);
    delay(15000); // chờ modem ổn định

    // Basic initialization: disable echo, set SMS text mode,
    // and enable new-message indications so incoming SMS are
    // forwarded as +CMT: lines to the serial port.
    modem.println("AT");
    delay(500);
    modem.println("ATE0"); // disable echo
    delay(200);
    modem.println("AT+CMGF=1"); // SMS text mode
    delay(200);
    modem.println("AT+CNMI=2,2,0,0,0"); // deliver incoming SMS as +CMT
    delay(200);
    Serial.println("[GSM] Ready");
}

// --- Chờ ký tự phản hồi từ modem ---
void GSM::waitForChar(char target, uint32_t timeout)
{
    uint32_t start = millis();
    while (millis() - start < timeout)
    {
        if (modem.available())
        {
            char c = modem.read();
            Serial.write(c); // log debug
            if (c == target) return;
        }
    }
}

// Wait for a substring in modem response; returns true if found before timeout
static bool waitForResponse(HardwareSerial &modemRef, const char *needle, uint32_t timeout)
{
    uint32_t start = millis();
    String buf;
    while (millis() - start < timeout)
    {
        while (modemRef.available()) {
            char c = modemRef.read();
            buf += c;
            // echo modem traffic to primary Serial for debugging
            Serial.write(c);
            // keep buffer bounded
            if (buf.length() > 1024) buf = buf.substring(buf.length() - 1024);
        }
        if (buf.indexOf(needle) >= 0) return true;
        delay(10);
    }
    return false;
}

// --- Gửi SMS ---
bool GSM::sendSMS(const char *phone, const char *message)
{
    // Set text mode
    modem.println("AT+CMGF=1");
    if (!waitForResponse(modem, "OK", 2000)) {
        Serial.println("[GSM] no OK after CMGF");
        // continue attempt, but report failure
    }

    // Start send
    modem.print("AT+CMGS=\"");
    modem.print(phone);
    modem.println("\"");

    // wait for '>' prompt
    if (!waitForResponse(modem, ">", 5000)) {
        Serial.println("[GSM] no '>' prompt for CMGS");
        return false;
    }

    modem.print(message);
    delay(200);
    modem.write(26); // Ctrl+Z

    // After sending, modem replies with +CMGS: <mr> and then OK, or ERROR
    if (waitForResponse(modem, "+CMGS", 15000) || waitForResponse(modem, "OK", 15000)) {
        return true;
    }

    // final check for ERROR
    if (waitForResponse(modem, "ERROR", 1000)) {
        Serial.println("[GSM] CMGS ERROR");
        return false;
    }

    // Default to failure if no positive response
    return false;
}


bool GSM::readSMS(String &sender, String &content)
{
    if (!modem.available()) return false;

    String line = modem.readStringUntil('\n');
    line.trim();

    // Debug raw
    Serial.print("[RAW] ");
    Serial.println(line);

    // Check header SMS
    if (line.startsWith("+CMT:"))
    {
        // ===== LẤY SỐ ĐIỆN THOẠI =====
        int q1 = line.indexOf('"');
        int q2 = line.indexOf('"', q1 + 1);

        if (q1 >= 0 && q2 > q1)
        {
            sender = line.substring(q1 + 1, q2);
        }
        else
        {
            sender = "UNKNOWN";
        }

        // ===== ĐỌC NỘI DUNG =====
        uint32_t t = millis();
        while (!modem.available())
        {
            if (millis() - t > 2000) return false; // timeout
        }

        content = modem.readStringUntil('\n');
        content.trim();

        Serial.print("[SMS] From: ");
        Serial.println(sender);
        Serial.print("[SMS] Content: ");
        Serial.println(content);

        return true;
    }

    return false;
}
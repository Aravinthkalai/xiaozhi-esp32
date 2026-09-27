#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "moto-edge60-fusion";
const char* WIFI_PASSWORD = "qazwsxedc";

const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* MQTT_TOPIC = "xiaozhi/test/slave/led";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void connectWiFi()
{
    Serial.print("Connecting to Wi-Fi");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    String message;

    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("MQTT message: ");
    Serial.println(message);

    if (message == "ON")
    {
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("LED ON");
    }
    else if (message == "OFF")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("LED OFF");
    }
}

void connectMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Connecting to MQTT...");

        String clientId = "xiaozhi-slave-";
        clientId += String(ESP.getChipId(), HEX);

        if (mqttClient.connect(clientId.c_str()))
        {
            Serial.println("connected");

            mqttClient.subscribe(MQTT_TOPIC);

            Serial.print("Subscribed to: ");
            Serial.println(MQTT_TOPIC);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.println(mqttClient.state());

            delay(3000);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println();
    Serial.println("================================");
    Serial.println("XiaoZhi Slave ESP8266");
    Serial.println("================================");

    connectWiFi();

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

void loop()
{
    if (!mqttClient.connected())
    {
        connectMQTT();
    }

    mqttClient.loop();
}
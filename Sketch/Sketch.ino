#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <AccelStepper.h>
#include "DHTesp.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 16    // Pin which is connected to the DHT11 sensor.
#define DHTTYPE DHT11  // DHT 11
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17
#define steps_per_revolution 171 // Adjust this value based on your stepper motor specifications
// #define BUZZER_PIN 4    // Pin connected to the buzzer


DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte degree_symbol[8] =
            {
              0b00111,
              0b00101,
              0b00111,
              0b00000,
              0b00000,
              0b00000,
              0b00000,
              0b00000
            };

WiFiClient espClient;
PubSubClient client(espClient);
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);

const char* ssid = "iPhone";
const char* password = "aaaa1111";
const char* mqtt_server = "broker.hivemq.com";  // Replace with your MQTT broker address
const int mqtt_port = 1883;  // Replace with your MQTT broker port if different

int currentWindowPosition = -1;  // Initialize to an invalid position

// WiFiClient espClient;
// PubSubClient client(espClient);
unsigned long lastMsg = 0;
// #define MSG_BUFFER_SIZE  (50)
float temp = 0;
float hum = 0;
int value = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degree_symbol);
  lcd.setCursor(0,0);
  lcd.print("  DHT11   with ");
  lcd.setCursor(0,1);
  lcd.print("  ESP32 DevKit ");
  delay(2000);
  lcd.clear();
  client.setServer(mqtt_server, mqtt_port);

  stepper.setMaxSpeed(1000.0);
  stepper.setAcceleration(500.0);
  stepper.setCurrentPosition(0);  // Start position
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// void soundBuzzer() {
//   digitalWrite(BUZZER_PIN, HIGH);
//   delay(1000); // Sound buzzer for 1 second
//   digitalWrite(BUZZER_PIN, LOW);
// }

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    int humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    lcd.setCursor(0,0);
    lcd.print("Temp = ");
    lcd.print(temperature);
    lcd.write(0);
    lcd.print("C");
    lcd.setCursor(0,1);
    lcd.print("Humidity = ");
    lcd.print(humidity);
    lcd.print("%");
    // delay(1000);

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("ºC ");

    float ftemp = (temperature * 9/5) + 32;
    float heatIndex = -42.379 + 2.04901523*ftemp + 10.14333127*humidity - .22475541*ftemp*humidity - .00683783*ftemp*ftemp - .05481717*humidity*humidity + .00122874*ftemp*ftemp*humidity + .00085282*ftemp*humidity*humidity - .00000199*ftemp*ftemp*humidity*humidity;
    Serial.print("Heat Index: ");
    Serial.print(heatIndex);
    Serial.print("ºF ");

    Serial.print("Humidity: ");
    Serial.println(humidity);

    String HI = String(heatIndex, 3);
    client.publish("iotfrontier/heatIndex", HI.c_str());

    // int newWindowPosition = getWindowPosition(heatIndex);
    // if (newWindowPosition != currentWindowPosition) {
    //   stepper.moveTo(newWindowPosition);
    //   stepper.runToPosition();
    //   soundBuzzer();
    //   currentWindowPosition = newWindowPosition;
    // }


    int newWindowPosition = getWindowPosition(heatIndex);
    if (newWindowPosition != currentWindowPosition) {
      stepper.moveTo(newWindowPosition);
      stepper.runToPosition();
      currentWindowPosition = newWindowPosition;
    }
  }
}

int getWindowPosition(float heatIndex) {
  if (heatIndex < 80) {
    return 0;  // Fully closed position
  } else if (heatIndex >= 80 && heatIndex <= 103) {
    return steps_per_revolution;  // Partially open position
  } else if (heatIndex > 103 && heatIndex <= 124) {
    return steps_per_revolution + 171;  // More open position
  } else if (heatIndex > 124) {
    return steps_per_revolution + 342;  // Fully open position
  }
  return 0;  // Default to fully closed
}
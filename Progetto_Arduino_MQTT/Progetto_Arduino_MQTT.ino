#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>

#define TRIG_PIN 4
#define ECHO_PIN 3
#define BUZZER_PIN 2
#define NEGATIVE_BUZZER_PIN 1
#define DATA_PIN 11
#define LATCH_PIN 12
#define CLOCK_PIN 13

LiquidCrystal lcd(10, 9, 5, 6, 7, 8);

// === CONFIGURAZIONE WiFi E MQTT ===
const char* ssid = "TUO_WIFI";              // ← Inserisci il tuo SSID
const char* password = "TUA_PASSWORD";      // ← Inserisci la tua password WiFi
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// === VARIABILI DI STATO ===
int alarmCount = 0;
unsigned long negativeBuzzerStartTime = 0;
bool negativeBuzzerActive = false;

// === CODICI PER IL DISPLAY A 7 SEGMENTI ===
byte numbers[] = {
  0b00111111, 0b00000110, 0b01011011, 0b01001111,
  0b01100110, 0b01101101, 0b01111101, 0b00000111,
  0b01111111, 0b01101111
};

void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connessione al WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connesso");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connessione a MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println(" ✅ connesso");
    } else {
      Serial.print(" ❌ errore: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(NEGATIVE_BUZZER_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Inizializzazione...");
  delay(2000);
  lcd.clear();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // === Lettura distanza dal sensore HC-SR04 ===
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.0344 / 2;
  if (duration == 0 || distance > 200) distance = 999;

  Serial.print("📏 Distanza: ");
  Serial.print(distance);
  Serial.println(" cm");

  // === Pubblicazione su MQTT ===
  char payload[10];
  sprintf(payload, "%ld", distance);
  client.publish("arduino/distanza", payload);

  int frequency = constrain(map(distance, 2, 200, 1000, 5000), 100, 5000);
  static long lastDistance = -1;

  if (lastDistance != distance) {
    lcd.clear();
    if (distance < 20) {
      tone(BUZZER_PIN, frequency);
      alarmCount++;
      lcd.setCursor(0, 0);
      lcd.print("ALLARME!");

      if (alarmCount == 9) {
        tone(NEGATIVE_BUZZER_PIN, 1000);
        negativeBuzzerStartTime = millis();
        negativeBuzzerActive = true;
        alarmCount = 0;
      }
    } else if (distance < 200) {
      noTone(BUZZER_PIN);
      lcd.setCursor(0, 0);
      lcd.print("Distanza: ");
      lcd.print(distance);
      lcd.print(" cm");
    } else {
      noTone(BUZZER_PIN);
      lcd.setCursor(0, 0);
      lcd.print("Fuori portata");
    }
    lastDistance = distance;
  }

  // === Gestione buzzer negativo ===
  if (negativeBuzzerActive) {
    unsigned long elapsedTime = millis() - negativeBuzzerStartTime;
    if (elapsedTime >= 10000) {
      noTone(NEGATIVE_BUZZER_PIN);
      negativeBuzzerActive = false;
      lcd.clear();
      lcd.print("Buzzer finito!");
    } else {
      tone(NEGATIVE_BUZZER_PIN, 1000);
      int elapsedSeconds = elapsedTime / 1000;
      displayNumberOn7Segment(elapsedSeconds);
    }
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Allarmi: ");
    lcd.print(alarmCount);
  }

  delay(100);
}

// === Display a 7 segmenti ===
void displayNumberOn7Segment(int num) {
  if (num > 9) num = 9;
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, numbers[num]);
  digitalWrite(LATCH_PIN, HIGH);
}

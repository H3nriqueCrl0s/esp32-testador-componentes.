/*
 * Testador de Componentes - ESP32
 * Página web com status dos sensores e controle do buzzer e do motor.
 *
 * Bibliotecas (instaladas automaticamente pelo PlatformIO via platformio.ini):
 *   - Adafruit INA219
 *   - OneWire
 *   - DallasTemperature
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "pagina.h"

// ===== Wi-Fi =====
// O ESP32 cria sua própria rede. Conecte nela e acesse http://192.168.4.1
const char* AP_SSID    = "ESP32-Testador";
const char* AP_SENHA   = "12345678";

// ===== Pinos =====
const int PINO_BUZZER = 14;
const int PINO_SDA    = 21;  // INA219 SDA
const int PINO_SCL    = 22;  // INA219 SCL
const int PINO_TEMP   = 23;  // DS18B20 (pull-up de 4,7k para 3,3V)
const int PINO_TRIG   = 18;  // HC-SR04 TRIG
const int PINO_ECHO   = 19;  // HC-SR04 ECHO (usar divisor de tensão 5V -> 3,3V)
const int PINO_ENA    = 15;  // ENA em curto com ENB
const int PINO_IN1    = 2;   // IN1 em curto com IN4
const int PINO_IN2    = 4;   // IN2 em curto com IN3

WebServer server(80);
Adafruit_INA219 ina219;
OneWire oneWire(PINO_TEMP);
DallasTemperature sensorTemp(&oneWire);

// ===== Estado =====
bool inaOk = false;
bool tempOk = false;
bool buzzerLigado = false;
int motorSentido = 0;        // 1 = horário, -1 = anti-horário, 0 = parado
int motorVelocidade = 100;   // 0..100 %

float distanciaCm = -1;
float temperaturaC = NAN;
float tensaoV = 0, correnteMa = 0, potenciaMw = 0;

unsigned long ultUltra = 0, ultTemp = 0, ultIna = 0;

// ===== Motor =====
void aplicarMotor() {
  if (motorSentido == 0) {
    analogWrite(PINO_ENA, 0);
    digitalWrite(PINO_IN1, LOW);
    digitalWrite(PINO_IN2, LOW);
    return;
  }
  digitalWrite(PINO_IN1, motorSentido > 0 ? HIGH : LOW);
  digitalWrite(PINO_IN2, motorSentido > 0 ? LOW : HIGH);
  analogWrite(PINO_ENA, map(motorVelocidade, 0, 100, 0, 255));
}

// ===== Sensores =====
float lerDistancia() {
  digitalWrite(PINO_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);
  unsigned long duracao = pulseIn(PINO_ECHO, HIGH, 30000);  // timeout ~5 m
  if (duracao == 0) return -1;
  return duracao * 0.0343f / 2.0f;
}

void atualizarSensores() {
  unsigned long agora = millis();

  if (agora - ultUltra >= 250) {
    ultUltra = agora;
    distanciaCm = lerDistancia();
  }

  if (agora - ultIna >= 250) {
    ultIna = agora;
    if (inaOk) {
      float shuntMv = ina219.getShuntVoltage_mV();
      float busV = ina219.getBusVoltage_V();
      tensaoV = busV + shuntMv / 1000.0f;
      correnteMa = ina219.getCurrent_mA();
      potenciaMw = ina219.getPower_mW();
    }
  }

  // DS18B20 em modo assíncrono: lê o resultado anterior e pede nova conversão
  if (agora - ultTemp >= 1000) {
    ultTemp = agora;
    if (!tempOk) sensorTemp.begin();  // tenta detectar se foi conectado depois
    float t = sensorTemp.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C) {
      tempOk = false;
      temperaturaC = NAN;
    } else {
      tempOk = true;
      temperaturaC = t;
    }
    sensorTemp.requestTemperatures();
  }
}

// ===== Rotas web =====
void handleRaiz() {
  server.send(200, "text/html; charset=utf-8", PAGINA);
}

void handleStatus() {
  String json = "{";
  json += "\"buzzer\":" + String(buzzerLigado ? "true" : "false");
  json += ",\"motorSentido\":" + String(motorSentido);
  json += ",\"motorVelocidade\":" + String(motorVelocidade);
  json += ",\"distancia\":" + (distanciaCm < 0 ? String("null") : String(distanciaCm, 1));
  json += ",\"tempOk\":" + String(tempOk ? "true" : "false");
  json += ",\"temperatura\":" + (isnan(temperaturaC) ? String("null") : String(temperaturaC, 1));
  json += ",\"inaOk\":" + String(inaOk ? "true" : "false");
  json += ",\"tensao\":" + String(tensaoV, 2);
  json += ",\"corrente\":" + String(correnteMa, 1);
  json += ",\"potencia\":" + String(potenciaMw, 0);
  json += "}";
  server.send(200, "application/json", json);
}

void handleBuzzer() {
  String estado = server.arg("estado");
  if (estado == "1") buzzerLigado = true;
  else if (estado == "0") buzzerLigado = false;
  else buzzerLigado = !buzzerLigado;
  digitalWrite(PINO_BUZZER, buzzerLigado ? HIGH : LOW);
  handleStatus();
}

void handleMotor() {
  int novo = server.arg("sentido").toInt();
  if (novo != 1 && novo != -1) novo = 0;
  // Ao inverter o sentido, para o motor um instante antes
  if (motorSentido != 0 && novo != 0 && novo != motorSentido) {
    motorSentido = 0;
    aplicarMotor();
    delay(150);
  }
  motorSentido = novo;
  aplicarMotor();
  handleStatus();
}

void handleVelocidade() {
  motorVelocidade = constrain(server.arg("valor").toInt(), 0, 100);
  aplicarMotor();
  handleStatus();
}

// ===== Wi-Fi =====
void iniciarWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_SENHA);
  Serial.print("Rede criada: ");
  Serial.print(AP_SSID);
  Serial.print("  Senha: ");
  Serial.println(AP_SENHA);
  Serial.print("Acesse: http://");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);

  pinMode(PINO_BUZZER, OUTPUT);
  digitalWrite(PINO_BUZZER, LOW);
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  pinMode(PINO_IN1, OUTPUT);
  pinMode(PINO_IN2, OUTPUT);
  pinMode(PINO_ENA, OUTPUT);
  aplicarMotor();  // garante motor parado

  Wire.begin(PINO_SDA, PINO_SCL);
  inaOk = ina219.begin();
  Serial.println(inaOk ? "INA219 OK" : "INA219 nao encontrado");

  sensorTemp.begin();
  sensorTemp.setWaitForConversion(false);
  sensorTemp.requestTemperatures();
  tempOk = sensorTemp.getDeviceCount() > 0;
  Serial.println(tempOk ? "DS18B20 OK" : "DS18B20 nao encontrado");

  iniciarWiFi();

  server.on("/", handleRaiz);
  server.on("/status", handleStatus);
  server.on("/buzzer", handleBuzzer);
  server.on("/motor", handleMotor);
  server.on("/velocidade", handleVelocidade);
  server.begin();
}

void loop() {
  server.handleClient();
  atualizarSensores();
}

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
#include "logo.h"

// ===== Wi-Fi =====
// O ESP32 cria sua própria rede. Conecte nela e acesse http://192.168.4.1
const char* AP_SSID    = "ESP32-Testador";
const char* AP_SENHA   = "12345678";

// ===== Pinos =====
const int PINO_BUZZER = 14;
const int PINO_SDA    = 21;  // INA219 SDA
const int PINO_SCL    = 22;  // INA219 SCL
const int PINO_ENA    = 15;  // ENA em curto com ENB
const int PINO_IN1    = 2;   // IN1 em curto com IN4
const int PINO_IN2    = 4;   // IN2 em curto com IN3

// Ultrassônicos HC-SR04 (ECHO precisa de divisor de tensão 5V -> 3,3V)
const int NUM_ULTRA = 4;
const int PINOS_TRIG[NUM_ULTRA] = {18, 13, 27, 25};
const int PINOS_ECHO[NUM_ULTRA] = {19, 12, 26, 33};

// DS18B20, um por pino (pull-up de 4,7k para 3,3V em cada)
const int NUM_TEMP = 2;
const int PINOS_TEMP[NUM_TEMP] = {23, 32};

WebServer server(80);
Adafruit_INA219 ina219;
OneWire oneWire[NUM_TEMP] = {OneWire(PINOS_TEMP[0]), OneWire(PINOS_TEMP[1])};
DallasTemperature sensorTemp[NUM_TEMP] = {DallasTemperature(&oneWire[0]), DallasTemperature(&oneWire[1])};

// ===== Estado =====
bool inaOk = false;
bool tempOk[NUM_TEMP] = {false};
bool buzzerLigado = false;
int motorSentido = 0;        // 1 = horário, -1 = anti-horário, 0 = parado
int motorVelocidade = 100;   // 0..100 %

float distanciaCm[NUM_ULTRA] = {-1, -1, -1, -1};  // -1 = sem eco (nada à frente)
bool ultraOk[NUM_ULTRA] = {false};     // sensor detectado no pino
int ultraConta[NUM_ULTRA] = {0};       // leituras seguidas contra o estado atual
float temperaturaC[NUM_TEMP] = {NAN, NAN};
float tensaoV = 0, correnteMa = 0, potenciaMw = 0;

unsigned long ultUltra = 0, ultTemp = 0, ultIna = 0;
int proximoUltra = 0;

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
const float ULTRA_SEM_ECO      = -1;  // sensor respondeu, mas nada à frente (> ~4 m)
const float ULTRA_DESCONECTADO = -2;  // ninguém respondeu no pino: sem sensor ou ruído
const int   ULTRA_CONFIRMAR    = 3;   // leituras seguidas para mudar de conectado/desconectado

float lerDistancia(int i) {
  int trig = PINOS_TRIG[i], echo = PINOS_ECHO[i];

  // Com o pull-down, o ECHO tem que estar em 0 antes do disparo
  if (digitalRead(echo) == HIGH) return ULTRA_DESCONECTADO;

  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // Um HC-SR04 de verdade sobe o ECHO ~0,5 ms depois do disparo
  unsigned long t0 = micros();
  while (digitalRead(echo) == LOW) {
    if (micros() - t0 > 3000) return ULTRA_DESCONECTADO;
  }

  unsigned long inicio = micros();
  while (digitalRead(echo) == HIGH) {
    if (micros() - inicio > 25000) return ULTRA_SEM_ECO;  // ~4,3 m
  }
  float cm = (micros() - inicio) * 0.0343f / 2.0f;
  if (cm < 2) return ULTRA_DESCONECTADO;  // pulso curto demais: ruído, não eco
  return cm;
}

// Só muda o estado conectado/desconectado após ULTRA_CONFIRMAR leituras seguidas
void registrarUltra(int i, float leitura) {
  bool respondeu = leitura != ULTRA_DESCONECTADO;
  if (respondeu == ultraOk[i]) {
    ultraConta[i] = 0;
  } else if (++ultraConta[i] >= ULTRA_CONFIRMAR) {
    ultraOk[i] = respondeu;
    ultraConta[i] = 0;
  }
  if (!respondeu) leitura = ULTRA_SEM_ECO;
  distanciaCm[i] = ultraOk[i] ? leitura : ULTRA_SEM_ECO;
}

void atualizarSensores() {
  unsigned long agora = millis();

  // Um ultrassônico por vez, para o eco de um não ser lido pelo outro
  if (agora - ultUltra >= 60) {
    ultUltra = agora;
    registrarUltra(proximoUltra, lerDistancia(proximoUltra));
    proximoUltra = (proximoUltra + 1) % NUM_ULTRA;
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
    for (int i = 0; i < NUM_TEMP; i++) {
      if (!tempOk[i]) sensorTemp[i].begin();  // tenta detectar se foi conectado depois
      float t = sensorTemp[i].getTempCByIndex(0);
      if (t == DEVICE_DISCONNECTED_C) {
        tempOk[i] = false;
        temperaturaC[i] = NAN;
      } else {
        tempOk[i] = true;
        temperaturaC[i] = t;
      }
      sensorTemp[i].requestTemperatures();
    }
  }
}

// ===== Rotas web =====
void handleRaiz() {
  server.send(200, "text/html; charset=utf-8", PAGINA);
}

void handleLogo() {
  server.sendHeader("Cache-Control", "max-age=86400");  // navegador guarda o logo por 1 dia
  server.send_P(200, "image/png", (const char*)LOGO_PNG, LOGO_PNG_TAM);
}

void handleStatus() {
  String json = "{";
  json += "\"buzzer\":" + String(buzzerLigado ? "true" : "false");
  json += ",\"motorSentido\":" + String(motorSentido);
  json += ",\"motorVelocidade\":" + String(motorVelocidade);
  json += ",\"distancias\":[";
  for (int i = 0; i < NUM_ULTRA; i++) {
    if (i) json += ",";
    json += distanciaCm[i] < 0 ? String("null") : String(distanciaCm[i], 1);
  }
  json += "],\"ultraOk\":[";
  for (int i = 0; i < NUM_ULTRA; i++) {
    if (i) json += ",";
    json += ultraOk[i] ? "true" : "false";
  }
  json += "],\"temperaturas\":[";
  for (int i = 0; i < NUM_TEMP; i++) {
    if (i) json += ",";
    json += isnan(temperaturaC[i]) ? String("null") : String(temperaturaC[i], 1);
  }
  json += "]";
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
  for (int i = 0; i < NUM_ULTRA; i++) {
    pinMode(PINOS_TRIG[i], OUTPUT);
    digitalWrite(PINOS_TRIG[i], LOW);
    pinMode(PINOS_ECHO[i], INPUT_PULLDOWN);  // sem sensor, o pino fica em 0 em vez de captar ruído
  }
  pinMode(PINO_IN1, OUTPUT);
  pinMode(PINO_IN2, OUTPUT);
  pinMode(PINO_ENA, OUTPUT);
  aplicarMotor();  // garante motor parado

  Wire.begin(PINO_SDA, PINO_SCL);
  inaOk = ina219.begin();
  Serial.println(inaOk ? "INA219 OK" : "INA219 nao encontrado");

  for (int i = 0; i < NUM_TEMP; i++) {
    sensorTemp[i].begin();
    sensorTemp[i].setWaitForConversion(false);
    sensorTemp[i].requestTemperatures();
    tempOk[i] = sensorTemp[i].getDeviceCount() > 0;
    Serial.printf("DS18B20 %d (pino %d): %s\n", i + 1, PINOS_TEMP[i], tempOk[i] ? "OK" : "nao encontrado");
  }

  iniciarWiFi();

  server.on("/", handleRaiz);
  server.on("/logo.png", handleLogo);
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

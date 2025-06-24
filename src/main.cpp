#include <Arduino.h>

// Variáveis de distância
float DIS = 400; // Sensor de entrada

// Estado do botão e controle da cancela
int BTN_STATE = 0;

// Definições de pinos
#define LED_R 18
#define LED_G 5
#define TRIG_PIN 33
#define ECHO_PIN 35
#define BTN_PIN 19

// Controle de tempo
unsigned long TIME_ST;
unsigned long LAST_MED = 0;
const unsigned long INT_MED = 500;

// Flags
bool PRES = false;        
bool MSG_IN = false;  
bool OPEN = false;        
bool EMPTY = false;        

// Protótipos
void MED();
void CLOSE();
void BTN();
void CHC_PRES();



void setup() {
  Serial.begin(9600);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  CLOSE(); 
}

void loop(){
  BTN();
  MED();
  CHC_PRES();

  // Estado inicial aguardando
  if (!MSG_IN && BTN_STATE == 0 && !PRES) {
    Serial.println("Aguardando veículo e botão...");
    MSG_IN = true;
  }

  // Botão pressionado sem veículo
  if (BTN_STATE == 1 && !PRES && !EMPTY) {
    Serial.println("Botão pressionado, mas nenhum veículo detectado.");
    BTN_STATE = 0; 
    EMPTY = true; 
    delay(1000); 
    MSG_IN = false; 
    return; 
  }

  // Impressão e abertura
  if (BTN_STATE == 1 && PRES && !OPEN) {
    Serial.println("Botão pressionado. Imprimindo ticket e abrindo cancela...");
    TIME_ST = millis();
    delay(3000); 
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, HIGH);
    OPEN = true;
    return;
  }

  // Tempo excedido
  if (BTN_STATE == 1 && millis() - TIME_ST >= 30000) {
    Serial.println("Tempo limite excedido. Fechando cancela.");
    CLOSE();
    return;
  }

  // Veículo saiu
  if (BTN_STATE == 1 && !PRES && !EMPTY) {
    Serial.println("Veículo saiu. Fechando cancela.");
    CLOSE();
    return;
  }
}

// Leitura do sensor de entrada
void MED() {
  if (millis() - LAST_MED >= INT_MED) {
    LAST_MED = millis();

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 20000);
    DIS = duration > 0 ? duration * 0.0343 / 2 : 400;
  }
}

// Fecha cancela e reseta estado
void CLOSE() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  BTN_STATE = 0;
  PRES = false;
  MSG_IN = false;
  OPEN = false;
  EMPTY = false;
  Serial.println("Cancela fechada.");
}

// Leitura do botão
void BTN() {
  if (digitalRead(BTN_PIN) == HIGH && BTN_STATE == 0) {
    delay(500);
    if(digitalRead(BTN_PIN) == LOW){
    BTN_STATE = 1; 
  }
}
}

// Atualiza presença de veículo
void CHC_PRES() {
  if (DIS <= 5 && !PRES) {
    PRES = true;
    MSG_IN = false;
    BTN_STATE = 0; 
    Serial.println("Veículo detectado na entrada, aperte o botão para abrir a cancela.");
  } else if (DIS > 15 && PRES) {
    PRES = false;
    Serial.println("Veículo saiu da entrada, fechando cancela.");
  CLOSE();
  }
}
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
bool AGUARDANDO_PAGAMENTO = false;
bool PAGAMENTO_CONFIRMADO = false;

// Tickets
String tickets[10];
int ticketCount = 0;
String ticketAtual = "";

// Protótipos
void MED();
void CLOSE();
void BTN();
void CHC_PRES();
String gerarID();
void verificarPagamento();

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

  // Verifica entrada de pagamento via Serial
  if (AGUARDANDO_PAGAMENTO && Serial.available()) {
    verificarPagamento();
  }

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

  // Impressão de ticket
  if (BTN_STATE == 1 && PRES && !OPEN && !AGUARDANDO_PAGAMENTO && !PAGAMENTO_CONFIRMADO) {
    ticketAtual = gerarID();
    if (ticketCount < 10) {
      tickets[ticketCount++] = ticketAtual;
    }

    Serial.println("Botão pressionado. Imprimindo ticket...");
    Serial.print("Ticket gerado: ");
    Serial.println(ticketAtual);
    Serial.println("Digite esse ID no Serial Monitor após pagamento.");

    AGUARDANDO_PAGAMENTO = true;
    BTN_STATE = 0;
    return;
  }

  // Tentativa de abrir sem pagar
  if (BTN_STATE == 1 && PRES && AGUARDANDO_PAGAMENTO && !PAGAMENTO_CONFIRMADO && !OPEN) {
    Serial.println("Pagamento pendente. Cancela não pode ser aberta.");
    BTN_STATE = 0;
    return;
  }

  // Tempo limite de cancela aberta
  if (OPEN && millis() - TIME_ST >= 30000) {
    Serial.println("Tempo limite excedido. Fechando cancela.");
    CLOSE();
    return;
  }

  // Veículo saiu
  if (OPEN && !PRES && !EMPTY) {
    Serial.println("Veículo saiu. Fechando cancela em 3 segundos...");
    delay(3000);
    CLOSE();
    return;
  }
}

// Leitura do sensor ultrassônico
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

// Fecha cancela e reseta variáveis
void CLOSE() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  BTN_STATE = 0;
  PRES = false;
  MSG_IN = false;
  OPEN = false;
  EMPTY = false;
  AGUARDANDO_PAGAMENTO = false;
  PAGAMENTO_CONFIRMADO = false;
  ticketAtual = "";
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
    Serial.println("Veículo detectado na entrada.");

    // Somente abre se já tiver pago
    if (PAGAMENTO_CONFIRMADO && !OPEN) {
      Serial.println("Pagamento verificado. Abrindo cancela.");
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, HIGH);
      OPEN = true;
      TIME_ST = millis();
    }

  } else if (DIS > 15 && PRES) {
    PRES = false;
    Serial.println("Veículo saiu da entrada.");
  }
}

// Geração de ID único simples
String gerarID() {
  return "TCK" + String(millis() % 100000);
}

// Verifica pagamento pelo Serial Monitor
void verificarPagamento() {
  String entrada = Serial.readStringUntil('\n');
  entrada.trim();

  for (int i = 0; i < ticketCount; i++) {
    if (tickets[i] == entrada) {
      Serial.println("Pagamento confirmado! Aguarde aproximação do veículo para abrir a cancela.");
      PAGAMENTO_CONFIRMADO = true;
      AGUARDANDO_PAGAMENTO = false;
      return;
    }
  }

  Serial.println("ID inválido. Ticket não reconhecido.");
}

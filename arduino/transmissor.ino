// transmissor.ino - CÓDIGO ATUALIZADO para usar 1 Arduino físico como 3 NÓS LÓGICOS

#include <SPI.h>
#include <RF24.h>
#include <DHT.h>
#include <Servo.h>
#include <ArduinoJson.h> // Adicionado para simulação, para serializar e ver dados

// --- Configurações NRF24L01 ---
#define CE_PIN   9
#define CSN_PIN  10
RF24 radio(CE_PIN, CSN_PIN); // Pino CE, Pino CSN
// Endereços de leitura/escrita para os 3 nós e o receptor (Pipe 0 do receptor, para enviar comandos)
// Cada nó lógico "escreve" para seu próprio endereço (que é um pipe de leitura no receptor)
// E cada nó lógico "lê" do endereço do receptor (para receber comandos)
const byte addresses[][6] = {"00001", "00002", "00003"}; // Endereços para os 3 nós lógicos
const byte receptorAddress[6] = "00000"; // Endereço do receptor para enviar comandos (pipe 0 do receptor)

// --- Configurações DHT11 ---
#define DHT_PIN    2     // Pino de dados do DHT11
#define DHT_TYPE   DHT11 // Tipo do sensor DHT (DHT11, DHT22)
DHT dht(DHT_PIN, DHT_TYPE);

// --- Configurações Atuadores (APENAS PARA NODE1 FÍSICO) ---
#define SERVO_PIN      3  // Pino do servo motor
#define BUZZER_PIN     4  // Pino do buzzer
#define LED_VERDE_PIN  5  // LED para indicar motor ligado/tudo OK
#define LED_VERMELHO_PIN 6  // LED para indicar alarme/anomalia

Servo myServo; // Objeto servo

// --- Estrutura de Dados ---
struct SensorData {
  char nodeID[10];
  float temperature;
  float humidity;
  bool motorOn;
  bool alarmOn;
};
SensorData dataToSend; // Estrutura para os dados a serem enviados

struct CommandData {
  char targetNodeID[10];
  char command[20];
};
CommandData receivedCommand; // Estrutura para os comandos recebidos

// --- Variáveis de Estado para CADA NÓ LÓGICO ---
// Usaremos arrays para simular estados independentes para cada nó lógico
bool motorStatus[3] = {true, true, true}; // true = ligado, false = desligado
bool alarmStatus[3] = {false, false, false}; // true = ligado, false = desligado

unsigned long lastSensorRead = 0;
const long sensorReadInterval = 2000; // Ler a cada 2 segundos e enviar dados para todos os nós lógicos
const int CRITICAL_TEMP = 30; // Temperatura crítica em °C

void setup() {
  Serial.begin(115200);
  Serial.println(F("Iniciando TRANSMISSOR MÚLTIPLO (1 físico -> 3 lógicos)"));

  dht.begin();
  myServo.attach(SERVO_PIN); // Atuadores só para o Node1 (físico)
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_VERMELHO_PIN, OUTPUT);

  // Inicializa o NRF24L01
  if (!radio.begin()) {
    Serial.println(F("Falha ao iniciar o módulo NRF24L01. Verifique as conexões."));
    while (1);
  }

  radio.setPALevel(RF24_PA_LOW); // Potência de transmissão
  radio.setDataRate(RF24_250KBPS); // Taxa de dados
  radio.setChannel(100); // Usaremos um canal fixo para o único rádio
  
  // O único rádio físico precisa "escrever" para todos os endereços dos nós lógicos
  // e "ler" de todos os endereços que o receptor usaria para enviar comandos.
  // Neste caso, o receptor envia para o endereço do nó lógico que ele quer controlar.
  // O transmissor precisa abrir pipes de leitura para *todos* os endereços dos nós lógicos
  // para poder receber comandos direcionados a qualquer um deles.
  radio.openReadingPipe(1, addresses[0]); // Pipe para comandos do Node1
  radio.openReadingPipe(2, addresses[1]); // Pipe para comandos do Node2
  radio.openReadingPipe(3, addresses[2]); // Pipe para comandos do Node3

  radio.startListening(); // Começa a ouvir por comandos

  // Configura estado inicial (apenas Node1 terá atuadores físicos)
  setMotorStatus(0, true); // Node1 motor ligado
  setAlarmStatus(0, false); // Node1 alarme desligado
  
  Serial.println(F("NRF24L01 Transmissor (Múltiplos Lógicos) configurado."));
}

void loop() {
  // --- Leitura de Sensores e Simulação de Dados para NÓS LÓGICOS ---
  if (millis() - lastSensorRead >= sensorReadInterval) {
    readAndSimulateNodes();
    lastSensorRead = millis();
  }

  // --- Verificação de Comandos Remotos do Receptor para QUALQUER NÓ LÓGICO ---
  checkRemoteCommands();
}

void readAndSimulateNodes() {
  // --- NODE1 (Real) ---
  float h1 = dht.readHumidity();
  float t1 = dht.readTemperature();

  if (isnan(h1) || isnan(t1)) {
    Serial.println(F("Erro ao ler do sensor DHT para Node1!"));
    t1 = 0; h1 = 0; // Valores padrão em caso de erro
  }

  // Lógica de atuação automática para Node1 (atuadores físicos)
  if (t1 > CRITICAL_TEMP) {
    if (motorStatus[0]) {
      setMotorStatus(0, false);
      Serial.println(F("Node1: TEMPERATURA CRÍTICA! Motor desligado."));
    }
    if (!alarmStatus[0]) {
      setAlarmStatus(0, true);
      Serial.println(F("Node1: ALARME ATIVADO devido à temperatura crítica!"));
    }
  } else {
    if (!motorStatus[0]) {
      setMotorStatus(0, true);
      Serial.println(F("Node1: Temperatura OK. Motor ligado."));
    }
    if (alarmStatus[0]) {
      setAlarmStatus(0, false);
      Serial.println(F("Node1: Temperatura OK. Alarme desativado."));
    }
  }

  // Envia dados para Node1
  sendNodeData("Node1", t1, h1, motorStatus[0], alarmStatus[0]);

  // --- NODE2 (Simulado) ---
  float t2 = random(20, 35); // Temperatura aleatória
  float h2 = random(30, 80); // Umidade aleatória
  // Simula lógica de atuação, mas sem atuadores físicos
  bool m2_status = motorStatus[1]; // Mantém o estado atual
  bool a2_status = alarmStatus[1];
  if (t2 > CRITICAL_TEMP + 2) { // Um pouco mais crítico para simular diferença
      m2_status = false;
      a2_status = true;
  } else {
      m2_status = true;
      a2_status = false;
  }
  sendNodeData("Node2", t2, h2, m2_status, a2_status);


  // --- NODE3 (Simulado) ---
  float t3 = random(18, 28); // Temperatura aleatória (geralmente mais baixa)
  float h3 = random(50, 60); // Umidade aleatória (geralmente estável)
  // Simula lógica de atuação, mas sem atuadores físicos
  bool m3_status = motorStatus[2]; // Mantém o estado atual
  bool a3_status = alarmStatus[2];
  if (h3 < 40) { // Exemplo de anomalia por umidade
      a3_status = true;
  } else {
      a3_status = false;
  }
  sendNodeData("Node3", t3, h3, m3_status, a3_status);

  Serial.println(F("-----------------------"));
}

void sendNodeData(const char* nodeID, float temp, float hum, bool motorOn, bool alarmOn) {
  radio.stopListening(); // Para de ouvir para poder transmitir

  // Copia o ID do nó para a estrutura
  String(nodeID).toCharArray(dataToSend.nodeID, 10);
  dataToSend.temperature = temp;
  dataToSend.humidity = hum;
  dataToSend.motorOn = motorOn;
  dataToSend.alarmOn = alarmOn;

  // Encontra o endereço do nó lógico para o qual os dados estão sendo enviados
  int nodeIndex = -1;
  if (String(nodeID) == "Node1") nodeIndex = 0;
  else if (String(nodeID) == "Node2") nodeIndex = 1;
  else if (String(nodeID) == "Node3") nodeIndex = 2;

  if (nodeIndex != -1) {
      radio.openWritingPipe(addresses[nodeIndex]); // Abre o pipe de escrita para o endereço deste nó lógico
      if (radio.write(&dataToSend, sizeof(SensorData))) {
          // Serial.print(F("Dados enviados para ")); Serial.println(nodeID);
      } else {
          Serial.print(F("Falha no envio dos dados para ")); Serial.println(nodeID);
      }
  }
  radio.startListening(); // Volta a ouvir
}


void checkRemoteCommands() {
  if (radio.available()) {
    radio.read(&receivedCommand, sizeof(CommandData));

    String targetNodeID = String(receivedCommand.targetNodeID);
    String cmd = String(receivedCommand.command);
    cmd.trim();

    Serial.print(F("Comando recebido para "));
    Serial.print(targetNodeID);
    Serial.print(F(": "));
    Serial.println(cmd);

    int nodeIndex = -1;
    if (targetNodeID == "Node1") nodeIndex = 0;
    else if (targetNodeID == "Node2") nodeIndex = 1;
    else if (targetNodeID == "Node3") nodeIndex = 2;

    if (nodeIndex != -1) {
      // Aplica o comando
      if (cmd == "LIGAR_MOTOR") {
        setMotorStatus(nodeIndex, true);
      } else if (cmd == "DESLIGAR_MOTOR") {
        setMotorStatus(nodeIndex, false);
      } else if (cmd == "ALARME_ON") {
        setAlarmStatus(nodeIndex, true);
      } else if (cmd == "ALARME_OFF") {
        setAlarmStatus(nodeIndex, false);
      } else {
        Serial.println(F("Comando desconhecido."));
      }
    } else {
      Serial.println(F("ID de nó alvo inválido para comando."));
    }
  }
}

void setMotorStatus(int nodeIndex, bool status) {
  motorStatus[nodeIndex] = status;
  // Apenas o Node1 tem atuadores físicos
  if (nodeIndex == 0) {
    if (motorStatus[0]) {
      myServo.write(180);
      digitalWrite(LED_VERDE_PIN, HIGH);
      digitalWrite(LED_VERMELHO_PIN, LOW); // Assumindo que LED Vermelho é para anomalia/alarme
    } else {
      myServo.write(0);
      digitalWrite(LED_VERDE_PIN, LOW);
      digitalWrite(LED_VERMELHO_PIN, HIGH); // LED Vermelho para indicar motor desligado
    }
  }
  // Se o alarme estiver ligado, mantém o LED Vermelho do Node1 ligado, independente do motor
  if (nodeIndex == 0 && alarmStatus[0]) {
      digitalWrite(LED_VERMELHO_PIN, HIGH);
  }
  Serial.print(F("Node")); Serial.print(nodeIndex + 1); Serial.print(F(": Motor -> ")); Serial.println(status ? F("LIGADO") : F("DESLIGADO"));
}

void setAlarmStatus(int nodeIndex, bool status) {
  alarmStatus[nodeIndex] = status;
  // Apenas o Node1 tem atuadores físicos
  if (nodeIndex == 0) {
    if (alarmStatus[0]) {
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(LED_VERMELHO_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      // Se o motor estiver desligado, o LED vermelho pode continuar ligado
      if (!motorStatus[0]) {
          digitalWrite(LED_VERMELHO_PIN, HIGH);
      } else {
          digitalWrite(LED_VERMELHO_PIN, LOW);
      }
    }
  }
  Serial.print(F("Node")); Serial.print(nodeIndex + 1); Serial.print(F(": Alarme -> ")); Serial.println(status ? F("ATIVADO") : F("DESATIVADO"));
}
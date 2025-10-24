// receptor.ino - Código para a Estação Central (Receptor)

#include <SPI.h>
#include <RF24.h>
#include <ArduinoJson.h>

// --- Configurações NRF24L01 ---
#define CE_PIN   9
#define CSN_PIN  10
RF24 radio(CE_PIN, CSN_PIN);

// Endereço único do receptor (para receber dados)
const byte receptorAddress[6] = "00000";

// Endereços de escrita para cada Node (para enviar comandos)
const byte nodeAddresses[][6] = {
  "00001", // Node1
  "00002", // Node2
  "00003"  // Node3
};

// --- Estrutura de Dados ---
struct SensorData {
  char nodeID[10];
  float temperature;
  float humidity;
  bool motorOn;
  bool alarmOn;
};
SensorData receivedData;

struct CommandData {
  char targetNodeID[10];
  char command[20];
};
CommandData commandToSend;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Iniciando Estação Central NRF24L01 Receptor."));

  if (!radio.begin()) {
    Serial.println(F("Falha ao iniciar o módulo NRF24L01. Verifique as conexões."));
    while (1);
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(100);

  // Abre apenas um pipe de leitura, que é o endereço do receptor
  radio.openReadingPipe(1, receptorAddress);
  radio.startListening();

  Serial.println(F("NRF24L01 Receptor configurado e ouvindo."));
  Serial.println(F("Para enviar comandos, digite no formato 'NodeX:COMANDO' (ex: Node1:LIGAR_MOTOR)"));
}

void loop() {
  readSensorData();
  processSerialCommands();
}

void readSensorData() {
  if (radio.available()) {
    radio.read(&receivedData, sizeof(SensorData));

    // Converte os dados recebidos para JSON e envia para a Serial
    StaticJsonDocument<200> doc;
    doc["timestamp"] = millis();
    doc["nodeID"] = receivedData.nodeID;
    doc["temperature"] = receivedData.temperature;
    doc["humidity"] = receivedData.humidity;
    doc["motorOn"] = receivedData.motorOn;
    doc["alarmOn"] = receivedData.alarmOn;

    serializeJson(doc, Serial);
    Serial.println();
  }
}

void processSerialCommands() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    int colonIndex = input.indexOf(':');
    if (colonIndex > 0) {
      String targetNode = input.substring(0, colonIndex);
      String command = input.substring(colonIndex + 1);

      targetNode.toCharArray(commandToSend.targetNodeID, 10);
      command.toCharArray(commandToSend.command, 20);

      int targetNodeIndex = -1;
      if (targetNode == "Node1") targetNodeIndex = 0;
      else if (targetNode == "Node2") targetNodeIndex = 1;
      else if (targetNode == "Node3") targetNodeIndex = 2;

      if (targetNodeIndex != -1) {
        radio.stopListening();
        radio.openWritingPipe(nodeAddresses[targetNodeIndex]);
        if (radio.write(&commandToSend, sizeof(CommandData))) {
          Serial.print(F("Comando '"));
          Serial.print(command);
          Serial.print(F("' enviado para "));
          Serial.println(targetNode);
        } else {
          Serial.print(F("Falha ao enviar comando '"));
          Serial.print(command);
          Serial.print(F("' para "));
          Serial.println(targetNode);
        }
        radio.startListening();
      } else {
        Serial.println(F("ID de nó inválido. Formato: NodeX:COMANDO"));
      }
    } else {
      Serial.println(F("Formato de comando inválido. Use NodeX:COMANDO"));
    }
  }
}

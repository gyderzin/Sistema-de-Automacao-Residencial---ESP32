#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Gree.h>

const uint16_t kIrLed = 18;  // Pino GPIO do ESP8266 a ser usado. Recomendado: 4 (D2).
IRGreeAC ac(kIrLed);  // Configuração do pino GPIO para envio de mensagens.

const int MAX_STORED_VALUES = 15;  // Número máximo de valores a serem armazenados
uint32_t storedValues[MAX_STORED_VALUES];  // Array para armazenar os valores
int storedValuesCount = 0;

void executeCommand(const String &command) {
if (command == "ligado") {
    ac.on();
  } else if (command == "desligado") {
    ac.off();
  } else if (command == "resfriamento") {
    ac.setMode(kGreeCool);
  } else if (command == "ventilador") {
    ac.setMode(kGreeFan);
  }  
   else if (command == "aumentar_ventilacao") {
    uint8_t fanSpeed = ac.getFan();
    ac.setFan(fanSpeed + 1);
  } else if (command == "automatizar_ventilacao") {
    ac.setFan(kGreeFanAuto);
  } else if (command == "direcao_automatica") {
    ac.setSwingVertical(true, kGreeSwingAuto);
  } else if (command == "direcao_fixa") {
    ac.setSwingVertical(true, kGreeSwingUp);  // Substitua pelo valor desejado
  } else if (command == "aumentar_temp") {
    ac.setTemp(ac.getTemp() + 1);
  } else if (command == "diminuir_temp") {
    ac.setTemp(ac.getTemp() - 1);
  }  else {
    Serial.println("Comando inválido.");
  }
  ac.send();
}

void setup() {
  ac.begin();
  ac.on();
  Serial.begin(115200);
  delay(200);
}

void loop() {
  if (Serial.available() > 0) {
    char buffer[20];  // Tamanho do buffer suficiente para armazenar uma entrada
    int bytesRead = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = '\0';  // Adiciona um terminador nulo ao final da string

    // Executa o comando correspondente ao valor recebido
    executeCommand(String(buffer));
  }
}

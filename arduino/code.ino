#include <String_Functions.h>
#include "SerialRelay.h"

// Configurações dinâmicas para a placa de relés
const uint8_t NUM_MODULES = 2;       // Número de módulos (ex.: 2)
const uint8_t RELAYS_PER_MODULE = 4; // Número de relés por módulo (ex.: 4)
const uint8_t BUFFER_SIZE = 10;      // Tamanho do buffer para comandos

SerialRelay relays(3, 2, NUM_MODULES); // (data pin, clock pin, número de módulos)
uint8_t currentModule = 1;            // Módulo ativo

char buffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;

// Configuração dos botões
const int NUM_BUTTONS = 8;
int buttonPins[NUM_BUTTONS] = {10, 11, 12, 13, 14, 15, 16, 17};
bool lastButtonStates[NUM_BUTTONS];

void setup() {
  Serial.begin(19200);
  delay(50); // Pequeno delay para estabilização

  // Configura os módulos com valores iniciais:
  // Módulo 1 com 0x00 e os demais com 0x55 (exemplo)
  for (uint8_t i = 1; i <= NUM_MODULES; i++) {
    if (i == 1) {
      relays.SetModule(0x00, i, false);
    } else {
      relays.SetModule(0x55, i);
    }
  }
  relays.Info(&Serial);
  bufferIndex = 0;

  // Configura os pinos dos botões como entrada com pull-up interno
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonStates[i] = digitalRead(buttonPins[i]);
  }
}

void loop() {
  // Processa comandos recebidos via Serial (terminados com '#')
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    if (inChar == '#') {
      buffer[bufferIndex] = '\0';  // Finaliza a string
      processCommand(buffer);
      bufferIndex = 0;             // Reseta o índice
    } else {
      if (bufferIndex < BUFFER_SIZE - 1) {
        buffer[bufferIndex++] = inChar;
      }
    }
  }
  
  // Verifica o estado de todos os botões
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool currentState = digitalRead(buttonPins[i]);
    if (lastButtonStates[i] == LOW && currentState == HIGH) {
      delay(100); // Aguarda 100ms para debounce
      // Verifica novamente o estado do botão
      if (digitalRead(buttonPins[i]) == HIGH) {
        Serial.print("BTN:");
        Serial.println(i + 1);
      }
    }
    lastButtonStates[i] = currentState;      
  }
}

// Função para processar comandos recebidos via Serial
void processCommand(char* cmd) {
  // Seleção de módulo: "mX" ou "MX" (X de 1 até NUM_MODULES)
  if (cmd[0] == 'm' || cmd[0] == 'M') {
    uint8_t mod = cmd[1] - '0';
    if (mod >= 1 && mod <= NUM_MODULES) {
      currentModule = mod;
      Serial.print("Módulo selecionado: ");
      Serial.println(currentModule);
    }
  }
  // Controle individual do relé: "+x" ou "-x" (x de 1 até RELAYS_PER_MODULE)
  else if (cmd[0] == '+' || cmd[0] == '-') {
    uint8_t relayNum = cmd[1] - '0';
    if (relayNum >= 1 && relayNum <= RELAYS_PER_MODULE) {
      byte state = (cmd[0] == '+') ? SERIAL_RELAY_ON : SERIAL_RELAY_OFF;
      relays.SetRelay(relayNum, state, currentModule);
      Serial.print("Set relay ");
      Serial.print(relayNum);
      Serial.print(" no módulo ");
      Serial.print(currentModule);
      Serial.print(" para o estado ");
      Serial.println(state);
    }
  }
  // Comando para configurar todos os relés do módulo com valor hexadecimal (ex.: "FF")
  else {
    if (strlen(cmd) >= 2) {
      byte value = 0;
      for (uint8_t i = 0; i < 2; i++) {
        char c = cmd[i];
        byte nibble = 0;
        if (c >= '0' && c <= '9')
          nibble = c - '0';
        else if (c >= 'A' && c <= 'F')
          nibble = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
          nibble = c - 'a' + 10;
        if (i == 0)
          value = nibble << 4;  // nibble alto
        else
          value |= nibble;      // nibble baixo
      }
      Serial.print("Valor hexadecimal recebido: ");
      Serial.println(value, HEX);
      relays.SetModule(value, currentModule);
    }
  }
}

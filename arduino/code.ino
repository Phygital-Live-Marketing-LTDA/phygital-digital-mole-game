#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <time.h>
#include <vector>
#include "String_Functions.h"
#include "SerialRelay.h"

using std::vector;

// ========================= Globais =========================
const int NUM_BUTTONS = 8;
int buttonPins[NUM_BUTTONS] = {14, 27, 26, 25, 33, 32, 35, 34};
bool lastButtonStates[NUM_BUTTONS];

// ========================= Configuração dos Relés e Botões =========================
const uint8_t NUM_MODULES = 2;       // Número de módulos (ex.: 2)
const uint8_t RELAYS_PER_MODULE = 4;  // Número de relés por módulo (ex.: 4)
const uint8_t BUFFER_SIZE = 10;       // Tamanho do buffer para comandos

unsigned long countdownStartTime = 0; // Início da contagem regressiva
bool animationStatus, animationEnabled = false;

// ========================= Lógica do Jogo e Ranking =========================
// Estados do jogo:
// 0 = Idle, 1 = Jogo, 2 = Tela de pontuação final, 3 = Tela de ranking,
// 4 = Contagem regressiva, 5 = Indicador de fim (relés acesos por 3s)
int gameState = 0;
bool gameRunning = false;
unsigned long gameStartTime = 0;
unsigned long stateChangeTime = 0;
const unsigned long gameDuration = 20000; // 20 segundos de jogo
uint16_t score = 0;

// Variáveis para agendamento da próxima rodada
unsigned long nextRoundTime = 0;
bool waitingNextRound = false;

// ========================= Configurações de Rede =========================
const char *ssid = "Phygital";
const char *password = "Eventstag";

// Instância do servidor assíncrono e do EventSource para SSE
AsyncWebServer server(80);
// Cria o objeto WebSocket no endpoint "/ws"
AsyncWebSocket ws("/ws");

// Inicializa o controlador de relés (pins: data em 13, clock em 12)
SerialRelay relays(13, 12, NUM_MODULES);
uint8_t currentModule = 1;  // Módulo ativo

char buffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;

// Estrutura que representa o "alvo" da rodada
struct Target {
  uint8_t module;
  uint8_t relay;
  uint8_t correctButton; // Para módulo 1: botões de 1 a 4 invertidos; para módulo 2: botões de 5 a 8 invertidos.
};

Target currentTarget;
Target previousTarget;
bool targetActive = false;

// -----------------------------------------------------------------------------
// Definição das sequências de animação (baseadas no novo arranjo)
// Exemplo para a animação "Spiral":
const uint8_t spiralSequence[] = { 1, 2, 8, 7, 3, 4, 6, 5 };
const size_t spiralSequenceCount = sizeof(spiralSequence) / sizeof(spiralSequence[0]);

// Exemplo para a animação "Snake":
const uint8_t snakeSequence[] = { 1, 2, 4, 8, 7, 5, 3, 1 };
const size_t snakeSequenceCount = sizeof(snakeSequence) / sizeof(snakeSequence[0]);

// -----------------------------------------------------------------------------
// Variáveis globais para controle das animações idle
enum IdleAnimType { SPIRAL, SNAKE };
IdleAnimType currentIdleAnim;
bool idleAnimActive = false;

// Para a animação Spiral:
bool spiralAnimRunning = false;
unsigned long spiralAnimStart = 0;
size_t spiralIndex = 0;

// Para a animação Snake:
bool snakeAnimRunning = false;
unsigned long snakeAnimStart = 0;
size_t snakeIndex = 0;

// ========================= Funções para Manipulação dos Relés =========================
// Liga todos os relés de todos os módulos
void turnOnAllRelays() {
  for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
    for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
      relays.SetRelay(r, SERIAL_RELAY_ON, mod);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
}

// Desliga todos os relés de todos os módulos
void turnOffAllRelays() {
  for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
    for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
      relays.SetRelay(r, SERIAL_RELAY_OFF, mod);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
}

// ========================= Função para Contagem Regressiva =========================
// Inicia a contagem regressiva para início do jogo
void triggerCountdown() {
  if (gameState == 0) {  // Somente se estiver em idle
    gameState = 4;       // Estado 4: contagem regressiva
    countdownStartTime = millis();
  }
}

// ========================= Função para Iniciar o Jogo =========================
void startGame() {
  gameState = 1; // Jogo em andamento
  score = 0;
  gameRunning = true;
  gameStartTime = millis();
  nextRound();
}

// ========================= Função para Gerar a Próxima Rodada =========================
void nextRound() {
  // Se o tempo do jogo expirou, não inicia nova rodada.
  if (millis() - gameStartTime >= gameDuration) return;
  
  uint8_t mod, relay, correctButton;
  uint8_t attempts = 0;
  
  // Tenta até 10 vezes obter uma combinação diferente da anterior
  do {
    mod = random(1, NUM_MODULES + 1);
    relay = random(1, RELAYS_PER_MODULE + 1);
    correctButton = (mod == 1) ? relay : relay + 4;
    attempts++;
  } while ((mod == previousTarget.module && relay == previousTarget.relay) && (attempts < 10));
  
  previousTarget = { mod, relay, correctButton };
  currentTarget = previousTarget;
  targetActive = true;
  
  Serial.printf("Nova rodada: Módulo %d, Relé %d, Botão correto: %d (tentativas: %d)\n", mod, relay, correctButton, attempts);
  relays.SetRelay(relay, SERIAL_RELAY_ON, mod);
}

// ========================= Função para Finalizar o Jogo =========================
void endGame() {
  gameRunning = false;
  targetActive = false;
  turnOffAllRelays();
  Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
  gameState = 2; // Tela de pontuação final
  stateChangeTime = millis();
}

// ========================= Atualiza o Estado do Jogo =========================
void updateGameState() {
  if(gameState > 1) {
    animationStatus = false;
  }
  // Durante o jogo (estado 1), se o tempo expirou, transita para o estado 5 (fim com relés acesos)
  if (gameState == 1 && (millis() - gameStartTime >= gameDuration)) {
    turnOnAllRelays();
    gameState = 5;
    stateChangeTime = millis();
  }
  // No estado 5, após 3 segundos, desliga os relés e passa para a tela de pontuação final (estado 2)
  else if (gameState == 5 && (millis() - stateChangeTime >= 3000)) {
    turnOffAllRelays();
    Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
    gameState = 2;
    stateChangeTime = millis();
  }
  // Na tela de pontuação final (estado 2), após 5 segundos, transita para ranking (estado 3)
  else if (gameState == 2 && (millis() - stateChangeTime >= 5000)) {
    gameState = 3;
    stateChangeTime = millis();
  }
  // Na tela de ranking (estado 3), após 10 segundos, reseta para idle (estado 0)
  else if (gameState == 3 && (millis() - stateChangeTime >= 10000)) {
    gameState = 0;
    score = 0;
    animationStatus = animationEnabled;
  }
}

// ========================= Processamento de Comandos (Serial) =========================
/**
 * processCommand - Processa um comando recebido (via serial ou API).
 *
 * O comando pode ter três formatos:
 * 1. "mX" ou "MX": seleciona o módulo X (onde X é um dígito).
 * 2. "+Y" ou "-Y": liga ("+") ou desliga ("-") o relé Y (onde Y é um dígito)
 *    no módulo atualmente selecionado.
 * 3. Dois caracteres hexadecimais: interpreta como um valor hexadecimal e o
 *    utiliza para configurar o módulo (neste exemplo, seta o módulo para o valor).
 */
void processCommand(const char *cmd) {
  // Caso o comando inicie com 'm' ou 'M', seleciona o módulo.
  if (cmd[0] == 'm' || cmd[0] == 'M') {
    uint8_t mod = cmd[1] - '0';
    if (mod >= 1 && mod <= NUM_MODULES) {
      currentModule = mod;
      Serial.printf("Módulo selecionado: %d\n", currentModule);
    }
  }
  // Se o comando iniciar com '+' ou '-', trata como comando de relé.
  else if (cmd[0] == '+' || cmd[0] == '-') {
    uint8_t relayNum = cmd[1] - '0';
    if (relayNum >= 1 && relayNum <= RELAYS_PER_MODULE) {
      // Define o estado: SERIAL_RELAY_ON se '+', SERIAL_RELAY_OFF se '-'
      byte state = (cmd[0] == '+') ? SERIAL_RELAY_ON : SERIAL_RELAY_OFF;
      relays.SetRelay(relayNum, state, currentModule);
      Serial.printf("Set relay %d no módulo %d para o estado %d\n", relayNum, currentModule, state);
    }
  }
  // Se o comando tiver pelo menos 2 caracteres e não corresponder aos casos acima,
  // interpreta como um valor hexadecimal.
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
          value = nibble << 4;  // Nibble de alta ordem
        else
          value |= nibble;      // Nibble de baixa ordem
      }
      Serial.printf("Valor hexadecimal recebido: %X\n", value);
      // Configura o módulo com o valor calculado.
      // Se a intenção for realmente utilizar o valor lido, use 'value' em vez de 0x00.
      relays.SetModule(value, currentModule);
    }
  }
}

// ========================= Função para Processamento dos Botões =========================
void setButtonsToResponse() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool currentState = digitalRead(buttonPins[i]);
    if (lastButtonStates[i] == LOW && currentState == HIGH) {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      if (digitalRead(buttonPins[i]) == HIGH) {
        uint8_t buttonPressed = i + 1;
        Serial.printf("Botão pressionado: %d\n", buttonPressed);
        if (targetActive && buttonPressed == currentTarget.correctButton) {
          score++;
          Serial.printf("Resposta correta! Pontuação: %d\n", score);
          relays.SetRelay(currentTarget.relay, SERIAL_RELAY_OFF, currentTarget.module);
          targetActive = false;
          waitingNextRound = true;
          nextRoundTime = millis() + 300;
        } else {
          Serial.println(F("Resposta incorreta."));
        }
      }
    }
    lastButtonStates[i] = currentState;
  }
}

// ========================= Função para Contagem Regressiva =========================
void handleCountdown() {
  unsigned long elapsed = millis() - countdownStartTime;
  if (elapsed < 3000) {
    // Durante a contagem, os relés piscam: 500ms ligados, 500ms apagados
    if ((elapsed % 1000) < 500) {
      turnOnAllRelays();
    } else {
      turnOffAllRelays();
    }
  } else {
    // Ao fim dos 3 segundos, apaga todos os relés e inicia o jogo
    turnOffAllRelays();
    startGame(); // Muda para estado 1 e registra gameStartTime
  }
}

// ========================= Task: Lógica do Jogo =========================
void gameLogicTask(void * parameter) {
  for (;;) {
    updateGameState();
    
    // Enquanto o jogo estiver idle e animações estiverem habilitadas, executa animações
    if (gameState == 0 && animationStatus) {
      updateIdleAnimation();
    }
    
    // Durante o jogo (estado 1), processa os botões para respostas
    if (gameState == 1) {
      setButtonsToResponse();
    }
    
    // Processa a contagem regressiva (estado 4)
    if (gameState == 4) {
      handleCountdown();
    }
    
    // Processa a próxima rodada se agendada
    if (waitingNextRound && millis() >= nextRoundTime) {
      waitingNextRound = false;
      nextRound();
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// -----------------------------------------------------------------------------
// Helper: Converte número de LED (1 a 8) para módulo e relé.
// LEDs 1-4: módulo 1, relé = LED; LEDs 5-8: módulo 2, relé = LED - 4.
void setLed(uint8_t led, byte state) {
  if (led >= 1 && led <= 4) {
    relays.SetRelay(led, state, 1);
  } else if (led >= 5 && led <= 8) {
    relays.SetRelay(led - 4, state, 2);
  }
}

// -----------------------------------------------------------------------------
// Funções para animação "Spiral"
// Inicia a animação Spiral: reseta o estado e desliga todos os LEDs.
void startSpiralAnimation() {
  spiralAnimRunning = true;
  spiralAnimStart = millis();
  spiralIndex = 0;
  turnOffAllRelays();
  Serial.println("Spiral animation iniciada");
}

// Atualiza a animação Spiral de forma não bloqueante.
// A cada 300ms, acende o próximo LED da sequência e desliga o anterior para criar um efeito de trilha.
void updateSpiralAnimation() {
  if (!spiralAnimRunning) return;
  unsigned long now = millis();
  // Intervalo de 300ms por passo
  if (now - spiralAnimStart >= spiralIndex * 300UL) {
    if (spiralIndex > 0) {
      // Desliga o LED anterior para criar o efeito de "trilho"
      setLed(spiralSequence[spiralIndex - 1], SERIAL_RELAY_OFF);
    }
    if (spiralIndex < spiralSequenceCount) {
      setLed(spiralSequence[spiralIndex], SERIAL_RELAY_ON);
      spiralIndex++;
    } else {
      // Após a última etapa, aguarda 500ms e encerra a animação
      if (now - spiralAnimStart >= spiralSequenceCount * 300UL + 500) {
        setLed(spiralSequence[spiralSequenceCount - 1], SERIAL_RELAY_OFF);
        spiralAnimRunning = false;
        Serial.println("Spiral animation finalizada");
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Funções para animação "Snake"
// Inicia a animação Snake: reseta o estado e desliga todos os LEDs.
void startSnakeAnimation() {
  snakeAnimRunning = true;
  snakeAnimStart = millis();
  snakeIndex = 0;
  turnOffAllRelays();
  Serial.println("Snake animation iniciada");
}

// Atualiza a animação Snake de forma não bloqueante.
void updateSnakeAnimation() {
  if (!snakeAnimRunning) return;
  unsigned long now = millis();
  // Intervalo de 300ms por etapa
  if (now - snakeAnimStart >= snakeIndex * 300UL) {
    if (snakeIndex > 0) {
      setLed(snakeSequence[snakeIndex - 1], SERIAL_RELAY_OFF);
    }
    if (snakeIndex < snakeSequenceCount) {
      setLed(snakeSequence[snakeIndex], SERIAL_RELAY_ON);
      snakeIndex++;
    } else {
      if (now - snakeAnimStart >= snakeSequenceCount * 300UL + 500) {
        setLed(snakeSequence[snakeSequenceCount - 1], SERIAL_RELAY_OFF);
        snakeAnimRunning = false;
        Serial.println("Snake animation finalizada");
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Função para escolher aleatoriamente uma animação idle
void chooseIdleAnimation() {
  currentIdleAnim = (IdleAnimType) random(0, 2); // Escolhe 0 ou 1
  idleAnimActive = true;
  // Inicia a animação escolhida
  if (currentIdleAnim == SPIRAL) {
    startSpiralAnimation();
  } else {
    startSnakeAnimation();
  }
}

// Atualiza a animação idle com base no tipo escolhido
void updateIdleAnimation() {
  if (!idleAnimActive) {
    chooseIdleAnimation();
  }
  if (currentIdleAnim == SPIRAL) {
    updateSpiralAnimation();
    if (!spiralAnimRunning) idleAnimActive = false;
  } else { // SNAKE
    updateSnakeAnimation();
    if (!snakeAnimRunning) idleAnimActive = false;
  }
}

// Função para tratar os eventos do WebSocket
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
  void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && info->index == 0 && info->len == len) {
        // Converte os dados recebidos para uma String
        String msg = "";
        for (size_t i = 0; i < len; i++) {
          msg += (char)data[i];
        }
        Serial.print("Mensagem recebida: ");
        Serial.println(msg);
        // Verifica se a mensagem é "start-game" para iniciar o jogo
        if (msg == "start-game") {
          triggerCountdown();
        }
      }
    }
}

// ========================= Setup =========================
void setup() {
  Serial.begin(115200);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  randomSeed(esp_random());

  if (!SPIFFS.begin(true)) {
    Serial.println(F("Falha ao montar SPIFFS"));
    return;
  }
  
  // Conecta ao WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando-se ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP do ESP: ");
  Serial.println(WiFi.localIP().toString());
  
  // Define o evento do WebSocket
  ws.onEvent(onWsEvent);
  // Adiciona o handler do WebSocket no servidor
  server.addHandler(&ws);

  // Inicia o servidor
  server.begin();
  Serial.println(F("Servidor web iniciado."));

  // Configura os pinos dos botões de controle
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonStates[i] = digitalRead(buttonPins[i]);
  }
  
  // Inicializa os módulos dos relés (desligados)
  for (uint8_t i = 1; i <= NUM_MODULES; i++) {
    relays.SetModule(0x00, i, false);
  }
  relays.Info(&Serial);
  bufferIndex = 0;
  
  gameState = 0;
  gameRunning = false;
  
  turnOffAllRelays();

  // Cria a task para a lógica do jogo
  xTaskCreate(
    gameLogicTask,     // Função da task
    "GameLogic",       // Nome da task
    4096,              // Tamanho da stack
    NULL,              // Parâmetro
    1,                 // Prioridade
    NULL               // Handle
  );
}

// ========================= Loop Principal =========================
void loop() {
  // Cria o documento JSON com os dados dinâmicos
  DynamicJsonDocument doc(128);
  doc["score"] = score;
  doc["state"] = gameState;
  
  // Serializa o JSON para uma String
  String data;
  serializeJson(doc, data);

  // Envia o JSON para todos os clientes conectados via WebSocket
  ws.textAll(data);

  vTaskDelay(100 / portTICK_PERIOD_MS);
}
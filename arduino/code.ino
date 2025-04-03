#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ===== Configurações do MCP23017 =====
#define MCP_ADDRESS 0x20   // Endereço do MCP23017
#define IODIRA 0x00        // Configura todos os pinos da porta A como saída
#define OLATA  0x14        // Registrador de saída da porta A

uint8_t currentOutput;     // Estado atual dos 8 pinos da porta A

// Escrita no MCP23017 usando escrita em bloco
void writeBlockData(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(MCP_ADDRESS);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

void initMCP23017() {
  Wire.begin();
  // Configura todos os 8 pinos da porta A como saída
  Wire.beginTransmission(MCP_ADDRESS);
  Wire.write(IODIRA);
  Wire.write(0x00);  // 0 = saída para todos os pinos
  Wire.endTransmission();
  
  // Inicializa com todos os relés desligados (relés ativos em nível baixo: 1 = ligado)
  currentOutput = 0xFF;
  writeBlockData(OLATA, currentOutput);
}

// Função para controlar um relé específico (relé 1 -> bit 7, relé 8 -> bit 0)
void setRelay(uint8_t relay, bool state) {
  if (relay < 1 || relay > 8) return;
  uint8_t mask = 1 << (8 - relay);
  if (state) {
    // Liga o relé: ativa em nível baixo (limpa o bit)
    currentOutput &= ~mask;
  } else {
    // Desliga o relé: seta o bit
    currentOutput |= mask;
  }
  writeBlockData(OLATA, currentOutput);
}

void turnOnAllRelays() {
  currentOutput = 0x00;
  writeBlockData(OLATA, currentOutput);
}

void turnOffAllRelays() {
  currentOutput = 0xFF;
  writeBlockData(OLATA, currentOutput);
}

// ========================= Configurações Gerais =========================
unsigned long lastWsUpdate = 0;
unsigned long lastHeapCheck = 0;
const int NUM_BUTTONS = 8;
const int buttonPins[NUM_BUTTONS] = {14, 27, 26, 25, 33, 32, 35, 34};
bool lastButtonStates[NUM_BUTTONS];

unsigned long countdownStartTime = 0;
bool animationStatus, animationEnabled = true;

int gameState = 3;  // 0: Idle (formulário), 1: Jogo, 2: Pontuação Final, 3: Ranking, 4: Contagem Regressiva, 5: Fim (todos os relés acesos)
bool gameRunning = false;
unsigned long gameStartTime = 0;
unsigned long stateChangeTime = 0;
const unsigned long gameDuration = 21000; // 21 segundos
uint16_t score = 0;

unsigned long nextRoundTime = 0;
bool waitingNextRound = false;

// ========================= Configurações de Rede =========================
const char *ssid = "Phygital Solutions";
const char *password = "@Phygital27";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ========================= Lógica do Jogo =========================
struct Target {
  uint8_t relay;         // Relé alvo (1 a 8)
  uint8_t correctButton; // Botão correto (1 a 8)
};

Target currentTarget;
Target previousTarget;
bool targetActive = false;

void triggerCountdown() {
  gameState = 4; // Estado de contagem regressiva
  countdownStartTime = millis();
  Serial.println("Iniciando contagem regressiva");
}

void startGame() {
  gameState = 1;
  score = 0;
  gameRunning = true;
  gameStartTime = millis();
  waitingNextRound = false;
  nextRound();
}

void nextRound() {
  if (targetActive || millis() - gameStartTime >= gameDuration) return;

  uint8_t relay;
  uint8_t attempts = 0;
  do {
    relay = random(1, 9); // Relé de 1 a 8
    attempts++;
  } while (relay == previousTarget.relay && attempts < 10);

  previousTarget = { relay, relay };
  currentTarget = previousTarget;
  targetActive = true;
  Serial.printf("Nova rodada: Relé %d, Botão correto: %d (tentativas: %d)\n", relay, relay, attempts);

  setRelay(relay, true); // Liga o relé e espera até o jogador apertar o botão correto
}

void endGame() {
  gameRunning = false;
  targetActive = false;
  turnOffAllRelays();
  Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
  gameState = 2;
  stateChangeTime = millis();
}

void updateGameState() {
  if (gameState != 3 && animationStatus == true) {
    animationStatus = false;
    turnOffAllRelays(); // Desliga todos os relés
  };
  
  // Se o tempo do jogo acabou, liga todos os relés por 3s e muda o estado
  if (gameState == 1 && (millis() - gameStartTime >= gameDuration)) {
    turnOnAllRelays();
    gameState = 5;
    stateChangeTime = millis();
  }
  else if (gameState == 5 && (millis() - stateChangeTime >= 3000)) {
    turnOffAllRelays();
    Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
    gameState = 2;
    stateChangeTime = millis();
  }
  else if (gameState == 2 && (millis() - stateChangeTime >= 5000)) {
    gameState = 3;
    stateChangeTime = millis();
    // No estado de ranking, reativa animações
    animationStatus = animationEnabled;
  }
  // Sem transição automática do ranking para o estado idle
}

void handleCountdown() {
  unsigned long elapsed = millis() - countdownStartTime;
  if (elapsed < 3000) {
    // Liga e desliga todos os relés a cada 500ms durante a contagem regressiva
    unsigned long phase = (elapsed / 500) % 2;
    static unsigned long lastPhase = 99;
    if (phase != lastPhase) {
      lastPhase = phase;
      if (phase == 0)
        turnOnAllRelays();
      else
        turnOffAllRelays();
    }
  } else {
    startGame();
  }
}

// ========================= Animações Idle =========================
// Sequências de animação usando os relés (de 1 a 8)
// --- Variáveis e Sequências (permanece igual) ---
const uint8_t spiralSequence[] = { 1, 2, 8, 7, 3, 4, 6, 5 };
const size_t spiralSequenceCount = sizeof(spiralSequence) / sizeof(spiralSequence[0]);
const uint8_t snakeSequence[] = { 1, 2, 4, 8, 7, 5, 3, 1 };
const size_t snakeSequenceCount = sizeof(snakeSequence) / sizeof(snakeSequence[0]);

enum IdleAnimType { SPIRAL, SNAKE };
IdleAnimType currentIdleAnim;

// --- Remova as variáveis "idleAnimActive" e os indicadores de animação anterior ---

// Função auxiliar para acionar um LED (permanece igual)
void setLed(uint8_t led, bool state) {
  setRelay(led, state);
}

// Função que executa a animação spiral até o fim
void runSpiralAnimation() {
  bool spiralAnimRunning = true;
  unsigned long spiralAnimStart = millis();
  size_t spiralIndex = 0;
  turnOffAllRelays();
  Serial.println("Spiral animation iniciada");

  while (spiralAnimRunning) {
    unsigned long now = millis();
    if (now - spiralAnimStart >= spiralIndex * 300UL) {
      if (spiralIndex > 0)
        setLed(spiralSequence[spiralIndex - 1], false);
      if (spiralIndex < spiralSequenceCount) {
        setLed(spiralSequence[spiralIndex], true);
        spiralIndex++;
      } else if (now - spiralAnimStart >= spiralSequenceCount * 300UL + 500) {
        setLed(spiralSequence[spiralSequenceCount - 1], false);
        spiralAnimRunning = false;
        Serial.println("Spiral animation finalizada");
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// Função que executa a animação snake até o fim
void runSnakeAnimation() {
  bool snakeAnimRunning = true;
  unsigned long snakeAnimStart = millis();
  size_t snakeIndex = 0;
  turnOffAllRelays();
  Serial.println("Snake animation iniciada");

  while (snakeAnimRunning) {
    unsigned long now = millis();
    if (now - snakeAnimStart >= snakeIndex * 300UL) {
      if (snakeIndex > 0)
        setLed(snakeSequence[snakeIndex - 1], false);
      if (snakeIndex < snakeSequenceCount) {
        setLed(snakeSequence[snakeIndex], true);
        snakeIndex++;
      } else if (now - snakeAnimStart >= snakeSequenceCount * 300UL + 500) {
        setLed(snakeSequence[snakeSequenceCount - 1], false);
        snakeAnimRunning = false;
        Serial.println("Snake animation finalizada");
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// Função que escolhe e executa uma animação idle até o fim
void runIdleAnimation() {
  currentIdleAnim = (IdleAnimType) random(0, 2);
  if (currentIdleAnim == SPIRAL) {
    runSpiralAnimation();
  } else {
    runSnakeAnimation();
  }
}

// ========================= Leitura dos Botões =========================
// Apenas a tarefa taskButtonRead realizará a leitura dos botões, evitando duplicação.
void taskButtonRead(void *pvParameters) {
  for (;;) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
      bool currentState = digitalRead(buttonPins[i]);
      // Detecta transição de LOW para HIGH (botão pressionado)
      if (lastButtonStates[i] == LOW && currentState == HIGH) {
        vTaskDelay(50 / portTICK_PERIOD_MS);  // Debounce
        if (digitalRead(buttonPins[i]) == HIGH) {
          uint8_t buttonPressed = i + 1;
          if (targetActive && buttonPressed == currentTarget.correctButton) {
            score++;
            setRelay(currentTarget.relay, false);
            targetActive = false;
            waitingNextRound = true;
            nextRoundTime = millis() + 300;
          }
        }
      }      
      lastButtonStates[i] = currentState;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Tarefa para alternar os relés sem bloquear o loop principal
void toggleRelaysTask(void * parameter) {
  animationStatus = false;
  turnOnAllRelays();
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  turnOffAllRelays();
  animationStatus = animationEnabled;
  vTaskDelete(NULL); // Encerra a tarefa
}

// ========================= WebSocket =========================
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len) {
      // Utilizando buffer fixo para evitar alocações dinâmicas desnecessárias
      char msgBuffer[64] = {0};
      size_t copyLen = (len < sizeof(msgBuffer) - 1 ? len : sizeof(msgBuffer) - 1);
      memcpy(msgBuffer, data, copyLen);
      msgBuffer[copyLen] = '\0';
      Serial.print("Mensagem recebida: ");
      Serial.println(msgBuffer);
      
      if (strcmp(msgBuffer, "show-form") == 0) {
        if (gameState == 3) { // Ranking
          gameState = 0; // Volta para o estado Idle (formulário)
          Serial.println("Mudando para formulário de leads (IDLE)");
        }
      }
      else if (strcmp(msgBuffer, "start-countdown") == 0 || strcmp(msgBuffer, "start-game") == 0) {
        // Se estiver no Ranking ou Idle, inicia a contagem regressiva
        if (gameState == 3 || gameState == 0) {
          triggerCountdown();
        }
      } else if (strcmp(msgBuffer, "toggle-relays") == 0) {
        // Cria a tarefa para alternar os relés sem bloquear o loop principal
        xTaskCreatePinnedToCore(
          toggleRelaysTask,   // Função da tarefa
          "ToggleRelaysTask", // Nome da tarefa
          2048,               // Tamanho da stack
          NULL,               // Parâmetro (não utilizado)
          1,                  // Prioridade
          NULL,               // Handle da tarefa (não utilizado)
          1                   // Núcleo onde a tarefa será executada
        );
      }
    }
  }
}

void taskWebSocket(void *pvParameters) {
  for (;;) {
    if (millis() - lastWsUpdate >= 100) {
      lastWsUpdate = millis();
      // Criação de JSON com alocação estática
      StaticJsonDocument<128> doc;
      doc["score"] = score;
      doc["state"] = gameState;
      if (gameState == 1)
        doc["timer"] = (gameDuration - (millis() - gameStartTime)) / 1000;
      else if (gameState == 4)
        doc["timer"] = (3000 - (millis() - countdownStartTime)) / 1000;
      else
        doc["timer"] = 0;
      
      char jsonBuffer[128];
      size_t jsonSize = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
      if (jsonSize > 0) {
        ws.textAll(jsonBuffer);
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ========================= Monitoramento de Heap =========================
// Imprime o valor de memória livre a cada 60 segundos
void taskHeapMonitor(void *pvParameters) {
  for (;;) {
    if (millis() - lastHeapCheck >= 60000) {
      lastHeapCheck = millis();
      Serial.printf("Heap livre: %d bytes\n", ESP.getFreeHeap());
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// ========================= Setup e Loop =========================
void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Inicializa o MCP23017
  initMCP23017();

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
  
  // Configura o WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("Servidor web iniciado.");
  
  // Configura os pinos dos botões
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonStates[i] = digitalRead(buttonPins[i]);
  }
  
  // Estado inicial do jogo e dos relés
  gameState = 3; // Inicia no estado de Ranking
  gameRunning = false;
  turnOffAllRelays();
  animationStatus = animationEnabled;
  
  // Cria as tarefas
  xTaskCreatePinnedToCore(taskWebSocket, "WebSocket", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskButtonRead, "ButtonRead", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskHeapMonitor, "HeapMonitor", 2048, NULL, 1, NULL, 0);
}

void loop() {
  updateGameState();
  
  if (gameState == 4) {
    handleCountdown();
  }
  
  if (waitingNextRound && millis() >= nextRoundTime && !targetActive) {
    waitingNextRound = false;
    nextRound();
  }  
  
  // Executa a animação idle somente se estiver no estado 3 (Ranking)
  if (gameState == 3 && animationStatus) {
    runIdleAnimation();
  }
  
  vTaskDelay(50 / portTICK_PERIOD_MS);
}
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ===== Configurações do MCP23017 =====
#define MCP_ADDRESS 0x20   // Endereço do MCP23017
#define IODIRA 0x00        // Configura todos os pinos da porta A como saída
#define OLATA  0x14        // Registrador de saída da porta A

uint8_t currentOutput;     // Estado atual dos 8 pinos da porta A

void writeBlockData(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(MCP_ADDRESS);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
  delay(10);
}

void initMCP23017() {
  Wire.begin();
  // Configura todos os 8 pinos da porta A como saída
  Wire.beginTransmission(MCP_ADDRESS);
  Wire.write(IODIRA);
  Wire.write(0x00);  // 0 = saída para todos os pinos
  Wire.endTransmission();
  
  // Inicializa com todos os relés desligados (relés ativos em nível baixo: 1 = desligado)
  currentOutput = 0xFF;
  writeBlockData(OLATA, currentOutput);
}

// Função para controlar um relé específico (relé 1 corresponde ao bit 7, relé 2 ao bit 6, ... relé 8 ao bit 0)
void setRelay(uint8_t relay, bool state) {
  if (relay < 1 || relay > 8) return;
  uint8_t mask = 1 << (8 - relay);  // Mapeamento: relé 1 -> bit 7, relé 8 -> bit 0
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
  for (uint8_t r = 1; r <= 8; r++) {
    setRelay(r, true);
  }
}

void turnOffAllRelays() {
  for (uint8_t r = 1; r <= 8; r++) {
    setRelay(r, false);
  }
}

// ========================= Configurações Gerais =========================
unsigned long lastWsUpdate = 0;
const int NUM_BUTTONS = 8;
int buttonPins[NUM_BUTTONS] = {14, 27, 26, 25, 33, 32, 35, 34};
bool lastButtonStates[NUM_BUTTONS];

unsigned long countdownStartTime = 0;
bool animationStatus, animationEnabled = true;

int gameState = 0;  // 0: Idle, 1: Jogo, 2: Pontuação Final, 3: Ranking, 4: Contagem Regressiva, 5: Fim (todos os relés acesos)
bool gameRunning = false;
unsigned long gameStartTime = 0;
unsigned long stateChangeTime = 0;
const unsigned long gameDuration = 21000; // 21 segundos
uint16_t score = 0;

unsigned long nextRoundTime = 0;
bool waitingNextRound = false;

// ========================= Configurações de Rede =========================
const char *ssid = "Phygital";
const char *password = "Eventstag";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ========================= Lógica do Jogo =========================
// Neste exemplo, o jogo trabalha com 8 relés: o "alvo" é um relé específico (1 a 8) e o botão correto é o mesmo número.
struct Target {
  uint8_t relay;         // Relé alvo (1 a 8)
  uint8_t correctButton; // Botão correto (1 a 8)
};

Target currentTarget;
Target previousTarget;
bool targetActive = false;

void triggerCountdown() {
  if (gameState == 0) {
    gameState = 4;
    countdownStartTime = millis();
  }
}

void startGame() {
  gameState = 1;
  score = 0;
  gameRunning = true;
  gameStartTime = millis();
  // Inicia a primeira rodada
  nextRound();
}

void nextRound() {
  if (millis() - gameStartTime >= gameDuration) return;
  
  uint8_t relay;
  uint8_t attempts = 0;
  do {
    relay = random(1, 9); // Relé de 1 a 8
    attempts++;
  } while (relay == previousTarget.relay && (attempts < 10));
  
  previousTarget = { relay, relay };
  currentTarget = previousTarget;
  targetActive = true;
  
  Serial.printf("Nova rodada: Relé %d, Botão correto: %d (tentativas: %d)\n", relay, relay, attempts);
  setRelay(relay, true);
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
  if (gameState > 1) animationStatus = false;
  
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
  }
  else if (gameState == 3 && (millis() - stateChangeTime >= 10000)) {
    gameState = 0;
    score = 0;
    animationStatus = animationEnabled;
  }
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
const uint8_t spiralSequence[] = { 1, 2, 8, 7, 3, 4, 6, 5 };
const size_t spiralSequenceCount = sizeof(spiralSequence) / sizeof(spiralSequence[0]);
const uint8_t snakeSequence[] = { 1, 2, 4, 8, 7, 5, 3, 1 };
const size_t snakeSequenceCount = sizeof(snakeSequence) / sizeof(snakeSequence[0]);

enum IdleAnimType { SPIRAL, SNAKE };
IdleAnimType currentIdleAnim;
bool idleAnimActive = false;

bool spiralAnimRunning = false;
unsigned long spiralAnimStart = 0;
size_t spiralIndex = 0;

bool snakeAnimRunning = false;
unsigned long snakeAnimStart = 0;
size_t snakeIndex = 0;

void setLed(uint8_t led, bool state) {
  setRelay(led, state);
}

void startSpiralAnimation() {
  spiralAnimRunning = true;
  spiralAnimStart = millis();
  spiralIndex = 0;
  turnOffAllRelays();
  Serial.println("Spiral animation iniciada");
}

void updateSpiralAnimation() {
  if (!spiralAnimRunning) return;
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
}

void startSnakeAnimation() {
  snakeAnimRunning = true;
  snakeAnimStart = millis();
  snakeIndex = 0;
  turnOffAllRelays();
  Serial.println("Snake animation iniciada");
}

void updateSnakeAnimation() {
  if (!snakeAnimRunning) return;
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
}

void chooseIdleAnimation() {
  currentIdleAnim = (IdleAnimType) random(0, 2);
  idleAnimActive = true;
  if (currentIdleAnim == SPIRAL)
    startSpiralAnimation();
  else
    startSnakeAnimation();
}

void updateIdleAnimation() {
  if (!idleAnimActive)
    chooseIdleAnimation();
  
  if (currentIdleAnim == SPIRAL) {
    updateSpiralAnimation();
    if (!spiralAnimRunning) idleAnimActive = false;
  } else {
    updateSnakeAnimation();
    if (!snakeAnimRunning) idleAnimActive = false;
  }
}

// ========================= Leitura dos Botões =========================
void readButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool currentState = digitalRead(buttonPins[i]);
    // Detecta transição de LOW para HIGH (botão pressionado)
    if (lastButtonStates[i] == LOW && currentState == HIGH) {
      delay(50);  // Debounce
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
}

// ========================= WebSocket =========================
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len) {
      String msg = "";
      for (size_t i = 0; i < len; i++) {
        msg += (char)data[i];
      }
      Serial.print("Mensagem recebida: ");
      Serial.println(msg);
      if (msg == "start-game")
        triggerCountdown();
    }
  }
}

void updateWebSocket() {
  if (millis() - lastWsUpdate >= 100) {
    lastWsUpdate = millis();
    DynamicJsonDocument doc(128);
    doc["score"] = score;
    doc["state"] = gameState;
    if (gameState == 1)
      doc["timer"] = (gameDuration - (millis() - gameStartTime)) / 1000;
    else if (gameState == 4)
      doc["timer"] = (3000 - (millis() - countdownStartTime)) / 1000;
    else
      doc["timer"] = 0;
    String data;
    serializeJson(doc, data);
    ws.textAll(data);
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
    delay(500);
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
  gameState = 0;
  gameRunning = false;
  turnOffAllRelays();
  animationStatus = animationEnabled;
}

void loop() {
  updateGameState();
  
  if (gameState == 4) {
    handleCountdown();
  }
  
  if (waitingNextRound && millis() >= nextRoundTime) {
    waitingNextRound = false;
    nextRound();
  }
  
  if (gameState == 0 && animationStatus) {
    updateIdleAnimation();
  }
  
  readButtons();
  updateWebSocket();
  
  delay(10);  // Pequeno delay para não sobrecarregar o loop
}

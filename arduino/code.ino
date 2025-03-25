#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include "SerialRelay.h"
#include <ArduinoJson.h>

// ========================= Configurações Gerais =========================
unsigned long lastWsUpdate = 0; // declare no início do código
const int NUM_BUTTONS = 8;
int buttonPins[NUM_BUTTONS] = {14, 27, 26, 25, 33, 32, 35, 34};
bool lastButtonStates[NUM_BUTTONS];

const uint8_t NUM_MODULES = 2;
const uint8_t RELAYS_PER_MODULE = 4;

unsigned long countdownStartTime = 0;
bool animationStatus, animationEnabled = true;

int gameState = 0;  // 0: Idle, 1: Jogo, 2: Pontuação Final, 3: Ranking, 4: Contagem Regressiva, 5: Fim (relés acesos)
bool gameRunning = false;
unsigned long gameStartTime = 0;
unsigned long stateChangeTime = 0;
const unsigned long gameDuration = 21000; // 20 segundos
uint16_t score = 0;

unsigned long nextRoundTime = 0;
bool waitingNextRound = false;

// ========================= Configurações de Rede =========================
const char *ssid = "Phygital";
const char *password = "Eventstag";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ========================= Controle dos Relés =========================
SerialRelay relays(13, 12, NUM_MODULES);

// ========================= Lógica do Jogo =========================
struct Target {
  uint8_t module;
  uint8_t relay;
  uint8_t correctButton; // Botões 1-4 para módulo 1 e 5-8 para módulo 2 (offset de 4)
};

Target currentTarget;
Target previousTarget;
bool targetActive = false;

void turnOnAllRelays() {
  for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
    for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
      relays.SetRelay(r, SERIAL_RELAY_ON, mod);
    }
  }
}

void turnOffAllRelays() {
  for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
    for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
      relays.SetRelay(r, SERIAL_RELAY_OFF, mod);
    }
  }
}

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
  
  uint8_t mod, relay, correctButton;
  uint8_t attempts = 0;
  // Garante que a nova rodada seja diferente da anterior (até 10 tentativas)
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

void endGame() {
  gameRunning = false;
  targetActive = false;
  turnOffAllRelays();
  Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
  gameState = 2;
  stateChangeTime = millis();
}

void updateGameState() {
  // Desativa animação se o jogo não estiver idle
  if(gameState > 1) animationStatus = false;
  
  // Transição para fim do jogo: relés acesos por 3s
  if (gameState == 1 && (millis() - gameStartTime >= gameDuration)) {
    turnOnAllRelays();
    gameState = 5;
    stateChangeTime = millis();
  }
  // Desliga os relés após 3s no estado 5 e passa para pontuação final
  else if (gameState == 5 && (millis() - stateChangeTime >= 3000)) {
    turnOffAllRelays();
    Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
    gameState = 2;
    stateChangeTime = millis();
  }
  // Após 5s em pontuação final, vai para ranking
  else if (gameState == 2 && (millis() - stateChangeTime >= 5000)) {
    gameState = 3;
    stateChangeTime = millis();
  }
  // Após 10s no ranking, volta para idle e reativa animação
  else if (gameState == 3 && (millis() - stateChangeTime >= 10000)) {
    gameState = 0;
    score = 0;
    animationStatus = animationEnabled;
  }
}

void setButtonsToResponse() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool currentState = digitalRead(buttonPins[i]);
    if (lastButtonStates[i] == LOW && currentState == HIGH) {
      delay(50); // Debounce simples
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
          Serial.println("Resposta incorreta.");
        }
      }
    }
    lastButtonStates[i] = currentState;
  }
}

void handleCountdown() {
  unsigned long elapsed = millis() - countdownStartTime;
  if (elapsed < 3000) {
    // Durante a contagem, os relés piscam a cada 500ms
    if ((elapsed % 1000) < 500) {
      turnOnAllRelays();
    } else {
      turnOffAllRelays();
    }
  } else {
    turnOffAllRelays();
    startGame();
  }
}

// ========================= Animações Idle =========================
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

void setLed(uint8_t led, byte state) {
  if (led >= 1 && led <= 4) {
    relays.SetRelay(led, state, 1);
  } else if (led >= 5 && led <= 8) {
    relays.SetRelay(led - 4, state, 2);
  }
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
      setLed(spiralSequence[spiralIndex - 1], SERIAL_RELAY_OFF);
    if (spiralIndex < spiralSequenceCount) {
      setLed(spiralSequence[spiralIndex], SERIAL_RELAY_ON);
      spiralIndex++;
    } else if (now - spiralAnimStart >= spiralSequenceCount * 300UL + 500) {
      setLed(spiralSequence[spiralSequenceCount - 1], SERIAL_RELAY_OFF);
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
      setLed(snakeSequence[snakeIndex - 1], SERIAL_RELAY_OFF);
    if (snakeIndex < snakeSequenceCount) {
      setLed(snakeSequence[snakeIndex], SERIAL_RELAY_ON);
      snakeIndex++;
    } else if (now - snakeAnimStart >= snakeSequenceCount * 300UL + 500) {
      setLed(snakeSequence[snakeSequenceCount - 1], SERIAL_RELAY_OFF);
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

// ========================= Setup e Loop =========================
void setup() {
  Serial.begin(115200);
  delay(50);
  
  WiFi.begin(ssid, password);
  Serial.print("Conectando-se ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP do ESP: ");
  Serial.println(WiFi.localIP().toString());
  
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("Servidor web iniciado.");
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonStates[i] = digitalRead(buttonPins[i]);
  }
  
  for (uint8_t i = 1; i <= NUM_MODULES; i++) {
    relays.SetModule(0x00, i, false);
  }
  relays.Info(&Serial);
  
  gameState = 0;
  gameRunning = false;
  turnOffAllRelays();
  animationStatus = animationEnabled;
}

void loop() {
  updateGameState();
  
  if (gameState == 0 && animationStatus)
    updateIdleAnimation();
  
  if (gameState == 1)
    setButtonsToResponse();
  
  if (gameState == 4)
    handleCountdown();
  
  if (waitingNextRound && millis() >= nextRoundTime) {
    waitingNextRound = false;
    nextRound();
  }
  
  // Envia dados do jogo via WebSocket
  if (millis() - lastWsUpdate >= 100) {  // envia a cada 100 ms
    lastWsUpdate = millis();
    DynamicJsonDocument doc(128);
    doc["score"] = score;
    doc["state"] = gameState;
    // Calcular o timer de acordo com o estado
    unsigned long remainingTime = 0;
    if (gameState == 1) { // Durante o jogo
      remainingTime = (gameDuration - (millis() - gameStartTime)) / 1000;
    } else if (gameState == 4) { // Durante a contagem regressiva
      remainingTime = (3000 - (millis() - countdownStartTime)) / 1000;
    }
    doc["timer"] = remainingTime;
    String data;
    serializeJson(doc, data);
    ws.textAll(data);
  }
  
  delay(10);
}
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
unsigned long lastPrintTime = 0;
const char* RANKING_FILE = "/ranking.json";
const int NUM_BUTTONS = 8;
int buttonPins[NUM_BUTTONS] = {14, 27, 26, 25, 33, 32, 35, 34};
bool lastButtonStates[NUM_BUTTONS];

// ========================= Configuração dos Relés e Botões =========================
const uint8_t NUM_MODULES = 2;       // Número de módulos (ex.: 2)
const uint8_t RELAYS_PER_MODULE = 4;  // Número de relés por módulo (ex.: 4)
const uint8_t BUFFER_SIZE = 10;       // Tamanho do buffer para comandos

// Pino físico para iniciar o jogo (start)
const int startButtonPin = 15;
unsigned long countdownStartTime = 0; // Início da contagem regressiva

// Pino para alternar animação idle
const int togglePin = 2;
bool animationEnabled = true;
bool geralAnimationEnabled = true;
bool lastToggleState = false;
bool preStartGameLeds = false;
bool invertRelayLogic = true;  // Se true, lógica de inversão será aplicada

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
AsyncEventSource events("/events");

// ========================= Utilitários =========================
// Função para determinar o Content-Type a partir da extensão do arquivo
String getContentType(const String &filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  return "text/plain";
}

// Configura o File Server para servir arquivos do SPIFFS
void setupFileServer() {
  server.onNotFound([](AsyncWebServerRequest *request) {
    String path = request->url();
    if (path.endsWith("/")) path += "index.html";
    if (SPIFFS.exists(path)) {
      request->send(SPIFFS, path, getContentType(path));
    } else {
      request->send(404, "text/plain", F("Arquivo não encontrado"));
    }
  });
}

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

// ========================= Ranking =========================
struct RankingEntry {
  String datetime;
  uint16_t score;
};

#define MAX_RANKING 10
RankingEntry ranking[MAX_RANKING];
uint8_t rankingCount = 0; // Quantas entradas estão armazenadas

// Formata a data/hora no padrão PT-BR (dd/mm/yyyy hh:mm:ss)
String formatDateTime() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char buf[25];
  sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d",
          timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(buf);
}

// Carrega o ranking do SPIFFS usando RAII com unique_ptr para buffer
void loadRanking() {
  rankingCount = 0;  // "Limpa" o ranking definindo o contador como zero
  if (SPIFFS.exists(RANKING_FILE)) {
    File file = SPIFFS.open(RANKING_FILE, "r");
    if (file) {
      size_t size = file.size();
      std::unique_ptr<char[]> buf(new char[size]);
      file.readBytes(buf.get(), size);
      DynamicJsonDocument doc(512); // Tamanho ajustado conforme necessário
      DeserializationError error = deserializeJson(doc, buf.get());
      if (!error && doc.is<JsonArray>()) {
        JsonArray arr = doc.as<JsonArray>();
        for (JsonObject obj : arr) {
          if (rankingCount < MAX_RANKING) {
            ranking[rankingCount].datetime = obj["datetime"].as<String>();
            ranking[rankingCount].score = obj["score"];
            rankingCount++;
          }
        }
      }
      file.close();
    }
  }
}

// Salva o ranking no SPIFFS; a memória do DynamicJsonDocument será liberada ao sair do escopo
void saveRanking() {
  DynamicJsonDocument doc(512);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < rankingCount; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["datetime"] = ranking[i].datetime;
    obj["score"] = ranking[i].score;
  }
  File file = SPIFFS.open(RANKING_FILE, "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
  }
}

// Atualiza o ranking, mantendo somente os 10 melhores resultados
void updateRanking(uint16_t newScore) {
  RankingEntry newEntry;
  newEntry.datetime = formatDateTime();
  newEntry.score = newScore;

  // Se ainda houver espaço, adiciona a nova entrada
  if (rankingCount < MAX_RANKING) {
    ranking[rankingCount] = newEntry;
    rankingCount++;
  } else {
    // Se o ranking estiver cheio, substitui a entrada com a menor pontuação, se o novo resultado for melhor
    int lowestIndex = 0;
    for (int i = 1; i < rankingCount; i++) {
      if (ranking[i].score < ranking[lowestIndex].score) {
        lowestIndex = i;
      }
    }
    if (newScore > ranking[lowestIndex].score) {
      ranking[lowestIndex] = newEntry;
    }
  }
  
  // Ordena o array (bubble sort simples, pois MAX_RANKING é pequeno)
  for (int i = 0; i < rankingCount - 1; i++) {
    for (int j = 0; j < rankingCount - i - 1; j++) {
      if (ranking[j].score < ranking[j + 1].score) {
        RankingEntry temp = ranking[j];
        ranking[j] = ranking[j + 1];
        ranking[j + 1] = temp;
      }
    }
  }
  
  saveRanking();
}

// ========================= Funções para Manipulação dos Relés =========================
// Liga todos os relés de todos os módulos
void turnOnAllRelays() {
  for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
    for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
      relays.SetRelay(r, SERIAL_RELAY_ON, mod);
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}

// Desliga todos os relés de todos os módulos
void resetRelays() {
  for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
    for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
      relays.SetRelay(r, SERIAL_RELAY_OFF, mod);
    }
  }
}

// Animação de fim: liga todos os relés e depois desliga, 3 vezes.
void playEndAnimation() {
  for (int i = 0; i < 3; i++) {
    turnOnAllRelays();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    resetRelays();
    vTaskDelay(500 / portTICK_PERIOD_MS);
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
  score = 0;
  gameRunning = true;
  gameStartTime = millis();
  resetRelays();
  vTaskDelay(500 / portTICK_PERIOD_MS);
  nextRound();
  gameState = 1; // Jogo em andamento
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
  resetRelays();
  Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
  gameState = 2; // Tela de pontuação final
  stateChangeTime = millis();
}

// ========================= Atualiza o Estado do Jogo =========================
void updateGameState() {
  // Durante o jogo (estado 1), se o tempo expirou, transita para o estado 5 (fim com relés acesos)
  if (gameState == 1 && (millis() - gameStartTime >= gameDuration)) {
    turnOnAllRelays();
    gameState = 5;
    stateChangeTime = millis();
  }
  // No estado 5, após 3 segundos, desliga os relés e passa para a tela de pontuação final (estado 2)
  else if (gameState == 5 && (millis() - stateChangeTime >= 3000)) {
    resetRelays();
    Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
    gameState = 2;
    stateChangeTime = millis();
  }
  // Na tela de pontuação final (estado 2), após 5 segundos, transita para ranking (estado 3)
  else if (gameState == 2 && (millis() - stateChangeTime >= 5000)) {
    updateRanking(score);
    gameState = 3;
    stateChangeTime = millis();
  }
  // Na tela de ranking (estado 3), após 10 segundos, reseta para idle (estado 0)
  else if (gameState == 3 && (millis() - stateChangeTime >= 10000)) {
    gameState = 0;
    score = 0;
    animationEnabled = geralAnimationEnabled;
  }
}

// ========================= SSE: Envia Atualizações para o Front-End =========================
void sendSSEUpdate() {
  DynamicJsonDocument doc(512);
  doc["score"] = score;
  doc["state"] = gameState;
  
  if (gameState == 1) {
    unsigned long elapsed = millis() - gameStartTime;
    int timeRemaining = (elapsed >= gameDuration) ? 0 : (gameDuration - elapsed) / 1000;
    doc["timeRemaining"] = timeRemaining;
  } else {
    doc["timeRemaining"] = 20;
  }
  
  // Envia comando de áudio conforme o estado:
  if (gameState == 4) {
    doc["audio"] = "countdown";
  } else if (gameState == 5) {
    doc["audio"] = "end";
  }
  
  // Se estiver na tela de ranking, inclui o ranking no JSON
  if (gameState == 3) {
    JsonArray rankArr = doc.createNestedArray("ranking");
    int idx = 1;
    for (auto &entry : ranking) {
      JsonObject obj = rankArr.createNestedObject();
      obj["index"] = idx;
      obj["datetime"] = entry.datetime;
      obj["score"] = entry.score;
      idx++;
    }
  }
  
  String data;
  serializeJson(doc, data);
  events.send(data.c_str(), "message", millis());
}

// ========================= Endpoints da API =========================
void handleStartGame(AsyncWebServerRequest *request) {
  triggerCountdown();
  String response = "{\"status\":\"Countdown started\",\"score\":" + String(score) + "}";
  request->send(200, "application/json", response);
}

void handleGameStatus(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(256);
  doc["score"] = score;
  doc["state"] = gameState;
  if (gameState == 1) {
    unsigned long elapsed = millis() - gameStartTime;
    int timeRemaining = (elapsed >= gameDuration) ? 0 : (gameDuration - elapsed) / 1000;
    doc["timeRemaining"] = timeRemaining;
  } else {
    doc["timeRemaining"] = 0;
  }
  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}

void handleRanking(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(512);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < rankingCount; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["index"] = i + 1;
    obj["datetime"] = ranking[i].datetime;
    obj["score"] = ranking[i].score;
  }
  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}

void handleSendCommand(AsyncWebServerRequest *request) {
  if (!request->hasParam("plain", true)) {
    request->send(400, "application/json", "{\"error\":\"No body received\"}");
    return;
  }
  AsyncWebParameter* p = request->getParam("plain", true);
  String body = p->value();
  
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  String command = doc["command"];
  processCommand(command.c_str());
  String response = "{\"status\":\"Command executed\",\"command\":\"" + command + "\"}";
  request->send(200, "application/json", response);
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

// ========================= Task: Lógica do Jogo =========================
void gameLogicTask(void * parameter) {
  for (;;) {
    updateGameState();
    
    // Se o jogo estiver idle, verifica se o pino start (físico) foi pressionado para disparar o countdown
    if (gameState == 0) {
      if (digitalRead(startButtonPin) == LOW) { // INPUT_PULLUP: pressionado = LOW
        triggerCountdown();
        vTaskDelay(200 / portTICK_PERIOD_MS); // Debounce
      }
    }
    
    // Enquanto o jogo estiver idle e animações estiverem habilitadas, executa animações
    if (gameState == 0 && animationEnabled) {
      for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
        idleAnimation(mod);
      }
    }
    
    // Durante o jogo (estado 1), processa os botões para respostas
    if (gameState == 1) {
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
    
    // Processa a contagem regressiva (estado 4)
    if (gameState == 4) {
      unsigned long elapsed = millis() - countdownStartTime;
      if (elapsed < 3000) {
        // Durante a contagem, os relés piscam: 500ms ligados, 500ms apagados
        if ((elapsed % 1000) < 500) {
          for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
            for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
              relays.SetRelay(r, SERIAL_RELAY_ON, mod);
            }
          }
        } else {
          for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
            for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
              relays.SetRelay(r, SERIAL_RELAY_OFF, mod);
            }
          }
        }
      } else {
        // Ao fim dos 3 segundos, apaga todos os relés e inicia o jogo
        for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
          for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
            relays.SetRelay(r, SERIAL_RELAY_OFF, mod);
          }
        }
        startGame(); // Muda para estado 1 e registra gameStartTime
      }
    }
    
    // Processa a próxima rodada se agendada
    if (waitingNextRound && millis() >= nextRoundTime) {
      waitingNextRound = false;
      nextRound();
    }
    
    // Verifica o pino de toggle para alternar animação idle
    bool currentToggleState = digitalRead(togglePin);
    if (lastToggleState == LOW && currentToggleState == HIGH) {
      animationEnabled = !animationEnabled;
      geralAnimationEnabled = animationEnabled;
      Serial.printf("Animação %s\n", animationEnabled ? "ativada" : "desativada");
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    lastToggleState = currentToggleState;
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ========================= Novas Animações Visuais =========================

// Estrutura para definir uma posição de relé (módulo e número do relé)
struct RelayPosition {
  uint8_t module;
  uint8_t relay;
};

// Ordem para a animação de espiral (supondo que os 8 relés estejam dispostos em círculo)
// Exemplo: Módulo 1: relé 1, Módulo 2: relé 1, Módulo 2: relé 2, Módulo 1: relé 2, etc.
const RelayPosition spiralOrder[] = {
  {1, 1},
  {2, 1},
  {2, 2},
  {1, 2},
  {1, 3},
  {2, 3},
  {2, 4},
  {1, 4}
};
const size_t spiralOrderCount = sizeof(spiralOrder) / sizeof(spiralOrder[0]);

// Ordem para a animação de snake (cobrinha), percorrendo os 8 relés em sequência
const RelayPosition snakeOrder[] = {
  {1, 1},
  {1, 2},
  {1, 3},
  {1, 4},
  {2, 4},
  {2, 3},
  {2, 2},
  {2, 1}
};
const size_t snakeOrderCount = sizeof(snakeOrder) / sizeof(snakeOrder[0]);

// Variáveis de controle para a animação de espiral
bool spiralAnimRunning = false;
unsigned long spiralAnimStart = 0;
size_t spiralIndex = 0;

// Variáveis de controle para a animação de snake
bool snakeAnimRunning = false;
unsigned long snakeAnimStart = 0;
size_t snakeIndex = 0;

// Variável para escolher qual animação idle usar (0 = Spiral, 1 = Snake)
int idleAnimType = -1;

// Função para iniciar a animação de espiral
void startSpiralAnimation() {
  spiralAnimRunning = true;
  spiralAnimStart = millis();
  spiralIndex = 0;
}

// Atualiza a animação de espiral de forma não bloqueante
void updateSpiralAnimation() {
  if (!spiralAnimRunning) return;
  unsigned long now = millis();
  // Define um tempo de 200ms por etapa
  if (now - spiralAnimStart >= spiralIndex * 200UL) {
    // Desliga o relé anterior (se houver)
    if (spiralIndex > 0) {
      RelayPosition prev = spiralOrder[spiralIndex - 1];
      relays.SetRelay(prev.relay, SERIAL_RELAY_OFF, prev.module);
    } else {
      // Opcional: garante que todos estejam desligados no início
      resetRelays();
    }
    // Se ainda há etapas, liga o relé atual
    if (spiralIndex < spiralOrderCount) {
      RelayPosition current = spiralOrder[spiralIndex];
      relays.SetRelay(current.relay, SERIAL_RELAY_ON, current.module);
      spiralIndex++;
    } else {
      // Acabou a sequência; apaga tudo e encerra a animação
      resetRelays();
      spiralAnimRunning = false;
    }
  }
}

// Função para iniciar a animação snake
void startSnakeAnimation() {
  snakeAnimRunning = true;
  snakeAnimStart = millis();
  snakeIndex = 0;
}

// Atualiza a animação snake de forma não bloqueante
void updateSnakeAnimation() {
  if (!snakeAnimRunning) return;
  unsigned long now = millis();
  // Define 200ms por etapa
  if (now - snakeAnimStart >= snakeIndex * 200UL) {
    // Desliga o relé anterior
    if (snakeIndex > 0) {
      RelayPosition prev = snakeOrder[snakeIndex - 1];
      relays.SetRelay(prev.relay, SERIAL_RELAY_OFF, prev.module);
    } else {
      resetRelays();
    }
    // Se ainda há etapas, liga o relé atual
    if (snakeIndex < snakeOrderCount) {
      RelayPosition current = snakeOrder[snakeIndex];
      relays.SetRelay(current.relay, SERIAL_RELAY_ON, current.module);
      snakeIndex++;
    } else {
      // Finaliza a animação
      resetRelays();
      snakeAnimRunning = false;
    }
  }
}

// Função idleAnimation integrada para as novas animações visuais.
// Se idleAnimType for -1, escolhe aleatoriamente entre 0 (spiral) e 1 (snake).
// Em seguida, chama a função de atualização correspondente.
void idleAnimation(uint8_t module) {
  if (idleAnimType < 0) {
    idleAnimType = random(0, 2); // 0 ou 1
    // Inicia a animação escolhida (apenas uma vez; a animação é global para o modo idle)
    if (idleAnimType == 0) {
      startSpiralAnimation();
    } else {
      startSnakeAnimation();
    }
  }
  // Chama a atualização da animação escolhida
  if (idleAnimType == 0) {
    updateSpiralAnimation();
    // Se a animação terminou, reseta para nova escolha
    if (!spiralAnimRunning) {
      idleAnimType = -1;
    }
  } else if (idleAnimType == 1) {
    updateSnakeAnimation();
    if (!snakeAnimRunning) {
      idleAnimType = -1;
    }
  }
}

void printFreeMemory() {
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
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

  loadRanking();
  
  // Configura a hora via NTP (ajuste o fuso conforme necessário)
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  
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
  
  setupFileServer();
  
  // Registra os endpoints da API
  server.on("/send-command", HTTP_POST, handleSendCommand);
  server.on("/start-game", HTTP_POST, handleStartGame);
  server.on("/game-status", HTTP_GET, handleGameStatus);
  server.on("/ranking", HTTP_GET, handleRanking);
  
  // Adiciona o handler para SSE
  server.addHandler(&events);
  
  server.begin();
  Serial.println(F("Servidor web iniciado."));

  // Configura o pino do botão de start físico
  pinMode(startButtonPin, INPUT_PULLUP);
  
  // Configura os pinos dos botões de controle
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonStates[i] = digitalRead(buttonPins[i]);
  }
  pinMode(togglePin, INPUT_PULLUP);
  lastToggleState = digitalRead(togglePin);
  
  // Inicializa os módulos dos relés (desligados)
  for (uint8_t i = 1; i <= NUM_MODULES; i++) {
    relays.SetModule(0x00, i, false);
  }
  relays.Info(&Serial);
  bufferIndex = 0;
  
  gameState = 0;
  gameRunning = false;
  
  resetRelays();

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
  // O AsyncWebServer é não bloqueante, portanto não precisamos de server.handleClient()
  // Envia atualizações SSE a cada ciclo para o front-end
  sendSSEUpdate();

  // Verifica o heap livre a cada 5 segundos
  if (millis() - lastPrintTime > 5000) {
    printFreeMemory();
    lastPrintTime = millis();
  }

  vTaskDelay(500 / portTICK_PERIOD_MS);
}
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

  // ------------------------- Configurações de Rede -------------------------
  const char *ssid = "Phygital";
  const char *password = "Eventstag";

  // Instância do servidor assíncrono e do SSE
  AsyncWebServer server(80);
  AsyncEventSource events("/events");

  // ------------------------- Utilitários -------------------------
  String getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    return "text/plain";
  }

  // Configuração do File Server via AsyncWebServer
  void setupFileServer() {
    server.onNotFound([](AsyncWebServerRequest *request) {
      String path = request->url();
      if (path.endsWith("/")) path += "index.html";
      if (SPIFFS.exists(path)) {
        request->send(SPIFFS, path, getContentType(path));
      } else {
        request->send(404, "text/plain", "Arquivo não encontrado");
      }
    });
  }

  // ------------------------- Configuração dos Relés e Botões -------------------------
  const uint8_t NUM_MODULES = 2;       // Número de módulos (ex.: 2)
  const uint8_t RELAYS_PER_MODULE = 4;  // Número de relés por módulo (ex.: 4)
  const uint8_t BUFFER_SIZE = 10;       // Tamanho do buffer para comandos

  SerialRelay relays(13, 12, NUM_MODULES); // (data pin, clock pin, número de módulos)
  uint8_t currentModule = 1;             // Módulo ativo

  char buffer[BUFFER_SIZE];
  uint8_t bufferIndex = 0;

  const int NUM_BUTTONS = 8;
  int buttonPins[NUM_BUTTONS] = {14, 27, 26, 25, 33, 32, 35, 34};
  bool lastButtonStates[NUM_BUTTONS];

  const int startButtonPin = 15;      // Pino físico para iniciar o jogo
  unsigned long countdownStartTime = 0; // Armazena o instante de início da contagem regressiva

  // Pino para alternar animação idle
  const int togglePin = 2;
  bool animationEnabled = false;
  bool lastToggleState;
  bool preStartGameLeds = false;
  bool invertRelayLogic = true; // Defina como true para inverter

  // ------------------------- Lógica do Jogo e Ranking -------------------------
  // Estados do jogo:
  // 0 = Idle (aguardando início)
  // 1 = Jogo em andamento
  // 2 = Tela de pontuação final (por 5s)
  // 3 = Tela de ranking (por 10s)
  int gameState = 0;
  bool gameRunning = false;
  unsigned long gameStartTime = 0;
  unsigned long stateChangeTime = 0;
  const unsigned long gameDuration = 20000; // 20 segundos
  uint16_t score = 0;


  // Declaração das variáveis de agendamento da próxima rodada
  unsigned long nextRoundTime = 0;
  bool waitingNextRound = false;

  // Estrutura para o "alvo" da rodada
  struct Target {
    uint8_t module;
    uint8_t relay;
    uint8_t correctButton; // Se módulo 1: botão = relay; se módulo 2: botão = relay + 4
  };
  Target currentTarget;
  Target previousTarget;
  bool targetActive = false;

  // ------------------------- Ranking -------------------------
  struct RankingEntry {
    String datetime;
    uint16_t score;
  };
  vector<RankingEntry> ranking; // Armazena as entradas do ranking

  // Formata data/hora no padrão PT-BR (dd/mm/yyyy hh:mm:ss)
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

  const char* RANKING_FILE = "/ranking.json";

  void loadRanking() {
    ranking.clear();
    if (SPIFFS.exists(RANKING_FILE)) {
      File file = SPIFFS.open(RANKING_FILE, "r");
      if (file) {
        size_t size = file.size();
        std::unique_ptr<char[]> buf(new char[size]);
        file.readBytes(buf.get(), size);
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, buf.get());
        if (!error && doc.is<JsonArray>()) {
          JsonArray arr = doc.as<JsonArray>();
          for (JsonObject obj : arr) {
            RankingEntry entry;
            entry.datetime = obj["datetime"].as<String>();
            entry.score = obj["score"];
            ranking.push_back(entry);
          }
        }
        file.close();
      }
    }
  }

  void saveRanking() {
    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.to<JsonArray>();
    for (auto &entry : ranking) {
      JsonObject obj = arr.createNestedObject();
      obj["datetime"] = entry.datetime;
      obj["score"] = entry.score;
    }
    File file = SPIFFS.open(RANKING_FILE, "w");
    if (file) {
      serializeJson(doc, file);
      file.close();
    }
  }

  void updateRanking(uint16_t newScore) {
    RankingEntry newEntry;
    newEntry.datetime = formatDateTime();
    newEntry.score = newScore;
    ranking.push_back(newEntry);
    std::sort(ranking.begin(), ranking.end(), [](const RankingEntry &a, const RankingEntry &b) {
      return a.score > b.score;
    });
    if (ranking.size() > 10) ranking.resize(10);
    saveRanking();
  }

  void triggerCountdown() {
    if (gameState == 0) {  // Somente se estiver em idle
      gameState = 4;       // Novo estado: contagem regressiva
      countdownStartTime = millis();
    }
  }

  // ------------------------- Funções de Jogo -------------------------
  void turnOnAllRelays() {
    for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
      if (mod == 1)
        relays.SetModule(0x00, mod, false);
      else
        relays.SetModule(0x55, mod);
      for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
        relays.SetRelay(r, SERIAL_RELAY_ON, mod);
      }
    }
  }

  void resetRelays() {
    for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
      if (mod == 1)
        relays.SetModule(0x00, mod, false);
      else
        relays.SetModule(0x55, mod);
      for (uint8_t r = 1; r <= RELAYS_PER_MODULE; r++) {
        relays.SetRelay(r, SERIAL_RELAY_OFF, mod);
      }
    }
  }

  void playEndAnimation() {
    for (int i = 0; i < 3; i++) {
      turnOnAllRelays();
      vTaskDelay(500 / portTICK_PERIOD_MS);
      resetRelays();
      // Aguarda 200ms para completar 1 segundo no ciclo
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }  

  void nextRound() {
    if (millis() - gameStartTime >= gameDuration) return;
    uint8_t mod, relay, correctButton;
    do {
      mod = random(1, NUM_MODULES + 1);
      relay = random(1, RELAYS_PER_MODULE + 1);
      correctButton = (mod == 1) ? relay : relay + 4;
    } while (targetActive && (previousTarget.module == mod && previousTarget.relay == relay));
    previousTarget = { mod, relay, correctButton };
    currentTarget = previousTarget;
    targetActive = true;
    Serial.printf("Nova rodada: Módulo %d, Relé %d, Botão correto: %d\n", mod, relay, correctButton);
    relays.SetRelay(relay, SERIAL_RELAY_ON, mod);
  }

  void endGame() {
    gameRunning = false;
    targetActive = false;
    resetRelays();
    Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
    gameState = 2; // Tela de pontuação final
    stateChangeTime = millis();
  }

  void startGame() {
    score = 0;
    gameRunning = true;
    gameStartTime = millis();
    resetRelays();
    vTaskDelay(500 / portTICK_PERIOD_MS); // libera o processador durante esse tempo
    nextRound();
    gameState = 1; // Jogo em andamento
  }

  // Atualiza o estado do jogo (transição entre fases)
  void updateGameState() {
    // Se o jogo está em andamento e o tempo acabou,
    // ao invés de chamar endGame() diretamente, entra no estado 5 (indicador de fim)
    if (gameState == 1 && (millis() - gameStartTime >= gameDuration)) {
      // Acende todos os relés para indicar o fim do jogo
      turnOnAllRelays();
      gameState = 5;  // Novo estado: indicador de fim
      stateChangeTime = millis();
    }
    // Se estiver no estado 5 por 3 segundos, apaga todos os relés e passa para a tela de pontuação final (estado 2)
    else if (gameState == 5 && (millis() - stateChangeTime >= 3000)) {
      resetRelays();
      // Exibe a pontuação final (estado 2)
      Serial.printf("Fim do jogo! Pontuação final: %d\n", score);
      gameState = 2;
      stateChangeTime = millis();
    }
    // Se estiver na tela de pontuação final por 5 segundos, passa para a tela de ranking (estado 3)
    else if (gameState == 2 && (millis() - stateChangeTime >= 5000)) {
      updateRanking(score);
      gameState = 3; // Tela de ranking
      stateChangeTime = millis();
    }
    // Se estiver na tela de ranking por 10 segundos, reseta para idle
    else if (gameState == 3 && (millis() - stateChangeTime >= 10000)) {
      gameState = 0; // Idle
      score = 0;
    }
  }

  // ------------------------- SSE: Atualiza o Front-End -------------------------
  void sendSSEUpdate() {
    // Use um DynamicJsonDocument com capacidade maior para incluir ranking se necessário
    DynamicJsonDocument doc(1024);
    doc["score"] = score;
    doc["state"] = gameState;
    if (gameState == 1) {
      unsigned long elapsed = millis() - gameStartTime;
      int timeRemaining = (elapsed >= gameDuration) ? 0 : (gameDuration - elapsed) / 1000;
      doc["timeRemaining"] = timeRemaining;
    } else {
      doc["timeRemaining"] = 20;
    }

    // Se estiver na contagem regressiva, envie o comando para tocar o áudio de countdown
    if (gameState == 4) {
      doc["audio"] = "countdown";
    }
    // Se estiver no estado 5 (fim do jogo), envie o comando para tocar o áudio de fim
    else if (gameState == 5) {
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

  // ------------------------- Endpoints da API -------------------------
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
    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.to<JsonArray>();
    int index = 1;
    for (auto &entry : ranking) {
      JsonObject obj = arr.createNestedObject();
      obj["index"] = index;
      obj["datetime"] = entry.datetime;
      obj["score"] = entry.score;
      index++;
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

  // ------------------------- Processamento de Comandos (Serial) -------------------------
  void processCommand(const char *cmd) {
    if (cmd[0] == 'm' || cmd[0] == 'M') {
      uint8_t mod = cmd[1] - '0';
      if (mod >= 1 && mod <= NUM_MODULES) {
        currentModule = mod;
        Serial.printf("Módulo selecionado: %d\n", currentModule);
      }
    }
    else if (cmd[0] == '+' || cmd[0] == '-') {
      uint8_t relayNum = cmd[1] - '0';
      if (relayNum >= 1 && relayNum <= RELAYS_PER_MODULE) {
        byte state = (cmd[0] == '+') ? SERIAL_RELAY_ON : SERIAL_RELAY_OFF;
        relays.SetRelay(relayNum, state, currentModule);
        Serial.printf("Set relay %d no módulo %d para o estado %d\n", relayNum, currentModule, state);
      }
    }
    else {
      if (strlen(cmd) >= 2) {
        byte value = 0;
        for (uint8_t i = 0; i < 2; i++) {
          char c = cmd[i];
          byte nibble = 0;
          if (c >= '0' && c <= '9') nibble = c - '0';
          else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
          else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
          if (i == 0) value = nibble << 4;
          else value |= nibble;
        }
        Serial.printf("Valor hexadecimal recebido: %X\n", value);
        relays.SetModule(value, currentModule);
      }
    }
  }

  // Animação: Liga cada relé em sequência e depois os desliga em sequência.
  void sequentialAnimation(uint8_t module) {
    for (int i = 1; i <= RELAYS_PER_MODULE; i++) {
      // Verifica se há alguma interrupção (por exemplo, comando via serial) e interrompe a animação se necessário
      if (Serial.available() > 0) return;
      relays.SetRelay(i, SERIAL_RELAY_ON, module);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    for (int i = 1; i <= RELAYS_PER_MODULE; i++) {
      if (Serial.available() > 0) return;
      relays.SetRelay(i, SERIAL_RELAY_OFF, module);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  // Animação: Pisca todos os relés simultaneamente.
  void blinkAllAnimation(uint8_t module) {
    if (Serial.available() > 0) return;
    relays.SetModule(0xFF, module);  // Liga todos
    vTaskDelay(300 / portTICK_PERIOD_MS);
    relays.SetModule(0x00, module);  // Desliga todos
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }

  // Animação: Pisca um relé escolhido aleatoriamente, repetindo 3 vezes.
  void randomBlinkAnimation(uint8_t module) {
    int relay = random(1, RELAYS_PER_MODULE + 1);
    for (int i = 0; i < 3; i++) {
      if (Serial.available() > 0) return;
      relays.SetRelay(relay, SERIAL_RELAY_ON, module);
      vTaskDelay(200 / portTICK_PERIOD_MS);
      relays.SetRelay(relay, SERIAL_RELAY_OFF, module);
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }
  }

  // Animação: "Cobrinha" – acende os relés um a um, dando a impressão de movimento.
  void snakeAnimation(uint8_t module) {
    for (int i = 1; i <= RELAYS_PER_MODULE; i++) {
      if (Serial.available() > 0) return;
      // Garante que apenas um relé fique aceso
      relays.SetModule(0x00, module);
      relays.SetRelay(i, SERIAL_RELAY_ON, module);
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }

  // Função que escolhe uma animação aleatoriamente para o módulo especificado
  void idleAnimation(uint8_t module) {
    int anim = random(0, 4); // Escolhe um número entre 0 e 3
    switch (anim) {
      case 0:
        sequentialAnimation(module);
        break;
      case 1:
        blinkAllAnimation(module);
        break;
      case 2:
        randomBlinkAnimation(module);
        break;
      case 3:
        snakeAnimation(module);
        break;
    }
  }

  // ------------------------- Task: Lógica do Jogo -------------------------
  void gameLogicTask(void * parameter) {
    for (;;) {
      updateGameState();

      if (gameState == 0) {
        if (digitalRead(startButtonPin) == LOW) {  // Botão pressionado (com INPUT_PULLUP, pressionado = LOW)
          triggerCountdown();
          vTaskDelay(200 / portTICK_PERIOD_MS); // Debounce
        }
      }
      
      if (gameState == 0 && animationEnabled) {
        for (uint8_t mod = 1; mod <= NUM_MODULES; mod++) {
          idleAnimation(mod);
        }
      }    
      
      if (gameState == 1) { // Jogo em andamento: processa botões
        for (int i = 0; i < NUM_BUTTONS; i++) {
          bool currentState = digitalRead(buttonPins[i]);
          if (lastButtonStates[i] == LOW && currentState == HIGH) {
            vTaskDelay(50 / portTICK_PERIOD_MS); // libera o processador durante esse tempo
            if (digitalRead(buttonPins[i]) == HIGH) {
              uint8_t buttonPressed = i + 1;
              Serial.printf("Botão pressionado: %d\n", buttonPressed);
              if (targetActive && buttonPressed == currentTarget.correctButton) {
                score++;
                Serial.printf("Resposta correta! Pontuação: %d\n", score);
                relays.SetRelay(currentTarget.relay, SERIAL_RELAY_OFF, currentTarget.module);
                targetActive = false;
                // Agenda a próxima rodada para daqui a 300ms
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

      // Processa a contagem regressiva (estado 4)
      if (gameState == 4) {
        unsigned long elapsed = millis() - countdownStartTime;
        if (elapsed < 3000) {
          playEndAnimation();
        } else {
          startGame(); // Isso define gameState = 1 e registra gameStartTime
        }
      }  
      
      if (waitingNextRound && millis() >= nextRoundTime) {
        waitingNextRound = false;
        nextRound();
      }
      
      bool currentToggleState = digitalRead(togglePin);
      if (lastToggleState == LOW && currentToggleState == HIGH) {
        animationEnabled = !animationEnabled;
        Serial.printf("Animação %s\n", animationEnabled ? "ativada" : "desativada");
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      lastToggleState = currentToggleState;
      
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }

  // ------------------------- Setup -------------------------
  void setup() {
    Serial.begin(115200);
    vTaskDelay(50 / portTICK_PERIOD_MS); // libera o processador durante esse tempo
    randomSeed(esp_random());

    if (!SPIFFS.begin(true)) {
      Serial.println("Falha ao montar SPIFFS");
      return;
    }
    loadRanking();
    
    // Configura a hora via NTP (ajuste o fuso conforme necessário)
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    
    // Conecta ao WiFi
    WiFi.begin(ssid, password);
    Serial.print("Conectando-se ao WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(500 / portTICK_PERIOD_MS); // libera o processador durante esse tempo
      Serial.print(".");
    }
    Serial.println("\nWiFi conectado!");
    Serial.print("IP do ESP: ");
    Serial.println(WiFi.localIP());
    
    setupFileServer();
    
    // Registra endpoints da API
    server.on("/send-command", HTTP_POST, handleSendCommand);
    server.on("/start-game", HTTP_POST, handleStartGame);
    server.on("/game-status", HTTP_GET, handleGameStatus);
    server.on("/ranking", HTTP_GET, handleRanking);
    
    // Adiciona o handler SSE
    server.addHandler(&events);
    
    server.begin();
    Serial.println("Servidor web iniciado.");

    pinMode(startButtonPin, INPUT_PULLUP);
    
    // Configura pinos dos botões
    for (int i = 0; i < NUM_BUTTONS; i++) {
      pinMode(buttonPins[i], INPUT_PULLUP);
      lastButtonStates[i] = digitalRead(buttonPins[i]);
    }
    pinMode(togglePin, INPUT_PULLUP);
    lastToggleState = digitalRead(togglePin);
    
    // Inicializa os módulos dos relés
    for (uint8_t i = 1; i <= NUM_MODULES; i++) {
      if (i == 1)
        relays.SetModule(0x00, i, false);
      else
        relays.SetModule(0x55, i);
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

  void loop() {
    // Com AsyncWebServer, não é necessário chamar server.handleClient()
    // Atualiza o SSE a cada ciclo (ou chame sendSSEUpdate() em pontos estratégicos)
    sendSSEUpdate();
    vTaskDelay(1000 / portTICK_PERIOD_MS); // libera o processador durante esse tempo
  }

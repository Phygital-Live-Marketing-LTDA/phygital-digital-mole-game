const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const PORT = 3000;

app.use(bodyParser.json());
app.use(express.static('public')); // Caso você queira servir arquivos estáticos

// ------------------- Simulação do Estado do Jogo -------------------
/*
  Estados do jogo:
  0 = Idle
  1 = Jogo em andamento
  2 = Tela de pontuação final
  3 = Tela de ranking
*/
let gameState = 0;
let score = 0;
const gameDuration = 20; // duração do jogo em segundos
let timeRemaining = gameDuration;

// Simulação do ranking (array de objetos com data e pontuação)
let ranking = [
  { datetime: "01/01/2023 12:00:00", score: 10 },
  { datetime: "02/01/2023 13:00:00", score: 8 }
];

// ------------------- Endpoints da API -------------------

// Inicia o jogo (POST /start-game)
app.post('/start-game', (req, res) => {
  gameState = 1; // jogo em andamento
  score = 0;
  timeRemaining = gameDuration;
  
  // Inicia um contador (simula a passagem do tempo do jogo)
  if (global.gameInterval) clearInterval(global.gameInterval);
  global.gameInterval = setInterval(() => {
    timeRemaining--;
    if (timeRemaining <= 0) {
      clearInterval(global.gameInterval);
      gameState = 2; // tela de pontuação final
      // Após 5 segundos, passa para a tela de ranking
      setTimeout(() => {
        gameState = 3;
        // Após 10 segundos, reseta para idle
        setTimeout(() => {
          gameState = 0;
        }, 10000);
      }, 5000);
    }
  }, 1000);

  res.json({ status: "Game started", score });
});

// Retorna o status do jogo (GET /game-status)
app.get('/game-status', (req, res) => {
  res.json({
    state: gameState,
    score: score,
    timeRemaining: timeRemaining
  });
});

// Retorna o ranking (GET /ranking)
app.get('/ranking', (req, res) => {
  res.json(ranking);
});

// Endpoint SSE para enviar atualizações em "push" (GET /events)
app.get('/events', (req, res) => {
  // Define os headers para SSE
  res.set({
    'Content-Type': 'text/event-stream',
    'Cache-Control': 'no-cache',
    'Connection': 'keep-alive'
  });
  res.flushHeaders();

  // Função que envia os dados do jogo
  const sendEvent = () => {
    const data = {
      state: gameState,
      score: score,
      timeRemaining: timeRemaining
    };
    // Se estiver na tela de ranking, envia também o ranking
    if (gameState === 3) {
      data.ranking = ranking;
    }
    res.write(`data: ${JSON.stringify(data)}\n\n`);
  };

  // Envia atualizações a cada 1 segundo
  const intervalId = setInterval(sendEvent, 1000);

  req.on('close', () => {
    clearInterval(intervalId);
    res.end();
  });
});

// Endpoint para simular envio de comando (POST /send-command)
app.post('/send-command', (req, res) => {
  const command = req.body.command;
  // Aqui você pode simular o processamento do comando
  res.json({ status: "Command executed", command });
});

// ------------------- Inicialização do Servidor -------------------
app.listen(PORT, () => {
  console.log(`Servidor rodando na porta ${PORT}`);
});

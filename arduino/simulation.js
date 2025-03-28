// server.js
const express = require('express');
const http = require('http');
const WebSocket = require('ws');

// Cria o app Express e configura a porta
const app = express();
const port = 3000;
app.use(express.static('public')); // Servirá arquivos estáticos da pasta "public"

// Cria o servidor HTTP e o WebSocket no caminho /ws
const server = http.createServer(app);
const wss = new WebSocket.Server({ server, path: '/ws' });

// Simulação dos relés para 2 módulos com 4 relés cada
class SimulatedRelay {
    constructor() {
        this.state = [
            [false, false, false, false], // Módulo 1
            [false, false, false, false]  // Módulo 2
        ];
    }

    setRelay(module, relay, on) {
        if (module < 1 || module > 2 || relay < 1 || relay > 4) return;
        this.state[module - 1][relay - 1] = on;
        console.log(`Módulo ${module}, Relé ${relay} ${on ? 'ON' : 'OFF'}`);
    }

    turnOnAll() {
        for (let m = 1; m <= 2; m++) {
            for (let r = 1; r <= 4; r++) {
                this.setRelay(m, r, true);
            }
        }
    }

    turnOffAll() {
        for (let m = 1; m <= 2; m++) {
            for (let r = 1; r <= 4; r++) {
                this.setRelay(m, r, false);
            }
        }
    }
}

const relays = new SimulatedRelay();

// Definição dos estados do jogo
const GameState = {
    IDLE: 0,
    COUNTDOWN: 4,
    GAME_RUNNING: 1,
    GAME_OVER: 5,
    FINAL_SCORE: 2,
    RANKING: 3
};

let gameState = GameState.IDLE;
let targetActive = false;
let currentTarget = null;
let previousTarget = null;
let score = 0;
let gameStartTime = null;
const gameDuration = 21000; // 21 segundos
let countdownStartTime = null;

// Funções para gerar módulo e relé aleatoriamente
function randomModule() {
    return Math.floor(Math.random() * 2) + 1;
}

function randomRelay() {
    return Math.floor(Math.random() * 4) + 1;
}

// Seleciona a próxima rodada
function nextRound() {
    if (Date.now() - gameStartTime >= gameDuration) return;

    let mod, relay, correctButton;
    let attempts = 0;
    do {
        mod = randomModule();
        relay = randomRelay();
        // Mapeia: módulo 1 → botões 1-4; módulo 2 → botões 5-8
        correctButton = (mod === 1) ? relay : relay + 4;
        attempts++;
    } while (previousTarget && mod === previousTarget.module && relay === previousTarget.relay && attempts < 10);

    previousTarget = { module: mod, relay: relay, correctButton: correctButton };
    currentTarget = previousTarget;
    targetActive = true;

    console.log(`Nova rodada: Módulo ${mod}, Relé ${relay}, Botão correto ${correctButton} (tentativas: ${attempts})`);
    relays.setRelay(mod, relay, true);
}

// Inicia a contagem regressiva (3 segundos)
function startCountdown() {
    gameState = GameState.COUNTDOWN;
    countdownStartTime = Date.now();
    console.log("Contagem regressiva iniciada...");

    const countdownInterval = setInterval(() => {
        const elapsed = Date.now() - countdownStartTime;
        if (elapsed >= 3000) {
            clearInterval(countdownInterval);
            startGame();
        } else {
            // Alterna o estado de todos os relés a cada 500ms para simular a contagem
            if (Math.floor(elapsed / 500) % 2 === 0)
                relays.turnOnAll();
            else
                relays.turnOffAll();
        }
    }, 100);
}

// Inicia o jogo propriamente dito
function startGame() {
    gameState = GameState.GAME_RUNNING;
    score = 0;
    gameStartTime = Date.now();
    console.log("Jogo iniciado!");
    nextRound();

    const gameInterval = setInterval(() => {
        if (Date.now() - gameStartTime >= gameDuration) {
            clearInterval(gameInterval);
            gameOver();
        }
    }, 100);
}

function resetGame() {
    console.log("Exibindo pontuação final. Aguarde 5 segundos para transição para Ranking...");
    setTimeout(() => {
        console.log("Transição para Ranking...");
        gameState = GameState.RANKING;
        setTimeout(() => {
            console.log("Transição para IDLE...");
            gameState = GameState.IDLE;
            // Aqui você pode reiniciar o jogo se desejar ou esperar um novo start
        }, 10000); // Estado Ranking por 10 segundos
    }, 5000); // Estado Final Score por 5 segundos
}

function gameOver() {
    gameState = GameState.GAME_OVER;
    relays.turnOnAll();
    console.log("Fim do jogo! Ligando todos os relés...");
    setTimeout(() => {
        relays.turnOffAll();
        console.log(`Fim do jogo! Pontuação final: ${score}`);
        gameState = GameState.FINAL_SCORE;
        resetGame(); // Chama a função que faz a transição para Ranking e depois para Idle
    }, 3000);
}


// Envia atualizações para todos os clientes conectados
function broadcast(data) {
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
        }
    });
}

// Envia periodicamente o estado do jogo via WebSocket
setInterval(() => {
    let timer = 0;
    if (gameState === GameState.GAME_RUNNING) {
        timer = Math.floor((gameDuration - (Date.now() - gameStartTime)) / 1000);
    } else if (gameState === GameState.COUNTDOWN) {
        timer = Math.floor((3000 - (Date.now() - countdownStartTime)) / 1000);
    }
    broadcast({
        score: score,
        state: gameState,
        timer: timer
    });
}, 100);

// Tratamento das conexões WebSocket
wss.on('connection', ws => {
    ws.on('message', message => {
        const msg = (typeof message === 'string') ? message : message.toString();
        console.log("Mensagem recebida:", msg);
        if (msg === 'start-game' && gameState === GameState.IDLE) {
            startCountdown();
        }
        if (msg.startsWith('button-')) {
            const button = parseInt(msg.split('-')[1]);
            if (targetActive && button === currentTarget.correctButton) {
                score++;
                console.log(`Botão ${button} pressionado corretamente! Pontuação: ${score}`);
                relays.setRelay(currentTarget.module, currentTarget.relay, false);
                targetActive = false;
                setTimeout(nextRound, 300);
            } else {
                console.log(`Botão ${button} pressionado incorretamente.`);
            }
        }
    });
});

// Simulação interna de cliques aleatórios (a cada 1 segundo)
setInterval(() => {
    if (gameState === GameState.GAME_RUNNING && targetActive) {
        // Gera um botão aleatório entre 1 e 8
        const simulatedButton = Math.floor(Math.random() * 8) + 1;
        console.log(`(Simulação) Clique no botão ${simulatedButton}`);
        // Processa a simulação como se a mensagem tivesse sido recebida
        if (simulatedButton === currentTarget.correctButton) {
            score++;
            console.log(`(Simulação) Botão ${simulatedButton} pressionado corretamente! Pontuação: ${score}`);
            relays.setRelay(currentTarget.module, currentTarget.relay, false);
            targetActive = false;
            setTimeout(nextRound, 300);
        } else {
            console.log(`(Simulação) Botão ${simulatedButton} pressionado incorretamente.`);
        }
    }
}, 1000);

// Inicia o servidor
server.listen(port, () => {
    console.log(`Servidor rodando em http://localhost:${port}`);
});

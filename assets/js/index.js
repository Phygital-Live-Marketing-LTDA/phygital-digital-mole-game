// Variáveis de estado do jogo
let gameRunning = false;
let gameStartTime = 0;
const gameDuration = 20000; // 20 segundos (em milissegundos)
let currentTarget = null;   // Objeto { module, relay, correctButton }
let score = 0;
let roundTimeout = null;

// Atualiza a exibição da pontuação
function updateScoreDisplay() {
  document.getElementById('score').innerText = "Pontuação: " + score;
}

// Função para enviar comandos para o servidor (que repassa para o Arduino)
function sendCommand(command) {
  return fetch('/send-command', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ command: command })
  }).then(response => response.json());
}

// Reseta todos os relés dos módulos 1 e 2 (relés 1 a 4 em cada)
async function resetRelays() {
  for (let module = 1; module <= 2; module++) {
    await sendCommand('m' + module);
    for (let relay = 1; relay <= 4; relay++) {
      await sendCommand('-' + relay);
    }
  }
}

// Liga um relé específico
async function turnOnRelay(module, relay) {
  await sendCommand('m' + module);
  await sendCommand('+' + relay);
}

// Desliga um relé específico
async function turnOffRelay(module, relay) {
  await sendCommand('m' + module);
  await sendCommand('-' + relay);
}

// Gera um inteiro aleatório entre min e max (inclusivo)
function randomInt(min, max) {
  return Math.floor(Math.random() * (max - min + 1)) + min;
}

// Inicia uma nova rodada do jogo
function nextRound() {
  const elapsed = Date.now() - gameStartTime;
  document.getElementById('timer').innerText =
    "Tempo restante: " + Math.max(0, Math.ceil((gameDuration - elapsed) / 1000)) + "s";

  // Escolhe aleatoriamente módulo (1 ou 2) e relé (1 a 4)
  const module = randomInt(1, 2);
  const relay = randomInt(1, 4);
  // Define o botão correto: módulo 1 → botão = relay; módulo 2 → botão = relay + 4
  const correctButton = (module === 1) ? relay : relay + 4;
  currentTarget = { module, relay, correctButton };
  console.log("Round: Módulo " + module + ", Relé " + relay + ", Botão correto: " + correctButton);

  // Liga o relé escolhido
  turnOnRelay(module, relay);
  // NÃO há timeout: o round só avança quando um botão é pressionado (ou quando o tempo total expira)
}

// Encerra o jogo e exibe a pontuação final
function endGame() {
  gameRunning = false;
  currentTarget = null;
  clearTimeout(roundTimeout);
  document.body.innerHTML = "<h1>Fim do Jogo</h1><p>Pontuação final: " + score + "</p>";
}

// Inicia o jogo: reseta os relés e inicia a contagem de 20 segundos
function startGame() {
  score = 0;
  updateScoreDisplay();
  gameRunning = true;
  gameStartTime = Date.now();
  resetRelays().then(() => {
    setTimeout(nextRound, 500);
  });
}

// Configura o Socket.IO para receber eventos de botão pressionado
const socket = io();
socket.on('connect', () => {
  console.log("Conectado ao Socket.IO");
});

socket.on('button_pressed', (data) => {
  let correctionFactor = 2;
  let button_pressed = parseInt(data.button) - correctionFactor;
  console.log("Botão pressionado: " + button_pressed);
  if (gameRunning && currentTarget) {
    if (button_pressed === currentTarget.correctButton) {
      score++;
      updateScoreDisplay();
      console.log("Resposta correta! Pontuação: " + score);
    } else {
      console.log("Resposta incorreta.");
    }
    // Desliga o relé atual e inicia a próxima rodada
    turnOffRelay(currentTarget.module, currentTarget.relay);
    currentTarget = null;
    nextRound();
  }
});

// Atualiza o temporizador a cada segundo (opcional)
setInterval(() => {
  if (gameRunning) {
    const elapsed = Date.now() - gameStartTime;
    if (elapsed >= gameDuration) {
      endGame();
      return;
    }
    document.getElementById('timer').innerText = "Tempo restante: " + Math.max(0, Math.ceil((gameDuration - elapsed) / 1000)) + "s";
  }
}, 1000);

document.getElementById('startButton').addEventListener('click', () => {
  // Desabilita o botão para evitar múltiplos cliques
  document.getElementById('startButton').disabled = true;

  let count = 3;
  const countdownElem = document.getElementById('countdown');
  countdownElem.innerText = count;

  const intervalId = setInterval(() => {
    count--;
    if (count > 0) {
      countdownElem.innerText = count;
    } else {
      clearInterval(intervalId);
      countdownElem.innerText = "Começou!";
      startGame();  // Chama a função que inicia o jogo
    }
  }, 1000);
});
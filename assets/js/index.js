/* 
      Estados do jogo:
      0 = Idle (pré-jogo)
      1 = Jogo em andamento (running)
      2 = Tela de pontuação final (score screen)
      3 = Tela de ranking (ranking screen)
    */
const ESP_URL = "192.168.0.117"; // Ajuste conforme necessário
let audioPlayed = false;
const startButton = document.getElementById('startButton');
const preGameDiv = document.getElementById('preGame');
const runningGameDiv = document.getElementById('runningGame');
const countdownDiv = document.getElementById('countdown');
const countdownTimerElem = document.getElementById('countdownTimer');
const scoreScreenDiv = document.getElementById('scoreScreen');
const rankingScreenDiv = document.getElementById('rankingScreen');
const scoreElem = document.getElementById('score');
const timerElem = document.getElementById('timer');
const finalScoreElem = document.getElementById('finalScore');
const rankingListElem = document.getElementById('rankingList');

const ws = new WebSocket(`ws://${ESP_URL}/ws`);

// Variáveis do timer
const gameDuration = 20; // duração em segundos
let remainingTime = gameDuration;
let timerInterval = null;

// Crie instâncias globais para os áudios e pré-carregue-os
const countdownAudio = new Audio("assets/audio/countdown.mp3");
countdownAudio.preload = "auto";
countdownAudio.load();

const endAudio = new Audio("assets/audio/end.mp3");
endAudio.preload = "auto";
endAudio.load();

function playAudio(type) {
    if (audioPlayed) return; // Evita tocar se já estiver tocando
    let audio;
    if (type === "countdown") {
        audio = countdownAudio;
    } else if (type === "end") {
        audio = endAudio;
    }
    if (audio) {
        // Reinicia o áudio para garantir que comece do início
        audio.currentTime = 0;
        audio.play()
            .then(() => { audioPlayed = true; })
            .catch(err => console.error("Erro ao tocar áudio:", err));

        // Quando o áudio terminar, reseta audioPlayed para false
        audio.addEventListener("ended", () => {
            audioPlayed = false;
        }, { once: true });
    }
    audioPlayed = true;
}

// Ao clicar no botão, envia requisição para iniciar o jogo
startButton.addEventListener('click', () => {
    startButton.disabled = true;
    ws.send("start-game");
    playAudio('countdown');
});

const handleRanking = () => {
    rankingListElem.innerHTML = "";
    // Assume que o servidor envie um array em data.ranking
    [].forEach((entry, index) => {
        const li = document.createElement('li');
        li.classList.add("mb-2", "p-2", "bg-gray-200", "rounded");
        li.innerText = (index + 1) + "º - " + entry.datetime + " - " + entry.score + " pontos";
        rankingListElem.appendChild(li);
    });
}

ws.onmessage = function (event) {
    const data = JSON.parse(event.data);

    // Atualiza o estado global se presente
    if (data.state !== undefined) {
        if (data.state === 0) {
            preGameDiv.classList.remove('hidden');
            runningGameDiv.classList.add('hidden');
            scoreScreenDiv.classList.add('hidden');
            rankingScreenDiv.classList.add('hidden');
        } else if (data.state === 1) {
            countdownDiv.classList.add('hidden');
            runningGameDiv.classList.remove('hidden');
            scoreScreenDiv.classList.add('hidden');
            rankingScreenDiv.classList.add('hidden');
            scoreElem.innerText = "Pontuação: " + data.score;
            timerElem.textContent = `Tempo restante: ${data.timer}s`;
        } else if (data.state === 2) {
            preGameDiv.classList.add('hidden');
            runningGameDiv.classList.add('hidden');
            scoreScreenDiv.classList.remove('hidden');
            rankingScreenDiv.classList.add('hidden');
            finalScoreElem.innerText = "Pontuação final: " + data.score;
        } else if (data.state === 3) {
            preGameDiv.classList.add('hidden');
            runningGameDiv.classList.add('hidden');
            scoreScreenDiv.classList.add('hidden');
            rankingScreenDiv.classList.remove('hidden');
            handleRanking();
            startButton.disabled = false;
        } else if (data.state === 4) {
            preGameDiv.classList.add('hidden');
            countdownDiv.classList.remove('hidden');
            countdownTimerElem.textContent = `${parseInt(data.timer) + 1}`;
        } else if (data.state === 5 && !audioPlayed) {
            playAudio('end');
        }
    }
};
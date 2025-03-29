/* 
      Estados do jogo:
      0 = Idle (pré-jogo)
      1 = Jogo em andamento (running)
      2 = Tela de pontuação final (score screen)
      3 = Tela de ranking (ranking screen)
    */
const ESP_URL = "192.168.0.117"; // Ajuste conforme necessário
// const ESP_URL = "localhost:3000"; // Ajuste conforme necessário
let audioPlayed = false;
let lastScore = null;
const nomeInput = document.getElementById('nome');
const telefoneInput = document.getElementById('telefone');
const interesseInput = document.getElementById('interesse');
const nomeError = document.getElementById('nomeError');
const telefoneError = document.getElementById('telefoneError');
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
let lastInsertId = null;

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

// Função para formatar o telefone conforme os dígitos digitados
function formatPhone(value) {
    const digits = value.replace(/\D/g, '');
    if (digits.length === 0) return '';
    if (digits.length <= 2) {
        return '(' + digits;
    }
    if (digits.length <= 6) {
        return '(' + digits.slice(0, 2) + ') ' + digits.slice(2);
    }
    if (digits.length <= 10) {
        return '(' + digits.slice(0, 2) + ') ' + digits.slice(2, 6) + '-' + digits.slice(6);
    }
    // Se tiver 11 dígitos
    return '(' + digits.slice(0, 2) + ') ' + digits.slice(2, 7) + '-' + digits.slice(7, 11);
}

// Validação: Nome deve ter pelo menos 3 caracteres (desconsiderando espaços)
function validateName(name) {
    return name.trim().length >= 3;
}

// Validação simples para telefone: 10 ou 11 dígitos (desconsidera formatação)
function validateTelefone(telefone) {
    const digits = telefone.replace(/\D/g, '');
    return digits.length === 10 || digits.length === 11;
}

// Valida os campos e habilita/desabilita o botão "Próximo"
function validateForm() {
    const nomeValid = validateName(nomeInput.value);
    const telefoneValid = validateTelefone(telefoneInput.value);

    nomeError.classList.toggle('hidden', nomeValid);
    telefoneError.classList.toggle('hidden', telefoneValid);

    const formValid = nomeValid && telefoneValid;
    startButton.disabled = !formValid;
    startButton.classList.toggle('opacity-50', !formValid);
    startButton.classList.toggle('cursor-not-allowed', !formValid);
}

// Formata o telefone enquanto o usuário digita e valida o formulário
telefoneInput.addEventListener('input', (e) => {
    const formatted = formatPhone(e.target.value);
    e.target.value = formatted;
    validateForm();
});

// Validação em tempo real para o campo Nome
nomeInput.addEventListener('input', validateForm);

const clearForm = () => {
    nomeInput.value = '';
    telefoneInput.value = '';
    interesseInput.checked = false;
    lastInsertId = null;
};

const handleFormSubmit = async () => {
    const data = JSON.stringify({
        nome: nomeInput.value.trim(),
        telefone: telefoneInput.value.trim(),
        interesse: interesseInput.checked
    });
    return fetch('./leads', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: data
    })
        .then(response => {
            if (!response.ok) {
                alert('Erro ao salvar os dados.');
                return false;
            }
            return response.json();
        })
        .then(result => {
            lastInsertId = result.id;
            return true;
        })
        .catch(error => {
            alert('Erro ao salvar os dados.');
            return false;
        });
};

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
    handleFormSubmit().then(() => {
        ws.send("start-game");
        playAudio('countdown');
    });
});

const handleRanking = (data) => {
    rankingListElem.innerHTML = ''; // Limpa o conteúdo anterior
    data.forEach((item, index) => {
        const linha = document.createElement('tr');

        // Coluna de posição
        const posicao = document.createElement('td');
        posicao.className = 'px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900';
        posicao.textContent = index + 1;
        linha.appendChild(posicao);

        // Coluna de nome
        const name = document.createElement('td');
        name.className = 'px-6 py-4 whitespace-nowrap text-sm text-gray-500';
        name.textContent = item.name;
        linha.appendChild(name);

        // Coluna de pontuação
        const score = document.createElement('td');
        score.className = 'px-6 py-4 whitespace-nowrap text-sm text-gray-500';
        score.textContent = item.score;
        linha.appendChild(score);

        rankingListElem.appendChild(linha);
    });
};

const storeRanking = () => {
    fetch('./ranking', { // Ajuste a URL conforme necessário
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            leads_id: lastInsertId,
            score: lastScore
        })
    })
        .then(response => response.json())
        .then(result => {
            if (result.status === 'success') {
                handleRanking(JSON.parse(result.ranking));
                clearForm();
            } else {
                alert('Erro ao salvar ranking:', result.message);
            }
        })
        .catch(error => alert('Erro na requisição:', error));
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
            if (lastScore != data.score) {
                lastScore = data.score;
                storeRanking();
            }
            finalScoreElem.innerText = "Pontuação final: " + data.score;
        } else if (data.state === 3) {
            preGameDiv.classList.add('hidden');
            runningGameDiv.classList.add('hidden');
            scoreScreenDiv.classList.add('hidden');
            rankingScreenDiv.classList.remove('hidden');
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
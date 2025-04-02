const queryString = window.location.search;
const urlParams = new URLSearchParams(queryString);
const registration = urlParams.get('registration');
/* 
      Estados do jogo:
      0 = Idle (pré-jogo/formulário)
      1 = Jogo em andamento (running)
      2 = Tela de pontuação final (score screen)
      3 = Tela de ranking (ranking screen/tela de descanso)
      4 = Contagem regressiva
      5 = Fim (relés acesos)
    */
const ESP_URL = "192.168.0.111"; // Ajuste conforme necessário
// const ESP_URL = "localhost:3000"; // Ajuste conforme necessário
let audioPlayed = false;
let lastScore = null;
const nomeInput = document.getElementById('nome');
const nomeDiv = document.getElementById('nomeDiv');
const telefoneInput = document.getElementById('telefone');
const interesseInput = document.getElementById('interesse');
const interesseDiv = document.getElementById('interesseDiv');
const nomeError = document.getElementById('nomeError');
const telefoneError = document.getElementById('telefoneError');
const startButton = document.getElementById('startButton');
const startGameButton = document.getElementById('startGameButton');
const preGameDiv = document.getElementById('preGame');
const userDataConfirmation = document.getElementById('userDataConfirmation');
const userDataConfirmationText = document.getElementById('userDataConfirmationText');
const runningGameDiv = document.getElementById('runningGame');
const countdownDiv = document.getElementById('countdown');
const countdownTimerElem = document.getElementById('countdownTimer');
const scoreScreenDiv = document.getElementById('scoreScreen');
const rankingScreenDiv = document.getElementById('rankingScreen');
const scoreElem = document.getElementById('score');
const timerElem = document.getElementById('timer');
const finalScoreElem = document.getElementById('finalScore');
const rankingListElem = document.getElementById('rankingList');
let currentUser = {};
let returnToRankingTimeout = null;

// Variável para armazenar a conexão WebSocket
let ws = null;
let wsReconnectTimer = null;
let pendingMessages = [];

// Prefixo padrão para URLs de API - Detecta automaticamente se estamos em localhost ou em produção
const BASE_URL = window.location.hostname.includes('localhost') || window.location.hostname.includes('127.0.0.1') 
    ? window.location.origin         // Em desenvolvimento: usa a origem atual
    : '';                           // Em produção: caminho relativo

// Configura os endpoints da API
const API = {
    leads: BASE_URL + '/leads',
    ranking: {
        get: BASE_URL + '/ranking/data',
        post: BASE_URL + '/ranking'
    }
};

// Função para conectar ou reconectar o WebSocket
function connectWebSocket() {
    // Limpe qualquer timer de reconexão
    if (wsReconnectTimer) {
        clearTimeout(wsReconnectTimer);
        wsReconnectTimer = null;
    }
    
    // Se já existe uma conexão, feche-a antes de criar uma nova
    if (ws) {
        try {
            ws.close();
        } catch (e) {
            console.error("Erro ao fechar WebSocket:", e);
        }
    }
    
    // Cria nova conexão WebSocket
    ws = new WebSocket(`ws://${ESP_URL}/ws`);
    
    // Configura handlers de eventos para o WebSocket
    ws.onopen = function() {
        console.log("WebSocket conectado");
        // Envia mensagens pendentes, se houver
        if (pendingMessages.length > 0) {
            console.log(`Enviando ${pendingMessages.length} mensagens pendentes`);
            pendingMessages.forEach(msg => {
                sendMessage(msg, false); // Não coloca na fila novamente se falhar
            });
            pendingMessages = [];
        }
        
        // Verifica se está na tela de ranking e carrega os dados se necessário
        if (!rankingScreenDiv.classList.contains('hidden') && !rankingLoaded) {
            console.log("Reconectado na tela de ranking, recarregando dados...");
            loadRanking(true);
        }
    };
    
    ws.onclose = function() {
        console.log("WebSocket fechado. Tentando reconectar em 2 segundos...");
        // Agenda uma reconexão após 2 segundos
        wsReconnectTimer = setTimeout(connectWebSocket, 2000);
    };
    
    ws.onerror = function(err) {
        console.error("Erro no WebSocket:", err);
        // O evento onclose será disparado após um erro, então não precisamos iniciar 
        // a reconexão aqui
    };
    
    ws.onmessage = function (event) {
        const data = JSON.parse(event.data);

        // Reseta o estado do ranking se mudar para outro estado que não seja o ranking
        if (data.state !== undefined && data.state !== 3) {
            resetRankingState();
        }

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
                // Estado inicial (tela de descanso)
                preGameDiv.classList.add('hidden');
                runningGameDiv.classList.add('hidden');
                scoreScreenDiv.classList.add('hidden');
                rankingScreenDiv.classList.remove('hidden');
                startButton.disabled = false;
                
                // Carregar o ranking apenas se ainda não foi carregado
                if (!rankingLoaded) {
                    loadRanking(true);
                }
            } else if (data.state === 4) {
                preGameDiv.classList.add('hidden');
                countdownDiv.classList.remove('hidden');
                // Adiciona animação pop ao mostrar o número do contador
                countdownTimerElem.textContent = `${parseInt(data.timer) + 1}`;
                countdownTimerElem.classList.remove('pop-in');
                void countdownTimerElem.offsetWidth; // Força recálculo para reiniciar a animação
                countdownTimerElem.classList.add('pop-in');
            } else if (data.state === 5 && !audioPlayed) {
                playAudio('end');
            }
        }
    };
}

if(!registration) {
    // Inicializa a conexão WebSocket
    connectWebSocket();
}

// Função para enviar mensagens com verificação de estado
function sendMessage(message, queueIfFailed = true) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        try {
            ws.send(message);
            console.log(`Mensagem enviada: ${message}`);
            return true;
        } catch (e) {
            console.error(`Erro ao enviar mensagem: ${e}`);
            if (queueIfFailed) {
                pendingMessages.push(message);
                console.log(`Mensagem "${message}" adicionada à fila de pendentes`);
            }
            return false;
        }
    } else {
        console.warn(`WebSocket não está aberto (estado: ${ws ? ws.readyState : 'null'}). Tentando reconectar...`);
        if (queueIfFailed) {
            pendingMessages.push(message);
            console.log(`Mensagem "${message}" adicionada à fila de pendentes`);
        }
        connectWebSocket(); // Tenta reconectar
        return false;
    }
}

// Variáveis do timer
const gameDuration = 20; // duração em segundos
let remainingTime = gameDuration;
let timerInterval = null;

// Flags para controlar a validação
let nomeInteracted = false;
let telefoneInteracted = false;

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

    // Só mostra os erros se o usuário já interagiu com os campos
    nomeError.classList.toggle('hidden', nomeValid || !nomeInteracted);
    telefoneError.classList.toggle('hidden', telefoneValid || !telefoneInteracted);

    // O botão sempre é habilitado/desabilitado com base na validação
    const formValid = telefoneValid;
    startButton.disabled = !formValid;
    startButton.classList.toggle('opacity-50', !formValid);
    startButton.classList.toggle('cursor-not-allowed', !formValid);
}

// Formata o telefone enquanto o usuário digita e valida o formulário
telefoneInput.addEventListener('input', (e) => {
    telefoneInteracted = true;
    const formatted = formatPhone(e.target.value);
    e.target.value = formatted;
    validateForm();
});

// Validação em tempo real para o campo Nome
nomeInput.addEventListener('input', (e) => {
    nomeInteracted = true;
    validateForm();
});

// Evento blur para marcar que o usuário interagiu com os campos
nomeInput.addEventListener('blur', () => {
    nomeInteracted = true;
    validateForm();
});

telefoneInput.addEventListener('blur', () => {
    telefoneInteracted = true;
    validateForm();
});

const clearForm = () => {
    nomeInput.value = '';
    telefoneInput.value = '';
    interesseInput.checked = false;
    currentUser = {};
    
    // Reinicia as flags de interação
    nomeInteracted = false;
    telefoneInteracted = false;
    
    // Oculta mensagens de erro
    nomeError.classList.add('hidden');
    telefoneError.classList.add('hidden');
};

const handleFormSubmit = async () => {
    const data = JSON.stringify({
        nome: nomeInput.value.trim(),
        telefone: telefoneInput.value.trim(),
        interesse: interesseInput.checked
    });
    return fetch(API.leads, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: data
    })
        .then(response => {
            if (!response.ok) {
                alert('Erro ao salvar os dados.');
                throw new Error('Erro ao salvar os dados.');
            } else {
                return response.json();
            }
        })
        .then(result => {
            return result;
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

// Botão "Começar" na tela de ranking
startGameButton.addEventListener('click', () => {
    // Envia mensagem para o backend mudar para o estado IDLE (formulário)
    sendMessage("show-form");
    // Esconde a tela de ranking e mostra a tela de formulário
    rankingScreenDiv.classList.add('hidden');
    preGameDiv.classList.remove('hidden');
    clearForm();
    validateForm();
});

// Botão "Iniciar" na tela de formulário
startButton.addEventListener('click', () => {
    startButton.disabled = true;
    handleFormSubmit().then((result) => {
        if(result?.no_data == true) {
            nomeDiv.classList.remove('hidden');
            interesseDiv.classList.remove('hidden');
            termosDiv.classList.remove('hidden');
            startButton.disabled = false;
        } else {
            currentUser = {
                id: result.id,
                nome: result.nome
            };
            if(!registration) {
                // Envia mensagem para iniciar o jogo
                sendMessage("start-countdown");
                setTimeout(() => {
                    playAudio('countdown');
                }, 10);
            } else {
                preGameDiv.classList.add('hidden');
                userDataConfirmation.classList.remove('hidden');
                let tempName = currentUser.nome.split(' ');
                userDataConfirmationText.textContent = "Seja bem vindo(a) " + tempName[0] + "!";
                setTimeout(() => {
                    location.reload(true);
                }, 5000);
            }
        }
    });
});

const handleRanking = (data) => {
    rankingListElem.innerHTML = ''; // Limpa o conteúdo anterior

    if (!Array.isArray(data) || data.length === 0) {
        showRankingMessage("Nenhum dado de ranking disponível"); // Usa a função adaptada
        return;
    }
    // -- Edição: Limita a lista aos 7 primeiros ---
    const top7Data = data.slice(0, 7);

    top7Data.forEach((item, index) => { // <-- Iterar sobre o top7data
        // Criar o div principal para o item do ranking
        const rankingItem = document.createElement('div');
        // Adiciona classes base e de animação
        rankingItem.className = 'ranking-item relative h-[100px] mb-2 ranking-animate'; // Adicionado mb-2 para espaçamento e altura fixa
        rankingItem.style.setProperty('--ranking-index', index); // Para animação sequencial

        // Destacar a linha do usuário atual, se disponível
        if (currentUser.id && item.leads_id === currentUser.id) {
            rankingItem.classList.add('current-user');
        }

        // Criar container para o conteúdo (Nome e Pontuação) usando Flexbox
        const contentContainer = document.createElement('div');
        contentContainer.className = 'flex justify-between items-center h-full px-32 relative z-10'; // Usa flex, alinha itens, padding e z-index

        // Preparar os dados para exibição
        const playerName = item.name || "Jogador Anônimo";
        const playerScore = item.score || 0;

        // Elemento para o nome (com capitalização)
        const nameElement = document.createElement('span');
        nameElement.className = 'ranking-name text-black font-normal text-3xl'; // Ajuste de classes
        // Capitaliza o nome (lógica existente)
        const nameParts = playerName.split(' ');
        const capitalizedName = nameParts.map(part => part.length > 0 ? part[0].toUpperCase() + part.slice(1).toLowerCase() : part).join(' ');
        nameElement.textContent = capitalizedName;

        // Elemento para a pontuação
        const scoreElement = document.createElement('span');
        scoreElement.className = 'ranking-score text-black font-normal text-3xl'; // Ajuste de classes
        scoreElement.textContent = playerScore;

        contentContainer.appendChild(nameElement);
        contentContainer.appendChild(scoreElement);

        // Adiciona o container de conteúdo ao item principal
        rankingItem.appendChild(contentContainer);

        // Adiciona o background (input.png) como um elemento div separado
        const bgElement = document.createElement('div');
        // Usa as classes existentes para o fundo, ajustadas para div
        bgElement.className = 'ranking-item-bg absolute inset-0 z-0 pointer-events-none';
        bgElement.style.background = "url('/assets/images/input.png') no-repeat center bottom"; // Aplica o background via style
        bgElement.style.backgroundSize = '100% 100%'; // Garante que cubra o div
        bgElement.style.transform = 'rotate(2deg)'; // Mantém a rotação se desejado

        rankingItem.appendChild(bgElement); // Adiciona o background div

        rankingListElem.appendChild(rankingItem); // Adiciona o item completo à lista
    });

    // Se não houver dados, mostra uma mensagem (usando a função adaptada)
    if (rankingListElem.children.length === 0) {
        showRankingMessage("Nenhum dado de ranking disponível");
    }
};

const storeRanking = () => {
    fetch(API.ranking.post, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            leads_id: currentUser.id,
            score: lastScore
        })
    })
        .then(response => response.json())
        .then(result => {
            if (result.status === 'success') {
                console.log("Ranking salvo com sucesso");
                try {
                    // O servidor está retornando um JSON duplo codificado para o ranking
                    if (result.ranking) {
                        const rankingData = JSON.parse(result.ranking);
                        if (Array.isArray(rankingData) && rankingData.length > 0) {
                            handleRanking(rankingData);
                            rankingLoaded = true; // Marca como carregado
                        }
                    } else {
                        // Se não tiver ranking na resposta, carrega do endpoint normal
                        // apenas se ainda não temos dados
                        if (!rankingLoaded) {
                            loadRanking(true);
                        }
                    }
                } catch (e) {
                    console.error("Erro ao processar o ranking:", e);
                    // Fallback: busca o ranking do endpoint normal apenas se necessário
                    if (!rankingLoaded) {
                        loadRanking(true);
                    }
                }
                
                clearForm();
                // Após salvar o ranking, exibe por 5 segundos e depois volta para a tela inicial de ranking
                if (returnToRankingTimeout) {
                    clearTimeout(returnToRankingTimeout);
                }
                returnToRankingTimeout = setTimeout(() => {
                    scoreScreenDiv.classList.add('hidden');
                    rankingScreenDiv.classList.remove('hidden');
                }, 5000);
            } else {
                console.error('Erro ao salvar ranking:', result.message);
                alert('Erro ao salvar ranking.');
            }
        })
        .catch(error => {
            console.error('Erro na requisição:', error);
            alert('Erro ao salvar seus dados.');
        });
}

// Dados mockados para o ranking (usado como fallback se o servidor não responder)
const MOCK_RANKING_DATA = [
    { name: "Jogador 1", score: 18, leads_id: null },
    { name: "Jogador 2", score: 15, leads_id: null },
    { name: "Jogador 3", score: 14, leads_id: null },
    { name: "Jogador 4", score: 12, leads_id: null },
    { name: "Jogador 5", score: 10, leads_id: null }
];

// Variável para controlar tentativas de carregamento
let rankingLoadAttempts = 0;
const MAX_LOAD_ATTEMPTS = 3;

// Flag para evitar carregamentos repetidos quando já temos dados
let rankingLoaded = false;
let rankingLoadInterval = null;

// Função para carregar o ranking
function loadRanking(forceReload = false) {
    // Se já temos os dados e não é forçado, não faz nada
    if (rankingLoaded && !forceReload) {
        console.log("Ranking já carregado, ignorando solicitação");
        return;
    }
    
    console.log("Carregando dados do ranking...", API.ranking.get);
    
    // Desabilita o botão enquanto carrega
    if (startGameButton) {
        startGameButton.style.opacity = "0.5";
        startGameButton.style.pointerEvents = "none";
    }
    
    // Mostra um indicador de carregamento se a tabela estiver vazia
    if (rankingListElem.innerHTML.trim() === '') {
        showRankingMessage("Carregando ranking...");
    }
    
    // Incrementa o contador de tentativas
    rankingLoadAttempts++;
    
    // Usa o endpoint correto para obter os dados do ranking
    fetch(API.ranking.get)
        .then(response => {
            if (!response.ok) {
                throw new Error('Erro ao buscar ranking: ' + response.status);
            }
            return response.json();
        })
        .then(data => {
            console.log("Ranking carregado com sucesso", data);
            // Reseta o contador de tentativas ao ter sucesso
            rankingLoadAttempts = 0;
            
            // Marca que o ranking foi carregado com sucesso
            rankingLoaded = true;
            
            // Reabilita o botão
            if (startGameButton) {
                startGameButton.style.opacity = "";
                startGameButton.style.pointerEvents = "";
            }
            
            if (Array.isArray(data) && data.length > 0) {
                handleRanking(data);
            } else {
                console.warn("Ranking vazio ou inválido", data);
                if (rankingLoadAttempts >= MAX_LOAD_ATTEMPTS) {
                    console.log("Usando dados mockados após várias tentativas sem sucesso");
                    handleRanking(MOCK_RANKING_DATA);
                } else {
                    showRankingMessage("Nenhum dado de ranking disponível");
                }
            }
        })
        .catch(error => {
            console.error('Erro ao buscar ranking:', error);
            
            // Reabilita o botão mesmo em caso de erro
            if (startGameButton) {
                startGameButton.style.opacity = "";
                startGameButton.style.pointerEvents = "";
            }
            
            // Se houve várias tentativas sem sucesso, usa os dados mockados
            if (rankingLoadAttempts >= MAX_LOAD_ATTEMPTS) {
                console.log("Usando dados mockados após várias tentativas sem sucesso");
                handleRanking(MOCK_RANKING_DATA);
                rankingLoaded = true; // Evita novas tentativas
                return;
            }
            
            // Mostrar mensagem útil para erros de CORS (comum em desenvolvimento)
            if (error.message && error.message.includes('NetworkError') || 
                error.name === 'TypeError' || 
                error.message.includes('Failed to fetch')) {
                console.error('Possível erro de CORS ou conexão. Verifique se o servidor está rodando e o CORS está configurado corretamente.');
                showRankingMessage("Erro de conexão com o servidor");
            } else {
                showRankingMessage("Não foi possível carregar o ranking");
            }
            
            // Tentar novamente após 5 segundos em caso de erro (apenas se não tiver dados ainda)
            if (!rankingLoaded) {
                setTimeout(() => loadRanking(true), 5000);
            }
        });
}

// Função auxiliar para exibir mensagens no ranking com o estilo correto (adaptada para div)
function showRankingMessage(message) {
    const messageDiv = `
        <div class="ranking-item relative h-[100px] mb-2 fade-in flex items-center justify-center text-center">
            <div class="ranking-item-bg absolute inset-0 z-0 pointer-events-none" style="background: url('/assets/images/input.png') no-repeat center bottom; background-size: 100% 100%; transform: rotate(2deg);"></div>
            <span class="relative z-10 text-black font-normal text-2xl">${message}</span>
        </div>`;
    rankingListElem.innerHTML = messageDiv;
}

// Ao carregar a página, verificar se o estado inicial é o ranking
window.addEventListener('load', () => {
    if(!registration) {
        // Iniciar no estado de ranking (tela de descanso)
        preGameDiv.classList.add('hidden');
        runningGameDiv.classList.add('hidden');
        scoreScreenDiv.classList.add('hidden');
        rankingScreenDiv.classList.remove('hidden');
        
        // Buscar ranking inicial (apenas uma vez)
        loadRanking(true);
        
        // Limpa qualquer intervalo antigo
        if (rankingLoadInterval) {
            clearInterval(rankingLoadInterval);
        }
        
        // Definir um intervalo para recarregar o ranking periodicamente (a cada 5 minutos)
        // apenas para manter os dados sincronizados com o servidor
        rankingLoadInterval = setInterval(() => loadRanking(true), 5 * 60 * 1000);
    }
});

// Função para resetar o estado do ranking
function resetRankingState() {
  rankingLoaded = false;
  rankingLoadAttempts = 0;
  
  // Limpa qualquer intervalo existente
  if (rankingLoadInterval) {
    clearInterval(rankingLoadInterval);
    rankingLoadInterval = null;
  }
}
let lastTimestamp = 0; // Armazena o último timestamp conhecido
const rankingListElem = document.getElementById('rankingList');

function checkForUpdate() {
    fetch('/ranking/check-update/?lastTimestamp=' + lastTimestamp)
        .then(response => response.json())
        .then(data => {
            if (data.update) {
                // Se houve atualização, atualiza o lastTimestamp e busca os dados atualizados
                lastTimestamp = data.lastTimestamp;
                fetchRankingData();
            }
        })
        .catch(error => console.error('Erro no check_update:', error));
}

function fetchRankingData() {
    fetch('/ranking/data')
        .then(response => response.json())
        .then(data => {
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
        })
        .catch(error => console.error('Erro ao buscar ranking data:', error));
}

// Verifica a cada 5 segundos
setInterval(checkForUpdate, 5000);
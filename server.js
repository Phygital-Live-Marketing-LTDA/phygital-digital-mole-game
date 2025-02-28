const express = require('express');
const path = require('path');
const fs = require('fs'); // Para manipular arquivos JSON
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);

// Permite o uso de JSON no body das requisições
app.use(express.json());
// Serve arquivos estáticos da pasta "public"
app.use(express.static(path.join(__dirname, 'public')));

// Configuração da porta serial – ajuste 'COM5' e baudRate conforme seu hardware
const serialPort = new SerialPort({
  path: 'COM5',
  baudRate: 19200,
  autoOpen: false
});

// Abre a porta serial
serialPort.open((err) => {
  if (err) {
    return console.error('Erro ao abrir a porta serial:', err.message);
  }
  console.log('Porta serial COM5 aberta com sucesso!');
});

// Cria um parser que lê os dados linha a linha (delimitador "\n")
const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

// Quando receber dados da porta serial...
parser.on('data', (data) => {
  data = data.trim();
  console.log('Recebido via serial:', data);
  // Se for do tipo "BTN:x", emite para os clientes via Socket.IO
  if (data.startsWith('BTN:')) {
    const parts = data.split(':');
    const btnNumber = parseInt(parts[1], 10);
    if (!isNaN(btnNumber)) {
      io.emit('button_pressed', { button: btnNumber });
      console.log(`Evento 'button_pressed' emitido: botão ${btnNumber}`);
    }
  }
});

// Endpoint para enviar comando ao Arduino (acionar relés)
app.post('/send-command', (req, res) => {
  const { command } = req.body;
  if (!command) {
    return res.status(400).json({ error: 'Comando não fornecido' });
  }
  // Se o comando não terminar com '#', adiciona
  let cmd = command;
  if (!cmd.endsWith('#')) {
    cmd += '#';
  }
  // Envia o comando via serial
  serialPort.write(cmd, (err) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }
    res.json({ message: `Comando '${cmd}' enviado com sucesso!` });
  });
});

// ====================
// Endpoints para Ranking
// ====================

const rankingFile = path.join(__dirname, 'ranking.json');

// Função para carregar o ranking do arquivo JSON
function loadRanking() {
  if (!fs.existsSync(rankingFile)) {
    fs.writeFileSync(rankingFile, JSON.stringify([]));
  }
  const data = fs.readFileSync(rankingFile, "utf8");
  return JSON.parse(data);
}

// Função para salvar o ranking no arquivo JSON
function saveRanking(ranking) {
  fs.writeFileSync(rankingFile, JSON.stringify(ranking, null, 2));
}

// Endpoint para salvar os dados do jogo
// Exemplo de corpo da requisição: { "name": "Jogador1", "score": 12 }
app.post('/save-game', (req, res) => {
  const { name, score } = req.body;
  if (typeof name !== "string" || typeof score !== "number") {
    return res.status(400).json({ error: "Dados inválidos" });
  }

  // Carrega ranking atual
  const ranking = loadRanking();
  
  // Adiciona os dados do jogo com um timestamp
  ranking.push({ name, score, timestamp: Date.now() });
  
  // Ordena o ranking do maior para o menor score
  ranking.sort((a, b) => b.score - a.score);
  
  // Salva o ranking atualizado
  saveRanking(ranking);
  
  res.json({ message: "Dados salvos com sucesso" });
});

// Endpoint para retornar o ranking ordenado (top 10)
app.get('/ranking', (req, res) => {
  const ranking = loadRanking();
  
  // Ordena novamente, por segurança (ordem decrescente)
  ranking.sort((a, b) => b.score - a.score);
  
  // Retorna os 10 primeiros
  res.json(ranking.slice(0, 10));
});

// ====================
// Configuração do Socket.IO
// ====================
io.on('connection', (socket) => {
  console.log('Cliente conectado via Socket.IO');
});

// Inicia o servidor HTTP na porta 3000
const PORT = process.env.PORT || 3000;
http.listen(PORT, () => {
  console.log(`Servidor rodando na porta ${PORT}`);
});

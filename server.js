const express = require('express');
const path = require('path');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);

// Permite o uso de JSON no body das requisições
app.use(express.json());
// Serve arquivos estáticos da pasta "public"
app.use(express.static(path.join(__dirname, 'public')));

// Configuração da porta serial – ajuste 'COM3' e baudRate conforme seu hardware
const serialPort = new SerialPort({
  path: 'COM3',
  baudRate: 19200,
  autoOpen: false
});

// Abre a porta serial
serialPort.open((err) => {
  if (err) {
    return console.error('Erro ao abrir a porta serial:', err.message);
  }
  console.log('Porta serial COM3 aberta com sucesso!');
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

// Configuração do Socket.IO
io.on('connection', (socket) => {
  console.log('Cliente conectado via Socket.IO');
});

// Inicia o servidor HTTP na porta 3000
const PORT = process.env.PORT || 3000;
http.listen(PORT, () => {
  console.log(`Servidor rodando na porta ${PORT}`);
});

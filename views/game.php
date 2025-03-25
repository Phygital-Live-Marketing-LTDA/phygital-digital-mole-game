<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title><?php echo $data['titulo']; ?></title>
  <meta name="referrer" content="no-referrer">
  <script src="/assets/js/tailwind.js"></script>
</head>
<body class="bg-gray-100 min-h-screen flex items-center justify-center">
  <div class="bg-white shadow-lg rounded-lg p-8 max-w-md w-full">
    <h1 class="text-3xl font-bold text-center mb-6">Jogo dos Relés ESP</h1>
    
    <!-- Tela pré-jogo (idle) -->
    <div id="preGame" class="flex flex-col items-center">
      <button id="startButton" class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded">
        Iniciar Jogo
      </button>
    </div>
    
    <!-- Tela do countdown -->
    <div id="countdown" class="hidden flex flex-col items-center">
      <p id="countdownTimer" class="text-3xl font-medium">3</p>
    </div>
    
    <!-- Tela do jogo em andamento -->
    <div id="runningGame" class="hidden flex flex-col items-center">
      <p id="score" class="text-xl font-medium">Pontuação: 0</p>
      <p id="timer" class="text-xl font-medium">Tempo restante: 20s</p>
    </div>
    
    <!-- Tela de pontuação final (exibida por 5 segundos) -->
    <div id="scoreScreen" class="hidden flex flex-col items-center">
      <h2 class="text-2xl font-bold mb-4">Fim do Jogo!</h2>
      <p id="finalScore" class="text-xl">Pontuação final: 0</p>
    </div>
    
    <!-- Tela de ranking (exibida por 10 segundos) -->
    <div id="rankingScreen" class="hidden flex flex-col items-center">
      <table class="min-w-full divide-y divide-gray-200">
        <thead class="bg-gray-50">
          <tr>
            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Posição</th>
            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Nome</th>
            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Pontuação</th>
          </tr>
        </thead>
        <tbody id="rankingList" class="bg-white divide-y divide-gray-200"></tbody>
      </table>
    </div>
  </div>
</body>
<script src="assets/js/game.js"></script>
</html>
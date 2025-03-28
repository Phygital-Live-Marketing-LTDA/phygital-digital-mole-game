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
    <div id="preGame" class="flex flex-col items-center gap-4">
      <h1 class="text-3xl font-bold text-center mb-6">Cadastro</h1>
      <form id="registrationForm" class="space-y-4">
        <!-- Campo Nome -->
        <div>
          <label for="nome" class="block text-sm font-medium text-gray-700">Nome</label>
          <input type="text" id="nome" name="nome" placeholder="Seu nome"
            class="mt-1 block w-full border border-gray-300 rounded-md p-2">
          <p id="nomeError" class="text-red-500 text-xs mt-1 hidden">Nome deve ter pelo menos 3 caracteres.</p>
        </div>
        
        <!-- Campo Telefone -->
        <div>
          <label for="telefone" class="block text-sm font-medium text-gray-700">Telefone</label>
          <input type="text" id="telefone" name="telefone" placeholder="(00) 00000-0000"
            class="mt-1 block w-full border border-gray-300 rounded-md p-2">
          <p id="telefoneError" class="text-red-500 text-xs mt-1 hidden">Telefone inválido.</p>
        </div>
        
        <!-- Checkbox de Interesse -->
        <div class="flex items-center">
          <input id="interesse" name="interesse" type="checkbox"
            class="h-4 w-4 text-blue-600 border-gray-300 rounded">
          <label for="interesse" class="ml-2 block text-sm text-gray-900">
            Quero saber mais sobre a Phygital
          </label>
        </div>
      </form>
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
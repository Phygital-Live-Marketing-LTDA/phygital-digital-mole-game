<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title><?php echo $data['titulo']; ?></title>
  <meta name="referrer" content="no-referrer">
  <script src="/assets/js/tailwind.js"></script>
</head>

<style>
  .divider td {
    height: 4px;
    padding: 0; /* remove padding para ajustar a altura */
    background: url('/assets/images/input.png') no-repeat center;
    background-size: cover;
    border: none;
  }
</style>

<body class="bg-[url('/assets/images/bg.png')] bg-cover bg-center bg-no-repeat flex items-center justify-center min-h-screen overflow-hidden">
    
    <!-- Tela pré-jogo (idle) -->
    <div id="preGame" class="w-screen h-screen flex flex-col justify-center bg-[url('/assets/images/bg.png')] bg-cover bg-center bg-no-repeat ">
      
    <div class="fixed top-20 left-1/2 -translate-x-1/2 object-contain items-center mx-auto">
      <img src="/assets/images/umalogo.png"  class="h-[5vh]">
    </div>
      <form id="registrationForm" class="space-y-4 mx-12">
        <!-- Campo Nome -->
        <div>
          <input type="text" id="nome" name="nome" placeholder="Nome"
            class="mt-1 bg-transparent w-full object-cover p-2 outline-none text-xl text-black placeholder-black">
            <img src="/assets/images/input.png"  class="-mt-6 object-contain">
          <p id="nomeError" class="text-red-500 text-xs mt-1 hidden">Nome deve ter pelo menos 3 caracteres.</p>
        </div>
        
        <!-- Campo Telefone -->
        <div>
          <input type="number" id="telefone" name="telefone" placeholder="Telefone"
          class="mt-1 bg-transparent w-full object-cover p-2 outline-none text-xl text-black placeholder-black">
            <img src="/assets/images/input.png"  class="-mt-6 object-contain">
          <p id="telefoneError" class="text-red-500 text-xs mt-1 hidden">Telefone inválido.</p>
        </div>
        
        <div>

    
        <!-- Checkbox de Interesse -->
        <div class="flex items-center">
         
          <label for="interesse" class="ml-2 mt-2 block text-sm text-gray-900 text-xl">
            Quero saber mais sobre a Phygital
          </label>

          <input id="interesse" name="interesse" type="checkbox"
            class="h-5 w-5 text-blue-600 border-black rounded mx-2 mt-2">
        </div>
        <img src="/assets/images/input.png"  class="-mt-3 object-contain">

        </div>
      </form>
      <img src="/assets/images/botao-iniciar.png" id="startButton" class="w-[90vw] object-contain -mt-4 mx-auto">
     
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

    <div id="rankingScreen" class="hidden bg-[url('/assets/images/bg.png')] bg-cover bg-center bg-no-repeat flex items-center justify-center h-screen w-screen overflow-hidden">
<div class="fixed top-0">
   <img src="/assets/images/ranking1.png" class="object-contain no-repeat" />
</div>  
<div class="fixed top-0 right-0">
  <img src="/assets/images/ranking2.png" class="object-contain no-repeat h-[50vh]" />
</div>
<div class="container mx-auto p-4">
    <div class="overflow-x-auto">
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
</div>



</body>
<script src="assets/js/game.js"></script>
</html>
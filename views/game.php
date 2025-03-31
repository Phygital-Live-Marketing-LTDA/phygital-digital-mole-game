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
  .input-container {
    position: relative;
    margin-bottom: 20px;
  }
  
  .custom-input {
    width: 100%;
    background: transparent;
    padding: 12px 16px;
    outline: none;
    font-size: 4rem;
    color: black;
    position: relative;
    z-index: 10;
  }
  
  .input-bg {
    background: url('/assets/images/input.png') no-repeat center;
    background-size: 100% 100%;
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    z-index: 1;
  }
  
  .divider td {
    height: 2rem;
    padding: 0;
    background: url('/assets/images/input.png') no-repeat center;
    background-size: cover;
    border: none;
  }
  
  /* Estilos para a tabela de ranking com o background input.png */
  .ranking-row {
    position: relative;
    height: 100px;
    margin-bottom: 8px;
    border-spacing: 0;
    border-collapse: separate;
  }
  
  .ranking-cell {
    position: relative;
    z-index: 10;
    background: transparent !important;
    color: black;
    font-weight: 400;
    font-size: 2rem !important;
    margin-bottom: 30px;
    
  }
  
  .ranking-row-bg {
    transform: rotate(2deg);
    background: url('/assets/images/input.png') no-repeat center bottom;
    background-size: 100% 100%;
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    z-index: 1;
    pointer-events: none;
  }
  
  .ranking-table table {
    background: transparent !important;
    border-spacing: 0 8px;
    border-collapse: separate;
    width: 100%;
  }
  
  .ranking-table thead th {
    padding: 12px 16px;
    font-weight: bold;
    color: #333;
    opacity: 0;
    font-size: 1.2rem;
    border-radius: 8px;
    margin-bottom: 30px;
  }
  
  .ranking-table tbody tr {
    margin-bottom: 30px;
  }
  
  .ranking-table tbody td:first-child {
    border-top-left-radius: 10px;
    border-bottom-left-radius: 10px;
  }
  
  .ranking-table tbody td:last-child {
    border-top-right-radius: 10px;
    border-bottom-right-radius: 10px;
  }
  
  @media (min-width: 768px) and (max-width: 1024px) {
    .custom-input {
      padding: 2px 10px;
      font-size: 2rem;
    }
    
    .ranking-table {
      max-width: 90%;
      margin: 0 auto;
    }
    

  }
  
  .current-user .ranking-cell {
    font-weight: bold;
    color: #000000;
  }
  
  .current-user .ranking-row-bg {
    background: url('/assets/images/input.png') no-repeat center;
    background-size: 100% 100%;
  }
  
  /* Animações de Fade e Pop */
  @keyframes fadeIn {
    from { 
      opacity: 0; 
    }
    to { 
      opacity: 1; 
    }
  }
  
  @keyframes popIn {
    0% {
      transform: scale(0.6);
      opacity: 0;
    }
    70% {
      transform: scale(1.1);
    }
    100% {
      transform: scale(1);
      opacity: 1;
    }
  }
  
  /* Classes de animação de fade */
  .fade-in {
    animation: fadeIn 0.8s ease forwards;
    opacity: 0;
  }
  
  .fade-in-delay-100 {
    animation: fadeIn 0.8s ease forwards;
    animation-delay: 0.1s;
    opacity: 0;
  }
  
  .fade-in-delay-200 {
    animation: fadeIn 0.8s ease forwards;
    animation-delay: 0.2s;
    opacity: 0;
  }
  
  .fade-in-delay-300 {
    animation: fadeIn 0.8s ease forwards;
    animation-delay: 0.3s;
    opacity: 0;
  }
  
  .fade-in-delay-400 {
    animation: fadeIn 0.8s ease forwards;
    animation-delay: 0.4s;
    opacity: 0;
  }
  
  .fade-in-delay-500 {
    animation: fadeIn 0.8s ease forwards;
    animation-delay: 0.5s;
    opacity: 0;
  }
  
  /* Classes de animação de pop */
  .pop-in {
    animation: popIn 0.6s ease forwards;
    opacity: 0;
  }
  
  .pop-in-delay-100 {
    animation: popIn 0.6s ease forwards;
    animation-delay: 0.1s;
    opacity: 0;
  }
  
  .pop-in-delay-200 {
    animation: popIn 0.6s ease forwards;
    animation-delay: 0.2s;
    opacity: 0;
  }
  
  .pop-in-delay-300 {
    animation: popIn 0.6s ease forwards;
    animation-delay: 0.3s;
    opacity: 0;
  }
  
  .pop-in-delay-400 {
    animation: popIn 0.6s ease forwards;
    animation-delay: 0.4s;
    opacity: 0;
  }
  
  .pop-in-delay-500 {
    animation: popIn 0.6s ease forwards;
    animation-delay: 0.5s;
    opacity: 0;
  }
  
  /* Variações mais lentas */
  .fade-in-slow {
    animation: fadeIn 1.2s ease forwards;
    opacity: 0;
  }
  
  .pop-in-slow {
    animation: popIn 1s ease forwards;
    opacity: 0;
  }
  
  /* Animação para itens de ranking com aparecimento sequencial */
  .ranking-animate {
    animation: fadeIn 0.5s ease forwards;
    animation-delay: calc(0.1s * var(--ranking-index, 0));
    opacity: 0;
  }
</style>

<body class="bg-[url('/assets/images/bg.png')] bg-cover bg-center bg-no-repeat flex items-center justify-center min-h-screen overflow-hidden">
    
    <!-- Tela pré-jogo (idle) -->
    <div id="preGame" class="w-screen h-screen flex flex-col justify-center bg-[url('/assets/images/bg.png')] bg-cover bg-center bg-no-repeat">
      
      <div class="fixed top-20 left-1/2 -translate-x-1/2 object-contain items-center mx-auto fade-in">
        <img src="/assets/images/umalogo.png" class="h-[5vh] md:h-[8vh]">
      </div>
      
      <form id="registrationForm" class="space-y-8 mx-12">
        <!-- Campo Nome -->
        <div class="input-container h-16 fade-in-delay-100">
          <input type="text" id="nome" name="nome" placeholder="Nome"
            class="custom-input placeholder-black capitalize border border-black border-b-4 border-0">
          <p id="nomeError" class="text-red-500 text-xs mt-1 hidden">Nome deve ter pelo menos 3 caracteres.</p>
        </div>
        
        <!-- Campo Telefone -->
        <div class="input-container h-16 fade-in-delay-200">
          <input type="text" id="telefone" name="telefone" placeholder="Telefone"
            class="custom-input placeholder-black border border-black border-b-4 border-0">
          <p id="telefoneError" class="text-red-500 text-xs mt-1 hidden">Telefone inválido.</p>
        </div>
        
        <!-- Checkbox de Interesse -->
        <div class="input-container h-16 fade-in-delay-300">
          <div class="flex items-center gap-2 px-2 h-full z-10 relative border border-black border-b-4 border-0">
            <label for="interesse" class="text-[2rem] text-black">Quero saber mais sobre a Phygital</label>
            <input id="interesse" name="interesse" type="checkbox"
              class="h-8 w-8 text-blue-600 border-black rounded">
          </div>
        </div>
      </form>
      
      <div class="flex justify-center mt-8 fade-in-delay-400">
        <img src="/assets/images/botao-iniciar.png" id="startButton" class="w-[80vw] md:w-[80vw] lg:w-[70vw] object-contain cursor-pointer">
      </div>
    </div>
    
    <!-- Tela do countdown -->
    <div id="countdown" class="hidden flex flex-col items-center justify-center h-screen">
      <p id="countdownTimer" class="text-5xl md:text-7xl font-medium pop-in">3</p>
    </div>
    
    <!-- Tela do jogo em andamento -->
    <div id="runningGame" class="hidden flex flex-col items-center justify-center h-screen w-screen bg-[url('/assets/images/bg.png')] bg-cover bg-center bg-no-repeat">
      <div class="fade-in">
        <h2 class="text-3xl md:text-4xl font-bold text-center mb-6">Jogando...</h2>
        <p id="score" class="text-2xl md:text-3xl font-medium mb-3 hidden fade-in-delay-100">Pontuação: 0</p>
        <p id="timer" class="text-2xl md:text-3xl font-medium hidden fade-in-delay-200">Tempo restante: 20s</p>
      </div>
    </div>
    
    <!-- Tela de pontuação final (exibida por 5 segundos) -->
    <div id="scoreScreen" class="hidden flex flex-col items-center justify-center h-screen">
      <div class="text-center">
        <h2 class="text-3xl md:text-4xl font-bold mb-4 pop-in">Fim do Jogo!</h2>
        <p id="finalScore" class="text-2xl md:text-3xl pop-in-delay-200">Pontuação final: 0</p>
      </div>
    </div>
    
    <!-- Tela de ranking (tela de descanso) -->
    <div id="rankingScreen" class="hidden bg-[url('/assets/images/bg.png')] bg-cover bg-center bg-no-repeat flex items-center justify-center h-screen w-screen overflow-hidden">
      <div class="fixed top-0 fade-in">
        <img src="/assets/images/ranking1.png" class="object-contain no-repeat" />
      </div>  
      <div class="fixed top-0 right-0 fade-in-delay-200">
        <img src="/assets/images/ranking2.png" class="object-contain no-repeat h-[20vh]" />
      </div>
      <div class="container mx-auto p-4 mt-12 fade-in-delay-300">
        <div class="overflow-x-auto ranking-table ">
          <table class="overflow-hidden">
            <thead>
              <tr class="">
                <th class="text-left px-6 py-3 text-sm md:text-base font-bold text-gray-100/10">Posição</th>
                <th class="text-left px-6 py-3 text-sm md:text-base font-bold text-gray-800/10">Nome</th>
                <th class="text-left px-6 py-3 text-sm md:text-base font-bold text-gray-800/10">Pontuação</th>
              </tr>
            </thead>
            <tbody id="rankingList"></tbody>
          </table>
        </div>
        <!-- Botão Começar na tela de ranking -->
        <div class="flex justify-center mt-8 pop-in-delay-500">
          <img src="/assets/images/botao-comecar.png" id="startGameButton" class="ranking-button w-[60vw] md:w-[40vw] lg:w-[30vw] object-contain cursor-pointer">
        </div>
      </div>
    </div>

</body>
<script src="assets/js/game.js"></script>
</html>
<?php
// views/admin_login.php
?>
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Login Administrativo</title>
  <script src="/assets/js/tailwind.js"></script>
</head>
<body class="bg-gray-100 flex items-center justify-center min-h-screen">
  <div class="w-full max-w-sm">
    <form action="/admin/login" method="POST" class="bg-white shadow-md rounded px-8 py-6">
      <h2 class="text-2xl font-bold mb-6 text-center">Painel Admin</h2>
      <?php if (!empty($_GET['error'])): ?>
        <div class="bg-red-100 border border-red-400 text-red-700 px-4 py-2 rounded mb-4">
          Senha incorreta. Tente novamente.
        </div>
      <?php endif; ?>

      <div class="mb-4">
        <label for="password" class="block text-gray-700 text-sm font-semibold mb-2">Senha</label>
        <input
          type="password"
          id="password"
          name="password"
          required
          class="shadow appearance-none border rounded w-full py-2 px-3 text-gray-700 leading-tight focus:outline-none focus:ring-2 focus:ring-blue-500"
          placeholder="Digite sua senha"
        >
      </div>

      <div class="flex items-center justify-between">
        <button
          type="submit"
          class="w-full bg-blue-600 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded focus:outline-none focus:ring-2 focus:ring-blue-500"
        >
          Entrar
        </button>
      </div>
    </form>
  </div>
</body>
</html>

<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title><?php echo $data['titulo']; ?></title>
  <script src="/assets/js/tailwind.js"></script>
</head>
<body class="bg-gray-100 flex items-center justify-center h-screen">
  <div class="bg-white p-8 rounded shadow-md w-1/2 max-w-md">
    <nav class="flex flex-col gap-4">
      <a href="/game" class="bg-blue-500 hover:bg-blue-600 text-white font-semibold py-2 px-4 rounded text-center">
        Game
      </a>
      <a href="/ranking" class="bg-green-500 hover:bg-green-600 text-white font-semibold py-2 px-4 rounded text-center">
        Ranking
      </a>
      <a href="/admin" class="bg-red-500 hover:bg-red-600 text-white font-semibold py-2 px-4 rounded text-center">
        Admin
      </a>
    </nav>
  </div>
</body>
</html>

<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title><?php echo $data['titulo']; ?></title>
  <meta name="referrer" content="no-referrer">
  <script src="/assets/js/tailwind.js"></script>
</head>
<body class="bg-[url('/assets/images/bg.png')] bg-cover bg-center bg-no-repeat flex items-center justify-center h-screen w-screen overflow-hidden">
<div class="fixed top-0">
   <img src="/assets/images/ranking1.png" class="object-contain no-repeat" />
</div>  
<div class="fixed top-0 right-0">
  <img src="/assets/images/ranking2.png" class="object-contain no-repeat h-[50vh]" />
</div>
<div class="container mx-auto p-4">
    <div class="overflow-x-auto">
      <table class="min-w-full divide-y ">
          <tbody id="rankingList" class="bg-[url('/assets/images/input.png')] object-cover h-[1px] bg-no-repeat"></tbody>
        </table>
    </div>
  </div>
</body>
<script src="/assets/js/ranking.js"></script>
</html>
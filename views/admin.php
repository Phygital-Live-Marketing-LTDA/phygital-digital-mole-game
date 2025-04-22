<?php
// views/backup_panel.php
if (session_status() !== PHP_SESSION_ACTIVE) {
    session_start();
}

$backupGenerated = !empty($_SESSION['backup_generated']);
?>
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Painel de Administração</title>
    <script src="https://cdn.tailwindcss.com"></script>
</head>
<body class="bg-gray-100">
<div class="container mx-auto p-6">
    <h1 class="text-2xl font-bold mb-4">Painel de Administração</h1>

    <?php if (isset($_GET['status'])): ?>
        <div class="bg-green-100 border border-green-400 text-green-700 px-4 py-3 rounded mb-4">
            <?= htmlspecialchars($_GET['status'], ENT_QUOTES) ?>
        </div>
    <?php endif; ?>

    <div class="bg-white shadow rounded p-6 space-y-4">
        <form action="/admin/generate-backup" method="POST">
            <button type="submit" class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded">
                Gerar Backup e Baixar
            </button>
        </form>

        <?php if ($backupGenerated): ?>
            <form id="clearForm" action="/admin/clear-database" method="POST">
                <input type="hidden" name="confirm_password" id="confirm_password" value="">
                <button id="clearBtn" type="submit" class="bg-red-500 hover:bg-red-700 text-white font-bold py-2 px-4 rounded">
                    Limpar Banco de Dados
                </button>
            </form>
        <?php else: ?>
            <button disabled class="bg-gray-400 text-white font-bold py-2 px-4 rounded cursor-not-allowed">
                Limpar Banco de Dados (gere backup antes)
            </button>
        <?php endif; ?>
    </div>
</div>

<script>
document.addEventListener('DOMContentLoaded', function() {
    const clearForm = document.getElementById('clearForm');
    if (!clearForm) return;

    clearForm.addEventListener('submit', function(e) {
        e.preventDefault();
        if (!confirm('Deseja realmente limpar o banco de dados?')) return;
        const pwd = prompt('Digite a senha de administrador para confirmar:');
        if (pwd === null) return;
        document.getElementById('confirm_password').value = pwd;
        clearForm.submit();
    });
});
</script>
</body>
</html>
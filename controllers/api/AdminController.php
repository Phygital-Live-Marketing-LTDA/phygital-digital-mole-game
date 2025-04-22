<?php
if (session_status() !== PHP_SESSION_ACTIVE) {
    session_start();
}

require_once 'services/api/ApiAdminService.php';
require_once './handleBackup.php';

class AdminController {
    public function generateBackup() {
        try {
            handleBackup(true);
            $_SESSION['backup_generated'] = true;
            header('Location: /admin?status=' . urlencode('Backup gerado com sucesso!'));
            exit;
        } catch (\Throwable $e) {
            http_response_code(500);
            header('Location: /admin?status=' . urlencode(htmlspecialchars($e->getMessage())));
        }
    }

    public function clearDatabase() {
        try {
            $svc = new ApiAdminService();
            $svc->clearDatabase();
            // Redireciona de volta ao painel com mensagem de status
            $_SESSION['backup_generated'] = false;
            header('Location: /admin?status=' . urlencode('Banco de dados limpo com sucesso'));
            exit;
        } catch (\Throwable $e) {
            header('Location: /admin?status=' . urlencode(htmlspecialchars($e->getMessage())));
        }
    }

    public function login() {
        $svc = new ApiAdminService();
        $svc->login();
    }
}
?>

<?php
// 1) carrega a lógica de sessão e login / backup
require __DIR__ . '/handleBackup.php';
require __DIR__ . '/auth.php';

// 2) define quais rotas precisam de autenticação
$protected = [
    '/admin',
    '/admin/generate-backup',
    '/admin/clear-database',
];

// 3) pega URI e método
$uri = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);
$method = $_SERVER['REQUEST_METHOD'];

// 4) se for rota protegida, checa sessão
if (in_array($uri, $protected, true)) {
    requireAdmin($method);
}

// 5) dispatch normal
if ($method === 'GET') {
    switch ($uri) {
        case '/':
            require_once 'controllers/HomeController.php';
            $controller = new HomeController();
            $controller->index();
            break;
        case '/game':
            require_once 'controllers/GameController.php';
            $controller = new GameController();
            $controller->index();
            break;
        case '/ranking':
            require_once 'controllers/RankingController.php';
            $controller = new RankingController();
            $controller->index();
            break;
        case '/ranking/data':
            require_once 'controllers/RankingController.php';
            $controller = new RankingController();
            $controller->data();
            break;
        case '/ranking/check-update/':
            require_once 'controllers/RankingController.php';
            $controller = new RankingController();
            $controller->checkUpdate();
        case '/admin':
            require_once 'controllers/AdminController.php';
            $controller = new AdminController();
            $controller->index();
            break;
        case '/admin/login':
            require_once 'controllers/AdminController.php';
            $controller = new AdminController();
            $controller->login();
            break;
    }
} else if ($method === 'POST') {
    switch ($uri) {
        case '/ranking':
            require_once 'controllers/api/RankingController.php';
            $controller = new RankingController();
            $controller->store();
            break;
        case '/leads':
            require_once 'controllers/api/LeadsController.php';
            $controller = new LeadsController();
            $controller->store();
            break;
        case '/admin/generate-backup':
            require_once 'controllers/api/AdminController.php';
            $controller = new AdminController();
            $controller->generateBackup();
            break;
        case '/admin/clear-database':
            require_once 'controllers/api/AdminController.php';
            $controller = new AdminController();
            $controller->clearDatabase();
            break;
        case '/admin/login':
            require_once 'controllers/api/AdminController.php';
            $controller = new AdminController();
            $controller->login();
            break;
        default:
            http_response_code(404);
            echo "Página não encontrada.";
            break;
    }
} else {
    http_response_code(405);
    echo "Método não permitido.";
}
?>
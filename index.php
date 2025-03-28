<?php
$uri = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);
$method = $_SERVER['REQUEST_METHOD'];

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
<?php
require_once 'services/GameService.php';

class GameController {
    public function index() {
        $service = new GameService();
        $data = $service->getData();
        require 'views/game.php';
    }
}
?>

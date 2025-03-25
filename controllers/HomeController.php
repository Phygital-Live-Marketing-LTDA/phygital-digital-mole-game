<?php
// controllers/HomeController.php
require_once 'services/HomeService.php';

class HomeController {
    public function index() {
        $service = new HomeService();
        // Obtem dados do serviço (ex: informações do banco de dados)
        $data = $service->getData();
        
        // Passa os dados para a view
        require 'views/home.php';
    }
}
?>

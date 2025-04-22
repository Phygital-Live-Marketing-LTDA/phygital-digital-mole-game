<?php
// controllers/AdminController.php
require_once 'services/AdminService.php';

class AdminController {
    public function index() {
        $service = new AdminService();
        // Obtem dados do serviço (ex: informações do banco de dados)
        $data = $service->getData();
        
        // Passa os dados para a view
        require 'views/admin.php';
    }

    public function login() {
        $service = new AdminService();
        // Obtem dados do serviço (ex: informações do banco de dados)
        $data = $service->login();
        
        // Passa os dados para a view
        require 'views/login.php';
    }
}
?>

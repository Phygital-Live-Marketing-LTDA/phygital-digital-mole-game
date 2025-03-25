<?php
require_once 'services/RankingService.php';

class RankingController {
    public function index() {
        $service = new RankingService();
        // Obtem dados do serviço (ex: informações do banco de dados)
        $data = $service->getData();
        
        // Passa os dados para a view
        require 'views/ranking.php';
    }

    public function data() {
        $service = new RankingService();
        // Obtem dados do serviço (ex: informações do banco de dados)
        $data = $service->getData();
        echo json_encode($data['ranking']);
    }

    public function checkUpdate() {
        header('Content-Type: application/json');
        
        $file = './storage/ranking/last-update.txt';
        $clientLastTimestamp = isset($_GET['lastTimestamp']) ? (int)$_GET['lastTimestamp'] : 0;
        
        if (file_exists($file)) {
            $serverLastTimestamp = (int) file_get_contents($file);
        } else {
            $serverLastTimestamp = 0;
        }
        
        if ($serverLastTimestamp > $clientLastTimestamp) {
            echo json_encode([
                "update" => true,
                "lastTimestamp" => $serverLastTimestamp
            ]);
        } else {
            echo json_encode([
                "update" => false,
                "lastTimestamp" => $serverLastTimestamp
            ]);
        }
    }
}
?>

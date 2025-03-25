<?php
require_once 'services/api/ApiRankingService.php';
require_once 'services/RankingService.php';

class RankingController {
    public function store() {
        $json = file_get_contents('php://input');
        $data = json_decode($json, true);

        if (empty($data)) {
            echo json_encode([
                'status' => 'error',
                'message' => 'Nenhum dado enviado.'
            ]);
            exit;
        }
        
        // Instancia o service e tenta salvar os dados
        $apiRankingService = new ApiRankingService();
        $result = $apiRankingService->saveData($data);
        
        if ($result) {
            $rankingService = new RankingService();
            $rankingServiceGetData = $rankingService->getData();
            file_put_contents("./storage/ranking/last-update.txt", time());

            echo json_encode([
                'status' => 'success',
                'message' => 'Dados salvos com sucesso!',
                'ranking' => json_encode($rankingServiceGetData['ranking'])
            ]);
        } else {
            echo json_encode([
                'status' => 'error',
                'message' => 'Falha ao salvar os dados.'
            ]);
        }
    }
}
?>

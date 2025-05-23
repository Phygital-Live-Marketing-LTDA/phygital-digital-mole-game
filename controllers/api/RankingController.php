<?php
require_once 'services/api/ApiRankingService.php';
require_once 'services/RankingService.php';

class RankingController {
    public function store() {
        $json = file_get_contents('php://input');
        $data = json_decode($json, true);

        // Verifica se os dados foram recebidos corretamente
        if (!$data || !isset($data['leads_id']) || !isset($data['score'])) {
            http_response_code(400);
            echo json_encode(['error' => 'Dados inválidos ou incompletos.']);
            exit;
        }

        // Sanitiza os dados
        $data['leads_id']      = filter_var($data['leads_id'], FILTER_SANITIZE_STRING);
        $data['score']         = filter_var($data['score'], FILTER_SANITIZE_STRING);
        
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

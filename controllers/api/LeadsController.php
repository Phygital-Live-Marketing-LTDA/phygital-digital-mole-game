<?php
require_once 'services/api/ApiLeadsService.php';

class LeadsController {
    public function store() {
        $json = file_get_contents('php://input');
        $data = json_decode($json, true);

        // Verifica se os dados foram recebidos corretamente
        if (!$data || !isset($data['nome']) || !isset($data['telefone']) || !isset($data['interesse'])) {
            http_response_code(400);
            echo json_encode(['error' => 'Dados inválidos ou incompletos.']);
            exit;
        }

        // Sanitiza os dados
        $data['nome']      = filter_var($data['nome'], FILTER_SANITIZE_STRING);
        $data['telefone']  = filter_var($data['telefone'], FILTER_SANITIZE_STRING);
        // Converte o checkbox para 1 (true) ou 0 (false)
        $data['interesse'] = $data['interesse'] ? 1 : 0;

        // Instancia o service e tenta salvar os dados
        $apiLeadsService = new ApiLeadsService();
        $result = $apiLeadsService->saveData($data);
        
        if ($result) {
            echo json_encode([
                'status' => 'success',
                'message' => 'Dados salvos com sucesso!',
                'id' => $result
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

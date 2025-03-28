<?php
require_once './config/Database.php';

class RankingService {
    public function getData() {
        // Obtém a conexão PDO através da classe Database
        $db = Database::getInstance()->getConnection();
        
        // Exemplo de consulta ao banco de dados
        $stmt = $db->prepare("
            SELECT r.id, l.nome AS name, r.score 
            FROM ranking r 
            LEFT JOIN leads l ON r.leads_id = l.id 
            ORDER BY r.score DESC 
            LIMIT 10
        ");
        $stmt->execute();
        $ranking_data = $stmt->fetchAll(PDO::FETCH_ASSOC);
        
        $data = [
            'titulo' => 'Ranking Agility Game',
            'ranking' => $ranking_data
        ];
        return $data;
    }
}
?>

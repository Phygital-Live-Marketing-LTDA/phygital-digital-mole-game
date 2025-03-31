<?php
require_once(__DIR__ . '/../config/database.php');

class RankingService {
    public function getData() {
        // Obtém a conexão PDO através da classe Database
        $db = Database::getInstance()->getConnection();
        
        // Exemplo de consulta ao banco de dados
        $stmt = $db->prepare("
            SELECT l.id AS id, l.nome AS name, MAX(r.score) AS score
            FROM leads l
            LEFT JOIN ranking r ON l.id = r.leads_id
            GROUP BY l.id, l.nome
            ORDER BY score DESC
            LIMIT 5
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

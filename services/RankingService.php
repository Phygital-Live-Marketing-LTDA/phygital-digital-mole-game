<?php
require_once './config/Database.php';

class RankingService {
    public function getData() {
        // Obtém a conexão PDO através da classe Database
        $db = Database::getInstance()->getConnection();
        
        // Exemplo de consulta ao banco de dados
        $stmt = $db->prepare("SELECT id, name, score FROM ranking ORDER BY score DESC LIMIT 10");
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

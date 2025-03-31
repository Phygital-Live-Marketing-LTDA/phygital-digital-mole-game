<?php
require_once(__DIR__ . '/../config/database.php');

class HomeService {
    public function getData() {
        // Obtém a conexão PDO através da classe Database
        // $db = Database::getInstance()->getConnection();
        
        // // Exemplo de consulta ao banco de dados
        // $stmt = $db->prepare("SELECT titulo, mensagem FROM home_info LIMIT 1");
        // $stmt->execute();
        // $data = $stmt->fetch(PDO::FETCH_ASSOC);
        
        // Caso não encontre dados, retorna um conjunto padrão
        if (!isset($data)) {
            $data = [
                'titulo' => 'Página Inicial'
            ];
        }
        return $data;
    }
}
?>

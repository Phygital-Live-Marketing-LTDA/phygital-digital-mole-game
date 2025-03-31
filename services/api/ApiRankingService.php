<?php
require_once(__DIR__ . '/../config/database.php');

class ApiRankingService {
    private $db;

    public function __construct() {
        // Obtém a conexão com o banco de dados
        $this->db = Database::getInstance()->getConnection();
    }

    /**
     * Salva os dados recebidos no banco.
     *
     * @param array $data Array associativo com os dados a serem salvos.
     * @return mixed O ID inserido ou false em caso de erro.
     */
    public function saveData(array $data) {
        // Extrai as colunas e cria os placeholders para a query
        $columns = array_keys($data);
        $placeholders = array_map(function($col) {
            return ":{$col}";
        }, $columns);

        // Monta a query de INSERT
        $sql = "INSERT INTO ranking (" . implode(", ", $columns) . ") VALUES (" . implode(", ", $placeholders) . ")";
        $stmt = $this->db->prepare($sql);

        // Associa os valores aos respectivos placeholders
        foreach ($data as $col => $value) {
            $stmt->bindValue(":{$col}", $value);
        }

        // Executa e retorna o ID inserido ou false em caso de erro
        if ($stmt->execute()) {
            return true;
        }
        return false;
    }
}
<?php
require_once './config/Database.php';

class ApiLeadsService {
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
        $columns      = array_keys($data);
        $placeholders = array_map(function($col) {
            return ":{$col}";
        }, $columns);

        $getData = $this->getData($data);
        if ($getData) {
            return $getData['id'];
        }

        // Monta a query de INSERT
        $sql  = "INSERT INTO leads (" . implode(", ", $columns) . ") VALUES (" . implode(", ", $placeholders) . ")";
        $stmt = $this->db->prepare($sql);

        // Associa os valores aos respectivos placeholders
        foreach ($data as $col => $value) {
            $stmt->bindValue(":{$col}", $value);
        }

        // Executa e retorna o ID inserido ou false em caso de erro
        if ($stmt->execute()) {
            return $this->db->lastInsertId();
        }
        return false;
    }

    /**
     * Busca os dados do usuário.
     *
     * @param array $data Array associativo com os dados a serem salvos.
     * @return array|false Array associativo com os dados do usuário ou false em caso de erro.
     */
    public function getData(array $data) {
        // Extrai as colunas e cria os placeholders para a query
        $columns      = array_keys($data);
        $placeholders = array_map(function($col) {
            return "{$col} = :{$col}";
        }, $columns);

        // Monta a query de SELECT
        $sql  = "SELECT id, " . implode(", ", $columns) . " FROM leads WHERE " . implode(" AND ", $placeholders);
        $stmt = $this->db->prepare($sql);

        // Associa os valores aos respectivos placeholders
        foreach ($data as $col => $value) {
            $stmt->bindValue(":{$col}", $value);
        }

        $stmt->execute();
        $result = $stmt->fetch(PDO::FETCH_ASSOC);
        if ($result) {
            return $result;
        }
        return false;
    }
}
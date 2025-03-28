<?php
class CreateLeadsTable {
    /**
     * Método responsável por aplicar a migration para a tabela de leads.
     *
     * @param PDO $db Conexão com o banco de dados.
     */
    public function up($db) {
        $sql = "CREATE TABLE IF NOT EXISTS leads (
            id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
            nome VARCHAR(255) NOT NULL,
            telefone VARCHAR(50) NOT NULL,
            interesse TINYINT(1) NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8;";
        $db->exec($sql);
    }
}
?>
<?php
class CreaterRankingTable {
    /**
     * Método responsável por aplicar a migration.
     *
     * @param PDO $db Conexão com o banco de dados.
     */
    public function up($db) {
        $sql = "CREATE TABLE IF NOT EXISTS ranking (
            id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(255) NOT NULL,
            score INT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8;";
        $db->exec($sql);
    }
}

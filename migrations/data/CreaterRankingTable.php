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
            leads_id INT UNSIGNED DEFAULT NULL,
            score INT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (leads_id) REFERENCES leads(id) ON DELETE SET NULL ON UPDATE CASCADE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8;";
        $db->exec($sql);        
    }
}

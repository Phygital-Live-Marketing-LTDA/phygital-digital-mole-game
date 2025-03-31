<?php
require_once(__DIR__ . '/../config/database.php');

class MigrationService {
    private $db;
    private $migrationsDir;

    public function __construct() {
        // Obtém a conexão com o banco de dados
        $this->db = Database::getInstance()->getConnection();
        // Diretório onde estão os arquivos de migration
        $this->migrationsDir = './migrations/data';
        // Garante que a tabela de migrations exista
        $this->createMigrationsTable();
    }

    /**
     * Cria a tabela de migrations se não existir.
     */
    private function createMigrationsTable() {
        $sql = "CREATE TABLE IF NOT EXISTS migrations (
            id INT AUTO_INCREMENT PRIMARY KEY,
            migration VARCHAR(255) NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8;";
        $this->db->exec($sql);
    }

    /**
     * Executa as migrations que ainda não foram aplicadas.
     */
    public function migrate() {
        // Recupera a lista de migrations já aplicadas
        $appliedMigrations = $this->getAppliedMigrations();

        // Lê os arquivos de migration do diretório
        $files = scandir($this->migrationsDir);
        $newMigrations = [];

        foreach ($files as $file) {
            // Ignora os diretórios . e ..
            if ($file === '.' || $file === '..') {
                continue;
            }
            // Se a migration já foi aplicada, pula para a próxima
            if (in_array($file, $appliedMigrations)) {
                continue;
            }
            // Carrega o arquivo de migration
            require_once $this->migrationsDir . '/' . $file;

            // Define o nome da classe com base no nome do arquivo (sem extensão)
            $className = pathinfo($file, PATHINFO_FILENAME);

            if (class_exists($className)) {
                // Instancia e executa a migration
                $migrationInstance = new $className();
                echo "Aplicando migration: $file\n";
                $migrationInstance->up($this->db);
                echo "Migration aplicada: $file\n";
                $newMigrations[] = $file;
            }
        }

        // Registra as migrations aplicadas no banco
        if (!empty($newMigrations)) {
            $this->saveMigrations($newMigrations);
        } else {
            echo "Nenhuma migration para aplicar.\n";
        }
    }

    /**
     * Recupera as migrations já aplicadas.
     *
     * @return array
     */
    private function getAppliedMigrations() {
        $stmt = $this->db->query("SELECT migration FROM migrations");
        return $stmt->fetchAll(PDO::FETCH_COLUMN);
    }

    /**
     * Salva no banco as migrations que foram executadas.
     *
     * @param array $migrations
     */
    private function saveMigrations(array $migrations) {
        $sql = "INSERT INTO migrations (migration) VALUES (:migration)";
        $stmt = $this->db->prepare($sql);

        foreach ($migrations as $migration) {
            $stmt->execute(['migration' => $migration]);
        }
    }
}

// Exemplo de uso do MigrationService
$migrationService = new MigrationService();
$migrationService->migrate();

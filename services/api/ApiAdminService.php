<?php
if (session_status() !== PHP_SESSION_ACTIVE) {
    session_start();
}

include './config/getEnv.php';
require_once './config/Database.php';

class ApiAdminService {
    public function generateBackup() {
        $db = Database::getInstance()->getConnection();
        $filename = 'backup_' . date('Ymd') . '.sql';
        $tmpPath  = sys_get_temp_dir() . DIRECTORY_SEPARATOR . $filename;
        $fh = fopen($tmpPath, 'w');
        if (!$fh) {
            throw new \RuntimeException("Não foi possível criar o arquivo de dump em {$tmpPath}");
        }
    
        // Cabeçalho
        fwrite($fh, "-- Exportação gerada em " . date('Y-m-d H:i:s') . "\n");
        fwrite($fh, "SET FOREIGN_KEY_CHECKS=0;\n\n");
    
        // Lista de tabelas
        $tables = $db
            ->query("SHOW TABLES")
            ->fetchAll(\PDO::FETCH_COLUMN);
    
        foreach ($tables as $table) {
            // DROP + CREATE
            fwrite($fh, "-- --------------------------------------------------------\n");
            fwrite($fh, "DROP TABLE IF EXISTS `{$table}`;\n");
    
            $row = $db
                ->query("SHOW CREATE TABLE `{$table}`")
                ->fetch(\PDO::FETCH_ASSOC);
            fwrite($fh, $row['Create Table'] . ";\n\n");
    
            // Prepara SELECT
            $stmt = $db->query("SELECT * FROM `{$table}`");
            $columnCount = $stmt->columnCount();
    
            if ($columnCount > 0) {
                // Pega nomes de colunas via metadata
                $columns = [];
                for ($i = 0; $i < $columnCount; $i++) {
                    $meta = $stmt->getColumnMeta($i);
                    $columns[] = $meta['name'];
                }
                $colsList = '`' . implode('`,`', $columns) . '`';
    
                // Re-executa a query para iterar todas as linhas
                $stmt = $db->query("SELECT * FROM `{$table}`");
                while ($row = $stmt->fetch(\PDO::FETCH_NUM)) {
                    $values = array_map(function($val) use ($db) {
                        return $val === null
                            ? 'NULL'
                            : $db->quote($val);
                    }, $row);
    
                    fwrite(
                        $fh,
                        "INSERT INTO `{$table}` ({$colsList}) VALUES (" . implode(',', $values) . ");\n"
                    );
                }
                fwrite($fh, "\n");
            }
        }
    
        // Rodapé
        fwrite($fh, "SET FOREIGN_KEY_CHECKS=1;\n");
        fclose($fh);
    
        return $tmpPath;
    }

    public function clearDatabase() {
        if (!hash_equals($_ENV['ADMIN_PASSWORD'], $_POST['confirm_password'] ?? '')) {
            throw new \RuntimeException('Senha incorreta.');
        }

        $db = Database::getInstance()->getConnection();
        $db->exec("SET FOREIGN_KEY_CHECKS = 0");
        $tables = $db
            ->query("SHOW TABLES")
            ->fetchAll(\PDO::FETCH_COLUMN);

        foreach ($tables as $table) {
            $db->exec("TRUNCATE TABLE `{$table}`");
        }

        $db->exec("SET FOREIGN_KEY_CHECKS = 1");
    }

    public function login() {
        $pw = $_POST['password'] ?? '';
        if (hash_equals($_ENV['ADMIN_PASSWORD'], $pw)) {
            $_SESSION['is_admin'] = true;
            header('Location: /admin');
        } else {
            // volta com parâmetro de erro
            header('Location: /admin/login?error=1');
        }
    }
}
?>

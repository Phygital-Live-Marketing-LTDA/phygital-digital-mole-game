<?php
include './config/getEnv.php';

class Database {
    private static $instance = null;
    private $connection;

    // O construtor é privado para evitar instanciar a classe diretamente
    private function __construct() {
        $host = $_ENV['DB_HOST'];
        $user = $_ENV['DB_USER'];
        $password = $_ENV['DB_PASS'];
        $dbname = $_ENV['DB_NAME'];

        $dsn = "mysql:host={$host};dbname={$dbname};charset=utf8";
        
        try {
            $this->connection = new PDO($dsn, $user, $password);
            $this->connection->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        } catch (PDOException $e) {
            echo json_encode('connection-error');
            exit();
        }
    }
    
    // Retorna a instância única da classe
    public static function getInstance() {
        if (self::$instance === null) {
            self::$instance = new Database();
        }
        return self::$instance;
    }
    
    // Retorna a conexão PDO
    public function getConnection() {
        return $this->connection;
    }
}
?>

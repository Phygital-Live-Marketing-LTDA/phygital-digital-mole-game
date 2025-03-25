<?php
class Database {
    private static $instance = null;
    private $connection;

    // O construtor é privado para evitar instanciar a classe diretamente
    private function __construct() {
        $host = 'localhost';
        $dbname = 'digital_mole_game';
        $user = 'root';
        $password = '';
        $dsn = "mysql:host={$host};dbname={$dbname};charset=utf8";
        
        try {
            $this->connection = new PDO($dsn, $user, $password);
            $this->connection->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        } catch (PDOException $e) {
            die('Conexão falhou: ' . $e->getMessage());
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

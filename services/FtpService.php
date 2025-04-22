<?php
// services/FtpService.php

class FtpService
{
    private $conn;
    private array $cfg;

    public function __construct()
    {
        $this->cfg = require __DIR__ . '/../config/ftp.php';
    }

    public function connect(): void
    {
        $this->conn = ftp_connect($this->cfg['host'], $this->cfg['port'], 10);
        if (!$this->conn) {
            throw new RuntimeException("Não foi possível conectar ao FTP {$this->cfg['host']}");
        }

        if (!ftp_login($this->conn, $this->cfg['username'], $this->cfg['password'])) {
            throw new RuntimeException("Falha no login FTP como {$this->cfg['username']}");
        }

        if ($this->cfg['passive']) {
            ftp_pasv($this->conn, true);
        }

        // já posiciona na pasta remota
        if (!ftp_chdir($this->conn, $this->cfg['remote_dir'])) {
            throw new RuntimeException("Não foi possível mudar para o diretório remoto {$this->cfg['remote_dir']}");
        }
    }

    /**
     * Retorna a lista de arquivos no diretório remoto
     *
     * @return string[]
     */
    public function listFiles(): array
    {
        if (!$this->conn) {
            $this->connect();
        }
        $files = ftp_nlist($this->conn, '.');
        if ($files === false) {
            throw new RuntimeException("Falha ao listar arquivos em {$this->cfg['remote_dir']}");
        }
        return $files;
    }

    /**
     * Verifica se já existe um arquivo cujo nome começa com o prefixo dado
     *
     * @param string $datePrefix ex: 'backup_20250422'
     */
    public function remoteFileExistsForDate(string $datePrefix): bool
    {
        $files = $this->listFiles();
        foreach ($files as $file) {
            if (strpos($file, $datePrefix) === 0) {
                return true;
            }
        }
        return false;
    }

    /**
     * Envia um arquivo local para o servidor FTP.
     */
    public function upload(string $localPath, string $remoteName): void
    {
        if (!file_exists($localPath)) {
            throw new RuntimeException("Arquivo local não encontrado: {$localPath}");
        }
        if (!$this->conn) {
            $this->connect();
        }
        if (!ftp_put($this->conn, $remoteName, $localPath, FTP_BINARY)) {
            throw new RuntimeException("Falha ao enviar {$localPath} como {$remoteName}");
        }
    }

    public function close(): void
    {
        if ($this->conn) {
            ftp_close($this->conn);
        }
    }
}

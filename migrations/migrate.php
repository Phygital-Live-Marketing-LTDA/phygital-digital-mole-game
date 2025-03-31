<?php
// Certifique-se de que o arquivo Database.php está no mesmo diretório ou ajustado o caminho
require_once(__DIR__ . '/../config/database.php');
require_once 'MigrationService.php';

$migrationService = new MigrationService();
$migrationService->migrate();

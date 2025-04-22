<?php
if (session_status() !== PHP_SESSION_ACTIVE) {
    session_start();
}

require __DIR__ . '/services/api/ApiAdminService.php';
require __DIR__ . '/services/FtpService.php';

function localStore($backupFile, $prefix = null)
{
    // Fallback local, mas só se ainda não tiver
    $localDir = __DIR__ . '/backups';
    if (!is_dir($localDir)) {
        mkdir($localDir, 0755, true);
    }

    if (!$prefix) {
        $prefix = 'backup_' . date('Ymd-His') . '.sql';
    }

    $existing = glob($localDir . DIRECTORY_SEPARATOR . $prefix . '*');

    if (!$existing) {
        $dest = $localDir . DIRECTORY_SEPARATOR . $prefix;
        if (!rename($backupFile, $dest)) {
            copy($backupFile, $dest);
            unlink($backupFile);
        }
    }
}

function handleBackup($complete_date = false)
{
    $svc = new ApiAdminService();
    $backupFile = $svc->generateBackup();

    $ftp = new FtpService();

    if($complete_date) {
        $todayPrefix = 'backup_' . date('Ymd-His') . '.sql';
    } else {
        $todayPrefix = 'backup_' . date('Ymd') . '.sql';
    }

    try {
        if (!$ftp->remoteFileExistsForDate($todayPrefix)) {
            $ftp->upload($backupFile, $todayPrefix);
        }
        $ftp->close();
        localStore($backupFile, $todayPrefix);
        $_SESSION['backup_generated'] = true;
    } catch (\Throwable $e) {
        localStore($backupFile, $todayPrefix);
    }
}

if(!isset($_SESSION['backup_generated'])) {
    handleBackup();
}
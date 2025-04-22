<?php
include './config/getEnv.php';
// config/ftp.php
return [
    'host'       => $_ENV['FTP_HOST'],
    'port'       => 21,
    'username'   => $_ENV['FTP_USER'],
    'password'   => $_ENV['FTP_PASS'],
    'passive'    => true,
    'remote_dir' => $_ENV['FTP_REMOTE_DIR']
];

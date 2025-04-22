<?php
// auth.php
if (session_status() !== PHP_SESSION_ACTIVE) {
    session_start();
}

/**
 * Se não estiver autenticado como admin, redireciona para o login.
 */
function requireAdmin($method)
{
    if (empty($_SESSION['is_admin'])) {
        if($method === 'GET') {
            header('Location: /admin/login');
            exit;
        } else if($method === 'POST') {
            http_response_code(405);
            exit('route_protected');
        }
    }
}
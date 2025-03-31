<?php
// Verifica se o arquivo .env existe
if (file_exists(__DIR__ . '/.env')) {
    // Lê o conteúdo do arquivo ignorando linhas vazias
    $lines = file(__DIR__ . '/.env', FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);

    foreach ($lines as $line) {
        // Ignora linhas que comecem com '#' (comentários)
        if (strpos(trim($line), '#') === 0) {
            continue;
        }

        // Separa a chave e o valor (limitando a 2 partes, pois o valor pode conter '=')
        list($name, $value) = explode('=', $line, 2);

        $name = trim($name);
        $value = trim($value);

        // Remove aspas, se estiverem presentes no valor
        if ((substr($value, 0, 1) === '"' && substr($value, -1) === '"') ||
            (substr($value, 0, 1) === "'" && substr($value, -1) === "'")
        ) {
            $value = substr($value, 1, -1);
        }

        // Define a variável de ambiente
        putenv("$name=$value");
        $_ENV[$name] = $value;
        $_SERVER[$name] = $value;
    }
}
<?php

// ==== Sicherheits-API-Key ====
$API_KEY = "Geheim";

// ==== API-Key prüfen ====
if (!isset($_GET['key']) || $_GET['key'] !== $API_KEY) {
    http_response_code(403);
    echo "Forbidden";
    exit;
}

// ==== Upload-Verzeichnis ====
$uploadDir = __DIR__ . "/uploads/";
//if (!is_dir($uploadDir)) {
//    mkdir($uploadDir, 0775, true);
//}

// ==== Dateiname ====
$filename = "aktuelle_Daten.txt";
$filepath = $uploadDir . $filename;

// ==== Rohdaten lesen ====
$data = file_get_contents("php://input");

// ==== Datei speichern ====
file_put_contents($filepath, $data);

// ==== Antwort ====
echo "OK: $filename gespeichert\n";
?>

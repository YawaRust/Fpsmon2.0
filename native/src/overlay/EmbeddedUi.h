#pragma once
// В CI этот файл перезаписывается реальным single-file build из dist/index.html.
// Локально служит заглушкой, чтобы проект собирался даже без npm build.
static const char* kEmbeddedUiHtml =
R"HTML(
<!doctype html>
<html lang="ru">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>YawaFPS HUD Config</title>
  <style>
    html,body{margin:0;height:100%;background:#05070b;color:#e8eef5;font:12px Consolas,monospace}
    .wrap{display:flex;align-items:center;justify-content:center;height:100%}
    .card{width:338px;border:1px solid rgba(0,255,136,.25);border-radius:12px;background:rgba(5,7,11,.95);box-shadow:0 24px 70px rgba(0,0,0,.7);padding:18px}
    .muted{color:#7b8499}.accent{color:#00ff88}.btn{margin-top:16px;background:#00ff88;color:#05100b;border:none;border-radius:8px;padding:10px 12px;width:100%;font:700 12px Consolas,monospace;cursor:pointer}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <div class="accent">YAWAFPS HUD</div>
      <p class="muted">React UI не был встроен в этот билд.</p>
      <p>Для CI/Release workflow этот файл автоматически заменяется на точную single-file сборку демо.</p>
      <button class="btn" onclick="chrome.webview?.postMessage({type:'close'})">Закрыть</button>
    </div>
  </div>
</body>
</html>
)HTML";

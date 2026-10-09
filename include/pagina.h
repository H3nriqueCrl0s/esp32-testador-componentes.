#pragma once

const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Testador ESP32</title>
<style>
  :root { --bg:#0f172a; --card:#1e293b; --txt:#e2e8f0; --sub:#94a3b8;
          --ok:#22c55e; --erro:#ef4444; --azul:#3b82f6; --amarelo:#f59e0b; }
  * { box-sizing:border-box; }
  body { margin:0; font-family:system-ui,Arial,sans-serif; background:var(--bg); color:var(--txt); }
  header { padding:16px; text-align:center; }
  header h1 { margin:0; font-size:1.4rem; }
  header small { color:var(--sub); }
  .grade { display:grid; gap:16px; padding:16px; max-width:1000px; margin:auto;
           grid-template-columns:repeat(auto-fit,minmax(260px,1fr)); }
  .card { background:var(--card); border-radius:12px; padding:16px; }
  .card h2 { margin:0 0 12px; font-size:1.05rem; display:flex; justify-content:space-between; align-items:center; }
  .valor { font-size:2.2rem; font-weight:bold; }
  .unid { font-size:1rem; color:var(--sub); }
  .linha { display:flex; justify-content:space-between; padding:4px 0; border-bottom:1px solid #334155; }
  .linha:last-child { border-bottom:none; }
  .tag { font-size:.75rem; padding:2px 8px; border-radius:99px; background:#334155; color:var(--sub); }
  .tag.ok { background:var(--ok); color:#052e16; }
  .tag.erro { background:var(--erro); color:#fff; }
  .tag.ativo { background:var(--amarelo); color:#422006; }
  button { border:none; border-radius:8px; padding:12px; font-size:1rem; font-weight:bold;
           cursor:pointer; color:#fff; background:var(--azul); width:100%; }
  button:active { transform:scale(.97); }
  .botoes { display:grid; grid-template-columns:1fr 1fr; gap:8px; margin-top:8px; }
  .parar { background:var(--erro); grid-column:1 / -1; }
  .ligado { background:var(--amarelo); color:#422006; }
  input[type=range] { width:100%; margin-top:8px; }
</style>
</head>
<body>
<header>
  <h1>Testador de Componentes</h1>
  <small id="conexao">conectando...</small>
</header>

<div class="grade">
  <div class="card">
    <h2>Ultrassônico HC-SR04 <span class="tag" id="tagUltra">--</span></h2>
    <div><span class="valor" id="distancia">--</span> <span class="unid">cm</span></div>
  </div>

  <div class="card">
    <h2>Temperatura DS18B20 <span class="tag" id="tagTemp">--</span></h2>
    <div><span class="valor" id="temperatura">--</span> <span class="unid">°C</span></div>
  </div>

  <div class="card">
    <h2>Corrente INA219 <span class="tag" id="tagIna">--</span></h2>
    <div class="linha"><span>Tensão</span><b><span id="tensao">--</span> V</b></div>
    <div class="linha"><span>Corrente</span><b><span id="corrente">--</span> mA</b></div>
    <div class="linha"><span>Potência</span><b><span id="potencia">--</span> mW</b></div>
  </div>

  <div class="card">
    <h2>Buzzer <span class="tag" id="tagBuzzer">--</span></h2>
    <button id="btnBuzzer" onclick="cmd('/buzzer')">Ligar buzzer</button>
  </div>

  <div class="card">
    <h2>Motor (Ponte H) <span class="tag" id="tagMotor">--</span></h2>
    <div class="botoes">
      <button onclick="cmd('/motor?sentido=1')">&#8635; Horário</button>
      <button onclick="cmd('/motor?sentido=-1')">&#8634; Anti-horário</button>
      <button class="parar" onclick="cmd('/motor?sentido=0')">&#9632; Parar</button>
    </div>
    <div class="linha" style="margin-top:12px"><span>Velocidade</span><b><span id="velTxt">--</span>%</b></div>
    <input type="range" id="vel" min="0" max="100" step="5"
           oninput="velTxt.textContent=this.value"
           onchange="cmd('/velocidade?valor='+this.value)">
  </div>
</div>

<script>
const $ = id => document.getElementById(id);
let arrastando = false;
$('vel').addEventListener('pointerdown', () => arrastando = true);
$('vel').addEventListener('pointerup',   () => arrastando = false);

function tag(el, texto, classe) { el.textContent = texto; el.className = 'tag ' + (classe || ''); }

function mostrar(s) {
  $('conexao').textContent = 'online';

  if (s.distancia === null) { $('distancia').textContent = '--'; tag($('tagUltra'), 'sem eco', 'erro'); }
  else { $('distancia').textContent = s.distancia.toFixed(1); tag($('tagUltra'), 'OK', 'ok'); }

  if (!s.tempOk || s.temperatura === null) { $('temperatura').textContent = '--'; tag($('tagTemp'), 'não encontrado', 'erro'); }
  else { $('temperatura').textContent = s.temperatura.toFixed(1); tag($('tagTemp'), 'OK', 'ok'); }

  if (!s.inaOk) {
    ['tensao','corrente','potencia'].forEach(k => $(k).textContent = '--');
    tag($('tagIna'), 'não encontrado', 'erro');
  } else {
    $('tensao').textContent = s.tensao.toFixed(2);
    $('corrente').textContent = s.corrente.toFixed(1);
    $('potencia').textContent = s.potencia.toFixed(0);
    tag($('tagIna'), 'OK', 'ok');
  }

  tag($('tagBuzzer'), s.buzzer ? 'LIGADO' : 'desligado', s.buzzer ? 'ativo' : '');
  $('btnBuzzer').textContent = s.buzzer ? 'Desligar buzzer' : 'Ligar buzzer';
  $('btnBuzzer').className = s.buzzer ? 'ligado' : '';

  const m = { '1':'girando ↻', '-1':'girando ↺', '0':'parado' }[s.motorSentido];
  tag($('tagMotor'), m, s.motorSentido ? 'ativo' : '');
  if (!arrastando) { $('vel').value = s.motorVelocidade; $('velTxt').textContent = s.motorVelocidade; }
}

async function cmd(url) {
  try { mostrar(await (await fetch(url)).json()); }
  catch (e) { $('conexao').textContent = 'sem conexão com o ESP32'; }
}

setInterval(() => cmd('/status'), 1000);
cmd('/status');
</script>
</body>
</html>
)rawliteral";

#pragma once

const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#ffffff">
<title>Testador WBM</title>
<style>
  /* Paleta tirada do logo: verde #00FC00, grafite #3C3C3C, branco */
  :root { --bg:#1b1b1b; --card:#262626; --borda:#3c3c3c; --txt:#f2f2f2; --sub:#9a9a9a;
          --verde:#00fc00; --verde-fundo:rgba(0,252,0,.12); --grafite:#3c3c3c;
          --erro:#ff5c5c; --erro-fundo:rgba(255,92,92,.14);
          --aviso:#ffb020; --aviso-fundo:rgba(255,176,32,.14); }
  * { box-sizing:border-box; }
  body { margin:0; font-family:system-ui,"Segoe UI",Arial,sans-serif; background:var(--bg); color:var(--txt); }

  header { background:#fff; border-bottom:3px solid var(--verde); }
  .topo { max-width:1100px; margin:auto; padding:12px 16px; display:flex; align-items:center;
          justify-content:space-between; gap:12px; flex-wrap:wrap; }
  .marca { display:flex; align-items:center; gap:14px; }
  .marca img { height:52px; display:block; }
  .marca .titulo { color:var(--grafite); border-left:2px solid #e2e2e2; padding-left:14px; }
  .marca .titulo b { display:block; font-size:1.1rem; }
  .marca .titulo small { color:#7a7a7a; font-size:.8rem; }
  .conexao { display:flex; align-items:center; gap:8px; font-size:.85rem; font-weight:600;
             color:var(--grafite); background:#f1f1f1; padding:6px 12px; border-radius:99px; }
  .ponto { width:10px; height:10px; border-radius:50%; background:#b5b5b5; }
  .conexao.on .ponto { background:var(--verde); box-shadow:0 0 0 3px rgba(0,252,0,.25); }
  .conexao.off .ponto { background:var(--erro); }

  main { max-width:1100px; margin:auto; padding:8px 16px 24px; }
  h3 { margin:22px 0 10px; font-size:.8rem; letter-spacing:.12em; text-transform:uppercase;
       color:var(--sub); display:flex; align-items:center; gap:10px; }
  h3::before { content:""; width:4px; height:14px; background:var(--verde); border-radius:2px; }
  .grade { display:grid; gap:14px; grid-template-columns:repeat(auto-fill,minmax(230px,1fr)); }

  .card { background:var(--card); border:1px solid var(--borda); border-radius:14px; padding:16px; }
  .card h2 { margin:0 0 10px; font-size:.95rem; font-weight:600; display:flex;
             justify-content:space-between; align-items:center; gap:8px; }
  .valor { font-size:2.3rem; font-weight:700; font-variant-numeric:tabular-nums; }
  .unid { font-size:1rem; color:var(--sub); }
  .pino { display:block; margin-top:6px; color:var(--sub); font-size:.78rem; }
  .linha { display:flex; justify-content:space-between; padding:7px 0; border-bottom:1px solid var(--borda); }
  .linha:last-child { border-bottom:none; }
  .linha b { font-variant-numeric:tabular-nums; }

  .tag { font-size:.72rem; font-weight:600; padding:3px 9px; border-radius:99px; white-space:nowrap;
         background:var(--borda); color:var(--sub); }
  .tag.ok    { background:var(--verde-fundo); color:var(--verde); }
  .tag.erro  { background:var(--erro-fundo);  color:var(--erro); }
  .tag.ativo { background:var(--aviso-fundo); color:var(--aviso); }

  button { border:none; border-radius:10px; padding:12px; font-size:.95rem; font-weight:700;
           cursor:pointer; width:100%; background:var(--verde); color:#0d1f0d; }
  button:hover { filter:brightness(1.08); }
  button:active { transform:scale(.97); }
  button.secundario { background:transparent; color:var(--txt); border:1px solid var(--borda); }
  button.parar { background:var(--erro); color:#fff; grid-column:1 / -1; }
  button.ligado { background:var(--aviso); color:#2b1a00; }
  .botoes { display:grid; grid-template-columns:1fr 1fr; gap:8px; margin-top:4px; }
  input[type=range] { width:100%; margin-top:8px; accent-color:var(--verde); }

  footer { text-align:center; color:var(--sub); font-size:.78rem; padding:8px 16px 24px; }
  footer b { color:var(--verde); }
</style>
</head>
<body>
<header>
  <div class="topo">
    <div class="marca">
      <img src="/logo.png" alt="WBM Technology">
      <div class="titulo"><b>Testador de Componentes</b><small>ESP32 · 192.168.4.1</small></div>
    </div>
    <div class="conexao" id="conexao"><span class="ponto"></span><span id="conexaoTxt">conectando...</span></div>
  </div>
</header>

<main>
  <h3>Distância</h3>
  <div class="grade" id="gradeUltra"></div>

  <h3>Temperatura</h3>
  <div class="grade" id="gradeTemp"></div>

  <h3>Energia e atuadores</h3>
  <div class="grade">
    <div class="card">
      <h2>Corrente INA219 <span class="tag" id="tagIna">--</span></h2>
      <div class="linha"><span>Tensão</span><b><span id="tensao">--</span> V</b></div>
      <div class="linha"><span>Corrente</span><b><span id="corrente">--</span> mA</b></div>
      <div class="linha"><span>Potência</span><b><span id="potencia">--</span> mW</b></div>
      <small class="pino">SDA 21 · SCL 22</small>
    </div>

    <div class="card">
      <h2>Buzzer <span class="tag" id="tagBuzzer">--</span></h2>
      <button id="btnBuzzer" onclick="cmd('/buzzer')">Ligar buzzer</button>
      <small class="pino">pino 14</small>
    </div>

    <div class="card">
      <h2>Motor (Ponte H) <span class="tag" id="tagMotor">--</span></h2>
      <div class="botoes">
        <button class="secundario" onclick="cmd('/motor?sentido=1')">&#8635; Horário</button>
        <button class="secundario" onclick="cmd('/motor?sentido=-1')">&#8634; Anti-horário</button>
        <button class="parar" onclick="cmd('/motor?sentido=0')">&#9632; Parar</button>
      </div>
      <div class="linha" style="margin-top:10px"><span>Velocidade</span><b><span id="velTxt">--</span>%</b></div>
      <input type="range" id="vel" min="0" max="100" step="5"
             oninput="velTxt.textContent=this.value"
             onchange="cmd('/velocidade?valor='+this.value)">
    </div>
  </div>
</main>

<footer><b>WBM</b> Technology · Testador de Componentes</footer>

<script>
const $ = id => document.getElementById(id);

// Mesma ordem dos vetores PINOS_TRIG/PINOS_ECHO e PINOS_TEMP do main.cpp
const ULTRA = ['TRIG 18 · ECHO 19', 'TRIG 13 · ECHO 12', 'TRIG 27 · ECHO 26', 'TRIG 25 · ECHO 33'];
const TEMP  = ['pino 23', 'pino 32'];

function card(id, titulo, pino, unid) {
  return `<div class="card">
    <h2>${titulo} <span class="tag" id="tag_${id}">--</span></h2>
    <div><span class="valor" id="${id}">--</span> <span class="unid">${unid}</span></div>
    <small class="pino">${pino}</small>
  </div>`;
}
$('gradeUltra').innerHTML = ULTRA.map((p, i) => card('ultra' + i, 'Ultrassônico ' + (i + 1), p, 'cm')).join('');
$('gradeTemp').innerHTML  = TEMP.map((p, i) => card('temp' + i, 'DS18B20 ' + (i + 1), p, '°C')).join('');

let arrastando = false;
$('vel').addEventListener('pointerdown', () => arrastando = true);
$('vel').addEventListener('pointerup',   () => arrastando = false);

function tag(el, texto, classe) { el.textContent = texto; el.className = 'tag ' + (classe || ''); }

function conexao(on) {
  $('conexao').className = 'conexao ' + (on ? 'on' : 'off');
  $('conexaoTxt').textContent = on ? 'online' : 'sem conexão';
}

function mostrar(s) {
  conexao(true);

  s.distancias.forEach((d, i) => {
    if (!s.ultraOk[i]) { $('ultra' + i).textContent = '--'; tag($('tag_ultra' + i), 'não conectado', 'erro'); }
    else if (d === null) { $('ultra' + i).textContent = '--'; tag($('tag_ultra' + i), 'fora de alcance', 'ativo'); }
    else { $('ultra' + i).textContent = d.toFixed(1); tag($('tag_ultra' + i), 'OK', 'ok'); }
  });

  s.temperaturas.forEach((t, i) => {
    if (t === null) { $('temp' + i).textContent = '--'; tag($('tag_temp' + i), 'não encontrado', 'erro'); }
    else { $('temp' + i).textContent = t.toFixed(1); tag($('tag_temp' + i), 'OK', 'ok'); }
  });

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
  tag($('tagMotor'), m, s.motorSentido ? 'ok' : '');
  if (!arrastando) { $('vel').value = s.motorVelocidade; $('velTxt').textContent = s.motorVelocidade; }
}

async function cmd(url) {
  try { mostrar(await (await fetch(url)).json()); }
  catch (e) { conexao(false); }
}

setInterval(() => cmd('/status'), 1000);
cmd('/status');
</script>
</body>
</html>
)rawliteral";

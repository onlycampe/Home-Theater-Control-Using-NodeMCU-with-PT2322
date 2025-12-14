#ifndef WEBPAGES_H
#define WEBPAGES_H

#include <Arduino.h>

// =============================================================================
// HTML armazenado na Flash (PROGMEM) para economizar RAM
// =============================================================================
// Ao usar PROGMEM, o HTML fica na Flash (3MB) em vez da RAM (80KB)
// This frees up ~3.5KB of RAM for the system!

const char HTML_HEADER[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<title>Control Panel</title>
<meta name='theme-color' content='#0f0c29'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.7.2/css/all.min.css'>
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap');
*{user-select:none;box-sizing:border-box;}
body{font-family:'Inter',sans-serif;margin:0;padding:0;background:linear-gradient(135deg,#0f0c29 0%,#302b63 50%,#24243e 100%);background-attachment:fixed;color:#fff;width:100vw;display:flex;justify-content:center;align-items:center;min-height:100vh;}
.container{width:100%;text-align:center;flex-direction:column;justify-content:flex-start;align-items:center;min-height:100vh;padding:60px 20px;font-size:50px;}
h1{font-size:clamp(20px,0.6em,40px);margin:30px 0 10px 0;font-weight:700;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;letter-spacing:-0.02em;}
p{font-size:0.35em;color:#a0aec0;margin:0;}
.button-container{width:100%;display:flex;flex-wrap:wrap;justify-content:center;gap:20px;margin:60px 0;}
button{font-size:1.5em;padding:0;cursor:pointer;border-radius:24px;color:#fff;border:none;width:25vw;height:25vw;max-width:180px;max-height:180px;min-width:100px;min-height:100px;box-shadow:0 10px 30px rgba(0,0,0,0.4),0 1px 8px rgba(0,0,0,0.2);user-select:none;transition:all 0.3s cubic-bezier(0.4,0,0.2,1);position:relative;overflow:hidden;display:flex;align-items:center;justify-content:center;}
button::before{content:'';position:absolute;top:50%;left:50%;width:0;height:0;border-radius:50%;background:rgba(255,255,255,0.1);transform:translate(-50%,-50%);transition:width 0.6s,height 0.6s;}
button:active{transform:translateY(-2px) scale(1.02);}
button i{position:relative;z-index:1;filter:drop-shadow(0 2px 4px rgba(0,0,0,0.3));font-size:clamp(24px,0.8em,48px);}
.red{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);}
.green{background:linear-gradient(135deg,#f093fb 0%,#f5576c 100%);}
@keyframes pulse-glow{0%,100%{box-shadow:0 0 20px rgba(102,126,234,0.6),0 0 40px rgba(102,126,234,0.4);transform:scale(1);}50%{box-shadow:0 0 30px rgba(245,87,108,0.8),0 0 60px rgba(245,87,108,0.5);transform:scale(1.08);}}
.pulse{animation:pulse-glow 0.4s ease-in-out;}
form{width:100%;max-width:600px;flex-direction:column;align-items:center;justify-content:center;margin:40px auto;padding:0 20px;}
.form-text{font-size:0.4em;font-weight:600;display:block;margin:20px 0 5px 0;color:#cbd5e0;text-transform:uppercase;letter-spacing:0.05em;}
input[type=range]{-webkit-appearance:none;width:100%;height:12px;background:rgba(255,255,255,0.1);border-radius:10px;outline:none;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.15);cursor:pointer;margin:15px 0 35px 0;box-shadow:inset 0 2px 8px rgba(0,0,0,0.3);}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:28px;height:28px;border-radius:50%;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);cursor:pointer;box-shadow:0 4px 12px rgba(102,126,234,0.6),0 2px 4px rgba(0,0,0,0.3);}
.footer-links{width:100%;display:flex;gap:20px;justify-content:center;align-items:center;margin-top:50px;padding-bottom:40px;font-size:0.35em;}
.footer-links a{color:#718096;text-decoration:none;display:flex;align-items:center;gap:8px;padding:8px 16px;border-radius:20px;background:rgba(255,255,255,0.05);backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);transition:all 0.3s ease;}
.footer-links a.status-link{color:#a0aec0;background:rgba(102,126,234,0.15);border-color:rgba(102,126,234,0.3);}
.footer-links .separator{color:#4a5568;}
</style>
</head>
<body>
<div class='container'>
<h1>Home Theater Control</h1>
<div class='button-container'>
)=====";

const char HTML_FOOTER[] PROGMEM = R"=====(
</div>
<div class='footer-links'>
<a href='https://github.com/onlycampe' target='_blank'>
<i class='fa-brands fa-github-alt'></i> <span>OnlyCampe</span>
</a>
<span class='separator'>|</span>
<a href='/status-page' class='status-link'>
<i class='fa-solid fa-chart-line'></i> <span>Status</span>
</a>
<span class='separator'>|</span>
<a href='/test-tone.html' class='status-link'>
<i class='fa-solid fa-waveform-lines'></i> <span>Teste</span>
</a>
</div>
</div>
</div>
<script>
function updateVolume(el){fetch('/setVolume',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`${el.name}=${el.value}`});}
function toggleButton(id){let btn=document.getElementById(id+'Button');if(id==='audioIn'||id==='audio51'){btn.classList.add('pulse');setTimeout(()=>btn.classList.remove('pulse'),300);fetch('/setFunc',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`func=${id}&state=1`});}else{fetch('/setFunc',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`func=${id}&state=${btn.classList.contains('green')?'0':'1'}`}).then(()=>{btn.classList.toggle('red');btn.classList.toggle('green');});}}
</script>
</body>
</html>
)=====";

const char TEST_TONE_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<title>Channel Test</title>
<meta name='theme-color' content='#0f0c29'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.7.2/css/all.min.css'>
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap');
body{font-family:'Inter',sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#0f0c29 0%,#302b63 50%,#24243e 100%);background-attachment:fixed;color:#fff;min-height:100vh;}
.container{max-width:900px;margin:0 auto;}
h1{color:#fff;text-align:center;font-size:2.2em;font-weight:700;margin-bottom:10px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;}
.info{background:rgba(102,126,234,0.15);border-left:4px solid #667eea;padding:15px;border-radius:8px;margin:20px 0;color:#cbd5e0;font-size:0.95em;}
.section{background:rgba(255,255,255,0.08);backdrop-filter:blur(20px);border-radius:20px;padding:30px;margin:25px 0;border:1px solid rgba(255,255,255,0.15);box-shadow:0 8px 32px rgba(0,0,0,0.3);}
.section h2{font-size:1.4em;margin:0 0 20px 0;color:#cbd5e0;font-weight:600;}
.channel-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:15px;margin:20px 0;}
.channel-btn{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);border:none;color:#fff;padding:20px;border-radius:12px;cursor:pointer;font-size:1em;font-weight:600;transition:all 0.3s ease;box-shadow:0 4px 15px rgba(102,126,234,0.4);}
.channel-btn:hover{transform:translateY(-2px);box-shadow:0 6px 20px rgba(102,126,234,0.6);}
.channel-btn.active{background:linear-gradient(135deg,#f093fb 0%,#f5576c 100%);animation:pulse 1s infinite;}
@keyframes pulse{0%,100%{transform:scale(1);}50%{transform:scale(1.05);}}
.control-group{display:flex;gap:15px;justify-content:center;margin:20px 0;flex-wrap:wrap;}
.control-btn{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);border:none;color:#fff;padding:15px 30px;border-radius:12px;cursor:pointer;font-size:1.1em;font-weight:600;transition:all 0.3s ease;}
.control-btn:hover{transform:translateY(-2px);}
.control-btn.stop{background:linear-gradient(135deg,#f093fb 0%,#f5576c 100%);}
.control-btn.all{background:linear-gradient(135deg,#4caf50 0%,#45a049 100%);}
.status-indicator{display:inline-block;width:12px;height:12px;border-radius:50%;margin-right:8px;background:#48bb78;animation:blink 1s infinite;}
@keyframes blink{0%,100%{opacity:1;}50%{opacity:0.3;}}
.btn-back{display:inline-block;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;padding:16px 32px;border-radius:12px;text-decoration:none;margin:30px 0;font-size:1.1em;font-weight:600;transition:all 0.3s ease;}
.btn-back:hover{transform:translateY(-2px);}
</style>
</head>
<body>
<div class='container'>
<h1><i class='fa-solid fa-waveform-lines'></i> Channel Test</h1>
<div class='info'>
<strong>💡 Note:</strong> Play music/audio before testing. The system isolates each channel for you to hear individually.
</div>
<div class='section'>
<h2>Channels</h2>
<div class='channel-grid'>
<button class='channel-btn' onclick='testChannel("front_left")' id='btn_front_left'>Front L</button>
<button class='channel-btn' onclick='testChannel("front_right")' id='btn_front_right'>Front R</button>
<button class='channel-btn' onclick='testChannel("center")' id='btn_center'>Center</button>
<button class='channel-btn' onclick='testChannel("subwoofer")' id='btn_subwoofer'>Subwoofer</button>
<button class='channel-btn' onclick='testChannel("rear_left")' id='btn_rear_left'>Rear L</button>
<button class='channel-btn' onclick='testChannel("rear_right")' id='btn_rear_right'>Rear R</button>
</div>
</div>
<div class='section'>
<h2>Controles</h2>
<div class='control-group'>
<button class='control-btn all' onclick='testChannel("all")'><i class='fa-solid fa-volume-high'></i> All</button>
<button class='control-btn stop' onclick='stopTest()'><i class='fa-solid fa-stop'></i> Stop</button>
</div>
<div id='status' style='text-align:center;margin-top:15px;color:#cbd5e0;'></div>
</div>
<a href='/' class='btn-back'><i class='fa-solid fa-arrow-left'></i> Back</a>
</div>
<script>
let active=null;let seq=null;
function testChannel(c){stopTest();active=c;if(c!=='all')document.getElementById('btn_'+c).classList.add('active');fetch('/testTone',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`channel=${c}`}).then(()=>{document.getElementById('status').innerHTML=c==='all'?'<i class="fa-solid fa-volume-high"></i> All active':`<span class='status-indicator'></span>Testing: ${c.replace('_',' ')}`;});}
function stopTest(){document.querySelectorAll('.channel-btn').forEach(b=>b.classList.remove('active'));active=null;if(seq)clearTimeout(seq);fetch('/stopTest',{method:'POST'}).then(()=>{document.getElementById('status').innerHTML='';});}
window.addEventListener('beforeunload',()=>stopTest());
</script>
</body>
</html>
)=====";

const char STATUS_PAGE_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<title>Status - Home Theater</title>
<meta name='theme-color' content='#0f0c29'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<meta http-equiv='refresh' content='5'>
<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.7.2/css/all.min.css'>
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap');
body{font-family:'Inter',sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#0f0c29 0%,#302b63 50%,#24243e 100%);background-attachment:fixed;color:#fff;min-height:100vh;}
.container{max-width:900px;margin:0 auto;}
h1{color:#fff;text-align:center;font-size:2.2em;font-weight:700;margin-bottom:10px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;}
h2{font-size:1.4em;margin:0 0 20px 0;color:#cbd5e0;font-weight:600;}
.card{background:rgba(255,255,255,0.08);backdrop-filter:blur(20px);border-radius:20px;padding:30px;margin:25px 0;border:1px solid rgba(255,255,255,0.15);box-shadow:0 8px 32px rgba(0,0,0,0.3);transition:all 0.3s ease;}
.card:hover{transform:translateY(-4px);box-shadow:0 12px 40px rgba(0,0,0,0.4);border-color:rgba(102,126,234,0.3);}
.info-grid{display:grid;grid-template-columns:auto 1fr;gap:15px 30px;font-size:1.05em;}
.label{color:#a0aec0;font-weight:600;text-transform:uppercase;font-size:0.85em;letter-spacing:0.05em;}
.value{color:#e2e8f0;text-align:right;font-weight:500;}
.status-ok{color:#48bb78;font-weight:600;}
.status-warn{color:#ed8936;font-weight:600;}
.btn-back{display:inline-block;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;padding:16px 32px;border-radius:12px;text-decoration:none;margin:30px 0;font-size:1.1em;font-weight:600;transition:all 0.3s ease;box-shadow:0 4px 15px rgba(102,126,234,0.4);}
.btn-back:hover{transform:translateY(-2px);box-shadow:0 6px 20px rgba(102,126,234,0.6);}
.refresh-note{text-align:center;color:#718096;font-size:0.95em;margin-top:30px;padding:15px;background:rgba(255,255,255,0.05);border-radius:12px;border:1px solid rgba(255,255,255,0.1);}
</style>
</head>
<body>
<div class='container'>
<h1><i class='fa-solid fa-chart-line'></i> System Status</h1>
)=====";

const char STATUS_PAGE_FOOTER[] PROGMEM = R"=====(
<a href='/' class='btn-back'><i class='fa-solid fa-arrow-left'></i> Back</a>
<div class='refresh-note'><i class='fa-solid fa-rotate'></i> Página atualiza automaticamente a cada 5 segundos</div>
</div>
</body>
</html>
)=====";

#endif


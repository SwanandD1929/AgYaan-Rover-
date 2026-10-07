const char dashboard[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>APOLLO-X LUNAR COMMAND HUD</title>
<style>
body { background: #020408; color: #00f3ff; font-family: 'Courier New', monospace; text-align: center; margin: 0; padding: 15px; }
h1 { font-size: 24px; letter-spacing: 4px; color: #ff6600; margin-bottom: 2px; text-shadow: 0 0 12px rgba(255,102,0,0.4); }
.hud-sub { font-size: 11px; color: #7799bb; margin-bottom: 15px; letter-spacing: 2px; }

.dashboard-container { max-width: 1300px; margin: auto; display: flex; flex-direction: column; gap: 15px; }
.top-section { display: grid; grid-template-columns: 1fr 1.3fr 1fr; gap: 15px; }
.panel { background: rgba(8, 16, 28, 0.9); border: 1px solid #00f3ff55; border-radius: 6px; padding: 15px; box-shadow: inset 0 0 20px rgba(0,243,255,0.03); text-align: left; }
.panel h2 { font-size: 13px; color: #ff6600; border-bottom: 1px dashed #00f3ff44; padding-bottom: 6px; margin-top: 0; letter-spacing: 2px; text-align: center; }

.moon-panel { display: flex; flex-direction: column; align-items: center; justify-content: center; background: radial-gradient(circle, #152238 0%, #050b14 80%); }
.moon-graphic { width: 80px; height: 80px; border-radius: 50%; background: radial-gradient(circle at 35% 35%, #ffffff 0%, #8899aa 50%, #223344 100%); box-shadow: 0 0 20px rgba(255,255,255,0.2); margin: 6px auto; position: relative; overflow: hidden; }
.moon-graphic::after { content: ""; position: absolute; top: 10px; left: 30px; width: 14px; height: 10px; background: rgba(0,0,0,0.15); border-radius: 50%; box-shadow: 20px 12px 0 3px rgba(0,0,0,0.15); }

.compass-box { position: relative; width: 70px; height: 70px; margin: 6px auto; border: 2px solid #00f3ff; border-radius: 50%; display: flex; align-items: center; justify-content: center; }
.compass-needle { width: 3px; height: 28px; background: #ff6600; position: absolute; top: 7px; transform-origin: bottom center; transition: transform 0.08s linear; box-shadow: 0 0 6px #ff6600; }
.compass-center { width: 6px; height: 6px; background: #00f3ff; border-radius: 50%; z-index: 2; }

canvas { background: #010204; border: 1px solid #00f3ff44; border-radius: 4px; display: block; margin: 4px auto; }

.metric-row { display: flex; justify-content: space-between; margin: 5px 0; font-size: 12px; border-bottom: 1px solid #ffffff11; padding-bottom: 3px; }
.metric-val { color: #fff; font-weight: bold; text-shadow: 0 0 5px rgba(255,255,255,0.5); }

.mode-selector { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 10px; }
.mode-btn { background: #050b14; border: 1px solid #7799bb; color: #7799bb; padding: 8px; font-family: monospace; font-weight: bold; font-size: 11px; border-radius: 4px; cursor: pointer; letter-spacing: 1px; }
.mode-btn.active { background: #ff6600; border-color: #ff6600; color: #fff; box-shadow: 0 0 10px rgba(255,102,0,0.5); }

.input-group { margin: 8px 0; display: flex; flex-direction: column; gap: 5px; }
.input-row { display: flex; gap: 6px; }
input[type=number], input[type=text] { background: #020408; border: 1px solid #00f3ff; color: #fff; padding: 6px; font-family: monospace; border-radius: 4px; }
.cmd-btn { width: 35%; background: #00f3ff; border: none; color: #020408; font-weight: bold; border-radius: 4px; cursor: pointer; font-family: monospace; }

.controls { display: grid; grid-template-columns: 65px 65px 65px; gap: 6px; justify-content: center; margin-top: 8px; }
button { width: 65px; height: 36px; background: #0a1422; border: 1px solid #00f3ff; color: #00f3ff; font-family: monospace; font-weight: bold; border-radius: 4px; cursor: pointer; }
button:active { background: #00f3ff; color: #030508; }
.stop-btn { background: #330505; border-color: #ff3333; color: #ff3333; }

.cam-container { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-top: 8px; }
.cam-feed { background: #000; border: 1px solid #00f3ff44; height: 210px; display: flex; align-items: center; justify-content: center; color: #446688; font-size: 11px; border-radius: 4px; position: relative; overflow: hidden; letter-spacing: 1px; }
.cam-feed iframe { width: 100%; height: 100%; border: none; overflow: hidden; }
.cam-feed::before { content: attr(data-label); position: absolute; top: 6px; left: 8px; font-size: 9px; color: #ff6600; letter-spacing: 1px; z-index: 2; background: rgba(0,0,0,0.7); padding: 2px 4px; border-radius: 2px; }
</style>
</head>
<body>

<h1>APOLLO-X LUNAR COMMAND HUD</h1>
<div class="hud-sub">AUTONOMOUS PATH PLANNING | 3D ISOMETRIC SCANNING & STEREO TELEMETRY</div>

<div class="dashboard-container">

    <div class="top-section">
        <!-- 1. LUNAR ENVIRONMENT -->
        <div class="panel moon-panel">
            <h2>LUNAR ENVIRONMENT</h2>
            <div class="moon-graphic"></div>
            <div style="width: 100%; margin-top: 4px;">
                <div class="metric-row"><span>SURFACE TEMP:</span><span class="metric-val">-20.0 °C</span></div>
                <div class="metric-row"><span>HEADING:</span><span class="metric-val" id="heading">0.0°</span></div>
                <div class="compass-box">
                    <div class="compass-needle" id="compassNeedle"></div>
                    <div class="compass-center"></div>
                </div>
            </div>
        </div>

        <!-- 2. 3D ISOMETRIC LUNAR SCAN -->
        <div class="panel">
            <h2>3D ISOMETRIC SCAN & RE-ROUTE MAP</h2>
            <canvas id="pathCanvas" width="360" height="190"></canvas>
            <div class="metric-row"><span>ROVER POSE (X, Y):</span><span class="metric-val" id="xyPos">0, 0 cm</span></div>
            <div class="metric-row"><span>ACTIVE TARGET:</span><span class="metric-val" id="targetPos" style="color:#ffcc00">NONE</span></div>
            <div class="metric-row"><span>TOTAL DIST / DISP:</span><span class="metric-val" id="distDisp">0.0 / 0.0 cm</span></div>
        </div>

        <!-- 3. GUIDANCE & EXPLORE COMMANDS -->
        <div class="panel">
            <h2>AUTONOMOUS GUIDANCE</h2>
            <div class="mode-selector">
                <button class="mode-btn active" id="btnManual" onclick="setMode('manual')">MANUAL</button>
                <button class="mode-btn" id="btnAuto" onclick="setMode('auto')">AUTO EXPLORE</button>
            </div>

            <div class="metric-row"><span>NAV STATUS:</span><span class="metric-val" id="obsStatus" style="color:#00ffcc">CLEAR</span></div>
            <div class="metric-row"><span>LiDAR RANGE:</span><span class="metric-val" id="lidarDist">--- cm</span></div>

            <div class="input-group">
                <div class="input-row">
                    <input type="number" id="cmInput" value="100" style="width:65%;" placeholder="Distance (cm)">
                    <button class="cmd-btn" onclick="sendExplore('forward')">EXPLORE</button>
                </div>
            </div>

            <div class="controls">
                <div></div>
                <button onclick="move('forward')">W</button>
                <div></div>
                <button onclick="move('left')">A</button>
                <button class="stop-btn" onclick="stop()">STP</button>
                <button onclick="move('right')">D</button>
                <div></div>
                <button onclick="move('backward')">S</button>
                <div></div>
            </div>
        </div>
    </div>

    <!-- 4. STEREO VISION -->
    <div class="panel">
        <h2>STEREOSCOPIC HAZ-CAM VISION NODES</h2>
        <div style="display: flex; justify-content: center; gap: 8px; margin-bottom: 8px;">
            <input type="text" id="cam1_ip" value="192.168.204.228" style="width:150px; font-size:11px;" placeholder="Node 1 IP">
            <input type="text" id="cam2_ip" value="192.168.204.192" style="width:150px; font-size:11px;" placeholder="Node 2 IP">
            <button onclick="reloadStreams()" style="width: auto; height: auto; padding: 5px 12px; font-size: 11px;">SYNC FEEDS</button>
        </div>
        <div class="cam-container">
            <div class="cam-feed" data-label="PORT HAZ-CAM (NODE 1)">
                <iframe id="frameCam1" src="" scrolling="no"></iframe>
            </div>
            <div class="cam-feed" data-label="STARBOARD HAZ-CAM (NODE 2)">
                <iframe id="frameCam2" src="" scrolling="no"></iframe>
            </div>
        </div>
    </div>

</div>

<script>
const canvas = document.getElementById("pathCanvas");
const ctx = canvas.getContext("2d");
let pathPoints = [];
let obstacleMap = []; 

function reloadStreams() {
    let ip1 = document.getElementById("cam1_ip").value.trim();
    let ip2 = document.getElementById("cam2_ip").value.trim();
    let frame1 = document.getElementById("frameCam1");
    let frame2 = document.getElementById("frameCam2");

    frame1.src = "about:blank";
    frame2.src = "about:blank";

    setTimeout(() => {
        let t = new Date().getTime();
        frame1.src = "http://" + ip1 + ":81/stream?" + t;
        frame2.src = "http://" + ip2 + ":81/stream?" + t;
    }, 200);
}

window.addEventListener('DOMContentLoaded', () => { setTimeout(reloadStreams, 500); });

function setMode(mode) {
    fetch("/mode?state=" + mode).then(() => updateModeUI(mode));
}

function updateModeUI(mode) {
    if(mode.toLowerCase() === 'auto' || mode.toLowerCase() === 'autonomous') {
        document.getElementById("btnAuto").classList.add("active");
        document.getElementById("btnManual").classList.remove("active");
    } else {
        document.getElementById("btnManual").classList.add("active");
        document.getElementById("btnAuto").classList.remove("active");
    }
}

function move(dir) { fetch("/move?direction=" + dir); }
function stop() { fetch("/stop"); }

function sendExplore(dir) {
    let cm = document.getElementById("cmInput").value;
    fetch(`/explore?dir=${dir}&cm=${cm}`);
}

document.addEventListener("keydown", function(e) {
    if(e.repeat) return;
    let key = e.key.toLowerCase();
    if(key === 'w') move('forward');
    if(key === 's') move('backward');
    if(key === 'a') move('left');
    if(key === 'd') move('right');
    if(key === ' ') stop();
});

document.addEventListener("keyup", function(e) {
    let key = e.key.toLowerCase();
    if(['w','s','a','d'].includes(key)) stop();
});

// Project 2D coordinates into an Isometric 3D Space
function toIso(x, y, z = 0) {
    let originX = canvas.width / 2;
    let originY = canvas.height / 2 + 30;
    let isoX = originX + (x - y) * 0.707;
    let isoY = originY + (x + y) * 0.35 - z;
    return { x: isoX, y: isoY };
}

function draw3DMap(rx, ry, headingDeg, lidarCm, tx, ty, hasTarget) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Draw Isometric 3D Ground Grid
    let gridSize = 160;
    let step = 40;
    ctx.strokeStyle = "rgba(0, 243, 255, 0.12)";
    ctx.lineWidth = 1;
    for (let x = -gridSize; x <= gridSize; x += step) {
        let p1 = toIso(x, -gridSize);
        let p2 = toIso(x, gridSize);
        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y); ctx.stroke();
    }
    for (let y = -gridSize; y <= gridSize; y += step) {
        let p1 = toIso(-gridSize, y);
        let p2 = toIso(gridSize, y);
        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y); ctx.stroke();
    }

    let scale = 0.7;
    let roverIso = toIso(rx * scale, ry * scale);

    // Rover path history
    pathPoints.push({ x: rx * scale, y: ry * scale });
    if (pathPoints.length > 80) pathPoints.shift();

    if (pathPoints.length > 1) {
        ctx.strokeStyle = "#ff6600";
        ctx.lineWidth = 2;
        ctx.beginPath();
        let p0 = toIso(pathPoints[0].x, pathPoints[0].y);
        ctx.moveTo(p0.x, p0.y);
        for (let pt of pathPoints) {
            let p = toIso(pt.x, pt.y);
            ctx.lineTo(p.x, p.y);
        }
        ctx.stroke();
    }

    // LiDAR 3D Hit registration
    if (lidarCm > 5 && lidarCm < 150) {
        let rad = headingDeg * (Math.PI / 180.0);
        let obsWorldX = (rx * scale) + (lidarCm * scale * Math.cos(rad));
        let obsWorldY = (ry * scale) + (lidarCm * scale * Math.sin(rad));
        obstacleMap.push({ x: obsWorldX, y: obsWorldY, life: 100 });
    }

    // Render 3D Hazard Pillars
    for (let i = obstacleMap.length - 1; i >= 0; i--) {
        let obs = obstacleMap[i];
        let base = toIso(obs.x, obs.y, 0);
        let top = toIso(obs.x, obs.y, 18); // 18px pillar height

        ctx.strokeStyle = `rgba(255, 50, 50, ${obs.life / 100})`;
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.moveTo(base.x, base.y);
        ctx.lineTo(top.x, top.y);
        ctx.stroke();

        ctx.fillStyle = "#ff3333";
        ctx.beginPath(); ctx.arc(top.x, top.y, 3, 0, Math.PI * 2); ctx.fill();

        obs.life -= 1;
        if (obs.life <= 0) obstacleMap.splice(i, 1);
    }

    // Draw Target Flag (if exploring)
    if (hasTarget) {
        let targetIsoBase = toIso(tx * scale, ty * scale, 0);
        let targetIsoTop = toIso(tx * scale, ty * scale, 25);
        ctx.strokeStyle = "#ffcc00";
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(targetIsoBase.x, targetIsoBase.y);
        ctx.lineTo(targetIsoTop.x, targetIsoTop.y);
        ctx.stroke();

        ctx.fillStyle = "#ffcc00";
        ctx.beginPath();
        ctx.moveTo(targetIsoTop.x, targetIsoTop.y);
        ctx.lineTo(targetIsoTop.x + 10, targetIsoTop.y + 4);
        ctx.lineTo(targetIsoTop.x, targetIsoTop.y + 8);
        ctx.fill();
    }

    // Draw 3D Rover Body & LiDAR Beam
    let roverTop = toIso(rx * scale, ry * scale, 10);
    ctx.fillStyle = "#00f3ff";
    ctx.beginPath(); ctx.arc(roverTop.x, roverTop.y, 5, 0, Math.PI * 2); ctx.fill();

    let rayAngle = headingDeg * (Math.PI / 180.0);
    let beamDist = (lidarCm > 0 && lidarCm < 150) ? lidarCm * scale : 45;
    let rayTarget = toIso((rx * scale) + beamDist * Math.cos(rayAngle), (ry * scale) + beamDist * Math.sin(rayAngle), 10);

    ctx.strokeStyle = "rgba(0, 255, 204, 0.7)";
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(roverTop.x, roverTop.y);
    ctx.lineTo(rayTarget.x, rayTarget.y);
    ctx.stroke();
}

let isFetching = false;
function updateTelemetry() {
    if (isFetching) return;
    isFetching = true;

    fetch("/sensor")
    .then(res => res.json())
    .then(data => {
        document.getElementById("distDisp").innerText = `${data.tot.toFixed(1)} / ${data.disp.toFixed(1)} cm`;
        document.getElementById("heading").innerText = data.heading.toFixed(1) + "°";
        document.getElementById("xyPos").innerText = `${data.x.toFixed(0)}, ${data.y.toFixed(0)} cm`;
        document.getElementById("lidarDist").innerText = data.lidar + " cm";

        document.getElementById("compassNeedle").style.transform = `rotate(${data.heading}deg)`;

        let obsElem = document.getElementById("obsStatus");
        let targetElem = document.getElementById("targetPos");

        if (data.targetActive) {
            targetElem.innerText = `${data.tx.toFixed(0)}, ${data.ty.toFixed(0)} cm (${data.navState})`;
        } else {
            targetElem.innerText = "STANDBY";
        }

        if (data.lidar > 0 && data.lidar < 30) {
            obsElem.innerText = `HAZARD (${data.lidar}cm) - RE-ROUTING`;
            obsElem.style.color = "#ff3333";
        } else {
            obsElem.innerText = data.navState || "PATH CLEAR";
            obsElem.style.color = "#00ffcc";
        }

        if(data.mode) updateModeUI(data.mode);

        draw3DMap(data.x, data.y, data.heading, data.lidar, data.tx, data.ty, data.targetActive);
        isFetching = false;
    })
    .catch(() => { isFetching = false; });
}

setInterval(updateTelemetry, 100);
</script>
</body>
</html>
)rawliteral";
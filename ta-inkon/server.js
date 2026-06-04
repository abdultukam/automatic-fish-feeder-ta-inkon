// =============================================
// AUTO FISH FEEDER - Laptop Server
// Run: node server.js
// Then expose with: ngrok http 3000
// =============================================

const http = require("http");
const fs   = require("fs");
const path = require("path");
const url  = require("url");

const PORT = 3000;

// Pending command queue (set by dashboard, consumed by ESP32)
let pendingCommand = "NONE";

// Feed history log (last 20 events)
let feedLog = [];

// Last ESP32 activity timestamp
let lastESPActivity = 0;

function addLog(type, angle, time) {
  feedLog.unshift({ type, angle, time, ts: Date.now() });
  if (feedLog.length > 20) feedLog.pop();
}

const server = http.createServer((req, res) => {
  const parsed = url.parse(req.url, true);
  const route  = parsed.pathname;

  // CORS — allow dashboard from any origin (ngrok, etc.)
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type");
  if (req.method === "OPTIONS") { res.writeHead(204); return res.end(); }

  // --- ESP32 polls this every 2 seconds ---
  if (route === "/command") {
    lastESPActivity = Date.now();
    res.setHeader("Content-Type", "text/plain");
    res.writeHead(200);
    res.end(pendingCommand);
    if (pendingCommand !== "NONE") {
      console.log(`[CMD] Sent to ESP32: ${pendingCommand}`);
      pendingCommand = "NONE"; // Clear after delivery
    }
    return;
  }

  // --- ESP32 reports events here ---
  if (route === "/event") {
    lastESPActivity = Date.now();
    const { type, angle, time } = parsed.query;
    addLog(type, angle || 0, time || "?");
    console.log(`[EVT] ${type} | angle=${angle} | time=${time}`);
    res.writeHead(200);
    res.end("OK");
    return;
  }

  // --- Dashboard triggers a feed ---
  if (route === "/feed" && req.method === "POST") {
    let body = "";
    req.on("data", d => body += d);
    req.on("end", () => {
      try {
        const { angle } = JSON.parse(body);
        const a = parseInt(angle) || 45;
        pendingCommand = `FEED:${a}`;
        console.log(`[FEED] Queued: FEED:${a}`);
        res.setHeader("Content-Type", "application/json");
        res.writeHead(200);
        res.end(JSON.stringify({ ok: true, queued: `FEED:${a}` }));
      } catch {
        res.writeHead(400);
        res.end("Bad JSON");
      }
    });
    return;
  }

  // --- Dashboard fetches log ---
  if (route === "/log") {
    res.setHeader("Content-Type", "application/json");
    res.writeHead(200);
    const lastSeenAgo = lastESPActivity > 0 ? (Date.now() - lastESPActivity) : -1;
    res.end(JSON.stringify({
      log: feedLog,
      lastSeenAgo: lastSeenAgo,
      serverTime: Date.now()
    }));
    return;
  }

  // --- Serve dashboard HTML ---
  if (route === "/" || route === "/index.html") {
    const file = path.join(__dirname, "dashboard.html");
    if (fs.existsSync(file)) {
      res.setHeader("Content-Type", "text/html");
      res.writeHead(200);
      fs.createReadStream(file).pipe(res);
    } else {
      res.writeHead(404);
      res.end("dashboard.html not found — place it next to server.js");
    }
    return;
  }

  res.writeHead(404);
  res.end("Not found");
});

server.listen(PORT, () => {
  console.log(`\n🐟 Fish Feeder Server running on http://localhost:${PORT}`);
  console.log(`\nTo expose online, run:\n  npx ngrok http ${PORT}\n`);
});

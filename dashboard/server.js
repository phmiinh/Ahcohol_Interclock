const path = require("path");
const http = require("http");
const express = require("express");
const { Server } = require("socket.io");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");

const DASHBOARD_PORT = Number(process.env.DASHBOARD_PORT || 3030);
const SERIAL_BAUD_RATE = Number(process.env.SERIAL_BAUD_RATE || 115200);
const STALE_DEVICE_MS = Number(process.env.STALE_DEVICE_MS || 5000);
const args = process.argv.slice(2);
const requestedPort = readArgValue("--serial") || process.env.SERIAL_PORT || null;
const mockMode = args.includes("--mock") || process.env.MOCK_SERIAL === "1";
const listPortsOnly = args.includes("--list-ports");

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
  cors: {
    origin: "*"
  }
});

const store = {
  connection: {
    mode: mockMode ? "mock" : "serial",
    status: mockMode ? "mock" : "starting",
    portName: requestedPort,
    baudRate: SERIAL_BAUD_RATE,
    lastTelemetryAt: null,
    lastError: null
  },
  telemetry: createEmptyTelemetry(),
  events: [],
  rawLines: []
};

let activePort = null;
let activeParser = null;
let reconnectTimer = null;
let staleEventSent = false;
let mockTimer = null;
let mockStep = 0;

function readArgValue(flag) {
  const index = args.indexOf(flag);
  if (index === -1 || index === args.length - 1) {
    return null;
  }
  return args[index + 1];
}

function createEmptyTelemetry() {
  return {
    state: "DISCONNECTED",
    result: "PENDING",
    fault: "NONE",
    sensorWarning: "NONE",
    liveAdc: 0,
    sampledAdc: 0,
    threshold: 2000,
    percent: 0,
    samplePercent: 0,
    sampleStdDev: 0,
    preheatRemainingMs: 0,
    demoMode: true,
    canStart: false,
    vehicleLocked: true,
    buzzerOn: false,
    servoAngle: 0,
    buttonMode: "module_3pin",
    buttonActiveHigh: true,
    buttonBias: "internal_pulldown",
    sensorWarningDurationMs: 0,
    sampleCount: 0,
    sampleTotal: 20,
    sampleElapsedMs: 0,
    testToResultMs: 0,
    passReadyToUnlockMs: 0,
    startToUnlockMs: 0,
    consecutiveFailCount: 0,
    retestRequired: false,
    retestIntervalMs: 30000,
    retestRemainingMs: 0,
    retestOverdueMs: 0,
    retestGraceRemainingMs: 0,
    retestDueToTestMs: 0,
    retestToResultMs: 0,
    runningSessionMs: 0,
    retestCycleCount: 0,
    uptimeMs: 0,
    updatedAt: null
  };
}

function nowIso() {
  return new Date().toISOString();
}

function normalizeEvent(payload) {
  return {
    id: `${Date.now()}-${Math.random().toString(16).slice(2, 8)}`,
    event: payload.event || "device_event",
    severity: payload.severity || "info",
    message: payload.message || "New device event",
    state: payload.state || store.telemetry.state,
    liveAdc: payload.liveAdc ?? store.telemetry.liveAdc,
    sampledAdc: payload.sampledAdc ?? store.telemetry.sampledAdc,
    threshold: payload.threshold ?? store.telemetry.threshold,
    vehicleLocked: payload.vehicleLocked ?? store.telemetry.vehicleLocked,
    buzzerOn: payload.buzzerOn ?? store.telemetry.buzzerOn,
    servoAngle: payload.servoAngle ?? store.telemetry.servoAngle,
    uptimeMs: payload.uptimeMs ?? store.telemetry.uptimeMs,
    timestamp: nowIso()
  };
}

function pushEvent(payload, shouldBroadcast = true) {
  const event = normalizeEvent(payload);
  store.events.unshift(event);
  store.events = store.events.slice(0, 60);

  if (shouldBroadcast) {
    io.emit("event", event);
    io.emit("snapshot", buildSnapshot());
  }
}

function pushRawLine(line) {
  store.rawLines.unshift({
    id: `${Date.now()}-${Math.random().toString(16).slice(2, 8)}`,
    line,
    timestamp: nowIso()
  });
  store.rawLines = store.rawLines.slice(0, 40);
  io.emit("raw-line", store.rawLines[0]);
}

function buildSnapshot() {
  return {
    connection: store.connection,
    telemetry: store.telemetry,
    events: store.events,
    rawLines: store.rawLines
  };
}

function updateConnection(patch) {
  store.connection = {
    ...store.connection,
    ...patch
  };
  io.emit("connection", store.connection);
  io.emit("snapshot", buildSnapshot());
}

function updateTelemetry(patch) {
  store.telemetry = {
    ...store.telemetry,
    ...patch,
    updatedAt: nowIso()
  };
  store.connection.lastTelemetryAt = Date.now();
  staleEventSent = false;
  if (!mockMode) {
    updateConnection({ status: "online", lastError: null });
  } else {
    io.emit("connection", store.connection);
  }

  io.emit("telemetry", store.telemetry);
  io.emit("snapshot", buildSnapshot());
}

function handleAiJson(line) {
  try {
    const payload = JSON.parse(line.slice(8));
    if (payload.type === "telemetry") {
      updateTelemetry(payload);
      return;
    }

    if (payload.type === "event") {
      pushEvent(payload);
      return;
    }
  } catch (error) {
    pushEvent({
      event: "dashboard_parse_error",
      severity: "warning",
      message: `Cannot parse AI_JSON payload: ${error.message}`
    });
  }
}

function handleSerialLine(line) {
  const trimmed = line.trim();
  if (!trimmed) {
    return;
  }

  pushRawLine(trimmed);
  if (trimmed.startsWith("AI_JSON ")) {
    handleAiJson(trimmed);
  }
}

async function choosePortPath() {
  if (requestedPort) {
    return requestedPort;
  }

  const ports = await SerialPort.list();
  if (!ports.length) {
    return null;
  }

  const preferred = ports.find((port) => {
    const blob = `${port.path} ${port.manufacturer || ""} ${port.friendlyName || ""} ${port.pnpId || ""}`.toLowerCase();
    return blob.includes("usb") || blob.includes("wch") || blob.includes("cp210") || blob.includes("silicon") || blob.includes("esp32");
  });

  return (preferred || ports[0]).path;
}

function clearReconnectTimer() {
  if (reconnectTimer) {
    clearTimeout(reconnectTimer);
    reconnectTimer = null;
  }
}

function scheduleReconnect(delayMs = 3000) {
  if (mockMode || reconnectTimer) {
    return;
  }

  reconnectTimer = setTimeout(() => {
    reconnectTimer = null;
    connectSerial().catch((error) => {
      updateConnection({ status: "error", lastError: error.message });
      scheduleReconnect();
    });
  }, delayMs);
}

async function connectSerial() {
  clearReconnectTimer();

  const portPath = await choosePortPath();
  if (!portPath) {
    updateConnection({
      status: "waiting",
      portName: null,
      lastError: "No serial port detected. Connect ESP32 or run npm run mock."
    });
    scheduleReconnect();
    return;
  }

  updateConnection({
    status: "connecting",
    portName: portPath,
    lastError: null
  });

  const port = new SerialPort({
    path: portPath,
    baudRate: SERIAL_BAUD_RATE,
    autoOpen: false
  });

  await new Promise((resolve, reject) => {
    port.open((error) => {
      if (error) {
        reject(error);
        return;
      }
      resolve();
    });
  });

  activePort = port;
  activeParser = activePort.pipe(new ReadlineParser({ delimiter: "\n" }));
  updateConnection({
    status: "connected",
    portName: portPath,
    lastError: null
  });

  pushEvent({
    event: "serial_connected",
    severity: "info",
    message: `Connected to ${portPath}`
  });

  activeParser.on("data", handleSerialLine);
  activePort.on("close", () => {
    updateConnection({
      status: "disconnected",
      lastError: "Serial port closed"
    });
    pushEvent({
      event: "serial_disconnected",
      severity: "warning",
      message: "ESP32 serial connection closed"
    });
    activeParser = null;
    activePort = null;
    scheduleReconnect();
  });

  activePort.on("error", (error) => {
    updateConnection({
      status: "error",
      lastError: error.message
    });
    pushEvent({
      event: "serial_error",
      severity: "warning",
      message: `Serial error: ${error.message}`
    });
  });
}

function startMockStream() {
  updateConnection({
    mode: "mock",
    status: "mock",
    portName: "MOCK-STREAM",
    lastError: null
  });

  pushEvent({
    event: "mock_started",
    severity: "info",
    message: "Mock realtime stream started"
  });

  mockTimer = setInterval(() => {
    mockStep = (mockStep + 1) % 28;

    let state = "STANDBY_LOCKED";
    let result = "PENDING";
    let liveAdc = 850;
    let sampledAdc = 0;
    let vehicleLocked = true;
    let buzzerOn = false;
    let servoAngle = 0;
    let canStart = false;
    let retestRequired = false;
    let retestRemainingMs = 0;
    let retestOverdueMs = 0;
    let retestGraceRemainingMs = 0;
    let retestDueToTestMs = 0;
    let retestToResultMs = 0;
    let runningSessionMs = 0;
    let retestCycleCount = 0;

    if (mockStep < 5) {
      state = "PREHEAT";
      liveAdc = 500 + mockStep * 80;
    } else if (mockStep < 12) {
      state = "STANDBY_LOCKED";
      liveAdc = 600 + mockStep * 20;
    } else if (mockStep < 16) {
      state = "PASS_READY";
      result = "PASS";
      liveAdc = 980;
      sampledAdc = 1120;
      canStart = true;
    } else if (mockStep < 20) {
      state = "RUNNING";
      result = "PASS";
      liveAdc = 870;
      sampledAdc = 1120;
      vehicleLocked = false;
      servoAngle = 90;
      retestRemainingMs = (20 - mockStep) * 1000;
      runningSessionMs = (mockStep - 16) * 1000;
    } else if (mockStep < 23) {
      state = "RETEST_REQUIRED";
      result = "RETEST_REQUIRED";
      liveAdc = 930;
      sampledAdc = 1120;
      vehicleLocked = false;
      buzzerOn = mockStep % 2 === 0;
      servoAngle = 90;
      retestRequired = true;
      retestOverdueMs = (mockStep - 20) * 1000;
      retestGraceRemainingMs = Math.max(0, 15000 - retestOverdueMs);
      runningSessionMs = (mockStep - 16) * 1000;
      retestCycleCount = 1;
    } else if (mockStep < 25) {
      state = "RETEST_SAMPLING";
      result = "RETESTING";
      liveAdc = 940;
      sampledAdc = 1120;
      vehicleLocked = false;
      servoAngle = 90;
      retestRequired = true;
      retestOverdueMs = (mockStep - 20) * 1000;
      retestDueToTestMs = 2500;
      runningSessionMs = (mockStep - 16) * 1000;
      retestCycleCount = 1;
    } else {
      state = "FAIL_LOCKED";
      result = "FAIL";
      liveAdc = 2900 + (mockStep - 23) * 90;
      sampledAdc = liveAdc;
      buzzerOn = mockStep % 2 === 0;
    }

    updateTelemetry({
      state,
      result,
      fault: "NONE",
      sensorWarning: "NONE",
      liveAdc,
      sampledAdc,
      threshold: 2000,
      percent: Number(((liveAdc / 4095) * 100).toFixed(1)),
      samplePercent: Number(((sampledAdc / 4095) * 100).toFixed(1)),
      sampleStdDev:
        state === "PASS_READY" || state === "RUNNING" || state === "RETEST_SAMPLING"
          ? 45.2
          : state === "FAIL_LOCKED"
            ? 61.8
            : 0,
      preheatRemainingMs: state === "PREHEAT" ? (5 - mockStep) * 1000 : 0,
      demoMode: true,
      canStart,
      vehicleLocked,
      buzzerOn,
      servoAngle,
      buttonMode: "module_3pin",
      buttonActiveHigh: true,
      buttonBias: "internal_pulldown",
      sensorWarningDurationMs: 0,
      sampleCount:
        state === "PASS_READY" || state === "RUNNING" || state === "RETEST_SAMPLING" || state === "FAIL_LOCKED"
          ? 20
          : 0,
      sampleTotal: 20,
      sampleElapsedMs:
        state === "PASS_READY" || state === "RUNNING" || state === "RETEST_SAMPLING" || state === "FAIL_LOCKED"
          ? 1000
          : 0,
      testToResultMs:
        state === "PASS_READY" || state === "RUNNING" || state === "RETEST_SAMPLING" || state === "FAIL_LOCKED"
          ? 1000
          : 0,
      passReadyToUnlockMs: state === "RUNNING" ? 1800 : 0,
      startToUnlockMs: state === "RUNNING" ? 12 : 0,
      consecutiveFailCount: state === "FAIL_LOCKED" ? 1 : 0,
      retestRequired,
      retestIntervalMs: 30000,
      retestRemainingMs,
      retestOverdueMs,
      retestGraceRemainingMs,
      retestDueToTestMs,
      retestToResultMs,
      runningSessionMs,
      retestCycleCount,
      uptimeMs: mockStep * 1000
    });

    if (mockStep === 5) {
      pushEvent({
        event: "state_changed",
        severity: "info",
        message: "Mock preheat complete"
      });
    }

    if (mockStep === 12) {
      pushEvent({
        event: "sample_result",
        severity: "success",
        message: "Mock PASS result received"
      });
    }

    if (mockStep === 20) {
      pushEvent({
        event: "retest_required",
        severity: "warning",
        message: "Mock periodic retest requested"
      });
    }

    if (mockStep === 23) {
      pushEvent({
        event: "sample_result",
        severity: "warning",
        message: "Mock FAIL result received"
      });
    }
  }, 1000);
}

function monitorDeviceStaleness() {
  setInterval(() => {
    if (mockMode || !store.connection.lastTelemetryAt) {
      return;
    }

    const age = Date.now() - store.connection.lastTelemetryAt;
    if (age <= STALE_DEVICE_MS) {
      return;
    }

    if (!staleEventSent) {
      staleEventSent = true;
      updateConnection({
        status: "stale",
        lastError: `No telemetry for ${Math.round(age / 1000)}s`
      });
      pushEvent({
        event: "device_stale",
        severity: "warning",
        message: "Realtime telemetry is stale. Check ESP32 connection."
      });
    }
  }, 1000);
}

app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

app.get("/api/status", (_req, res) => {
  res.json(buildSnapshot());
});

app.get("/api/ports", async (_req, res) => {
  const ports = await SerialPort.list();
  res.json(ports);
});

app.post("/api/dev/notify-test", (req, res) => {
  pushEvent({
    event: "manual_test",
    severity: req.body?.severity || "info",
    message: req.body?.message || "Manual notification test"
  });
  res.json({ ok: true });
});

io.on("connection", (socket) => {
  socket.emit("snapshot", buildSnapshot());
});

async function printPortsAndExit() {
  const ports = await SerialPort.list();
  if (!ports.length) {
    console.log("No serial ports detected.");
    return;
  }

  ports.forEach((port) => {
    console.log(`${port.path} | ${port.manufacturer || "Unknown"} | ${port.friendlyName || "No friendly name"}`);
  });
}

async function main() {
  if (listPortsOnly) {
    await printPortsAndExit();
    return;
  }

  monitorDeviceStaleness();

  if (mockMode) {
    startMockStream();
  } else {
    connectSerial().catch((error) => {
      updateConnection({
        status: "error",
        lastError: error.message
      });
      pushEvent({
        event: "serial_error",
        severity: "warning",
        message: `Cannot connect to serial port: ${error.message}`
      });
      scheduleReconnect();
    });
  }

  server.listen(DASHBOARD_PORT, () => {
    console.log(`Dashboard ready at http://localhost:${DASHBOARD_PORT}`);
    console.log(mockMode ? "Mode: mock stream" : "Mode: ESP32 serial");
    if (requestedPort) {
      console.log(`Requested serial port: ${requestedPort}`);
    }
  });
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});

const elements = {
  connectionChip: document.getElementById("connectionChip"),
  modeLabel: document.getElementById("modeLabel"),
  portLabel: document.getElementById("portLabel"),
  updatedLabel: document.getElementById("updatedLabel"),
  liveAdcValue: document.getElementById("liveAdcValue"),
  thresholdValue: document.getElementById("thresholdValue"),
  sampledAdcValue: document.getElementById("sampledAdcValue"),
  percentLabel: document.getElementById("percentLabel"),
  gaugeFill: document.getElementById("gaugeFill"),
  riskPill: document.getElementById("riskPill"),
  stateValue: document.getElementById("stateValue"),
  stateDescription: document.getElementById("stateDescription"),
  resultValue: document.getElementById("resultValue"),
  servoValue: document.getElementById("servoValue"),
  lockValue: document.getElementById("lockValue"),
  buzzerValue: document.getElementById("buzzerValue"),
  startValue: document.getElementById("startValue"),
  preheatValue: document.getElementById("preheatValue"),
  eventsList: document.getElementById("eventsList"),
  rawLines: document.getElementById("rawLines"),
  notificationsBtn: document.getElementById("notificationsBtn"),
  notifyTestBtn: document.getElementById("notifyTestBtn"),
  historyChart: document.getElementById("historyChart")
};

const socket = io();
const history = [];
const state = {
  snapshotLoaded: false,
  notificationsEnabled: false,
  connection: null,
  telemetry: null,
  events: [],
  rawLines: []
};

elements.notificationsBtn.addEventListener("click", requestNotifications);
elements.notifyTestBtn.addEventListener("click", triggerNotificationTest);

fetch("/api/status")
  .then((response) => response.json())
  .then((snapshot) => {
    renderSnapshot(snapshot);
    state.snapshotLoaded = true;
  })
  .catch((error) => {
    console.error(error);
  });

socket.on("snapshot", (snapshot) => {
  renderSnapshot(snapshot);
  state.snapshotLoaded = true;
});

socket.on("connection", (connection) => {
  state.connection = connection;
  renderConnection();
});

socket.on("telemetry", (telemetry) => {
  state.telemetry = telemetry;
  pushHistory(telemetry.liveAdc);
  renderTelemetry();
});

socket.on("event", (event) => {
  state.events.unshift(event);
  state.events = state.events.slice(0, 60);
  renderEvents();
  maybeNotify(event);
});

socket.on("raw-line", (line) => {
  state.rawLines.unshift(line);
  state.rawLines = state.rawLines.slice(0, 40);
  renderRawLines();
});

function renderSnapshot(snapshot) {
  state.connection = snapshot.connection;
  state.telemetry = snapshot.telemetry;
  state.events = snapshot.events || [];
  state.rawLines = snapshot.rawLines || [];

  if (snapshot.telemetry) {
    pushHistory(snapshot.telemetry.liveAdc);
  }

  renderConnection();
  renderTelemetry();
  renderEvents();
  renderRawLines();
}

function renderConnection() {
  const connection = state.connection || {};
  const status = connection.status || "unknown";
  const mode = connection.mode || "--";
  const portName = connection.portName || "--";
  const updatedAt = state.telemetry?.updatedAt || "--";

  elements.connectionChip.textContent = describeConnection(status, connection.lastError);
  elements.connectionChip.className = `status-chip ${status}`;
  elements.modeLabel.textContent = `Mode: ${mode}`;
  elements.portLabel.textContent = `Port: ${portName}`;
  elements.updatedLabel.textContent = `Cập nhật: ${formatDate(updatedAt)}`;
}

function renderTelemetry() {
  const telemetry = state.telemetry;
  if (!telemetry) {
    return;
  }

  elements.liveAdcValue.textContent = telemetry.liveAdc ?? 0;
  elements.thresholdValue.textContent = telemetry.threshold ?? 0;
  elements.sampledAdcValue.textContent = telemetry.sampledAdc ?? 0;
  elements.percentLabel.textContent = `${Number(telemetry.percent ?? 0).toFixed(1)}%`;
  elements.gaugeFill.style.width = `${Math.max(0, Math.min(100, ((telemetry.liveAdc || 0) / 4095) * 100))}%`;
  elements.stateValue.textContent = telemetry.state || "UNKNOWN";
  elements.stateDescription.textContent = describeState(telemetry);
  elements.resultValue.textContent = telemetry.result || "--";
  elements.servoValue.textContent = `${telemetry.servoAngle ?? 0}°`;
  elements.lockValue.textContent = telemetry.vehicleLocked ? "Đang khóa" : "Đã mở";
  elements.buzzerValue.textContent = telemetry.buzzerOn ? "ON" : "OFF";
  elements.startValue.textContent = telemetry.canStart ? "Cho phép" : "Chưa cho phép";
  elements.preheatValue.textContent = formatTimerValue(telemetry);
  if (telemetry.retestRequired) {
    elements.startValue.textContent = "TEST lại";
  }
  const riskLevel = computeRiskLevel(telemetry);
  elements.riskPill.textContent = computeRiskLabel(telemetry);
  elements.riskPill.className = `risk-pill ${riskLevel}`;

  drawHistoryChart();
}

function renderEvents() {
  const items = state.events.slice(0, 8);
  if (!items.length) {
    elements.eventsList.innerHTML = `<li class="event-item"><div class="event-copy">Chưa có sự kiện nào.</div></li>`;
    return;
  }

  elements.eventsList.innerHTML = items
    .map(
      (event) => `
        <li class="event-item ${event.severity}">
          <div class="event-meta">
            <span>${event.event}</span>
            <span>${formatDate(event.timestamp)}</span>
          </div>
          <div class="event-title">${event.state || "DEVICE"}</div>
          <div class="event-copy">${escapeHtml(event.message || "")}</div>
        </li>
      `
    )
    .join("");
}

function renderRawLines() {
  if (!state.rawLines.length) {
    elements.rawLines.textContent = "Chưa có dữ liệu serial.";
    return;
  }

  elements.rawLines.textContent = state.rawLines
    .slice(0, 14)
    .map((line) => `[${formatDate(line.timestamp)}] ${line.line}`)
    .join("\n");
}

function describeConnection(status, error) {
  switch (status) {
    case "online":
      return "ESP32 đang gửi dữ liệu";
    case "connected":
      return "Đã mở cổng serial";
    case "connecting":
      return "Đang kết nối ESP32";
    case "waiting":
      return error || "Đang chờ ESP32";
    case "stale":
      return error || "Mất heartbeat từ ESP32";
    case "error":
      return error || "Lỗi kết nối";
    case "mock":
      return "Đang chạy mock realtime";
    case "disconnected":
      return "Serial đã ngắt";
    default:
      return "Đang khởi tạo";
  }
}

function describeState(telemetry) {
  const map = {
    PREHEAT: "Hệ thống đang làm nóng cảm biến trước khi cho phép đo.",
    STANDBY_LOCKED: "Thiết bị đang chờ người dùng nhấn TEST để đo lại nồng độ cồn.",
    SAMPLING: "ESP32 đang lấy nhiều mẫu ADC để tính trung bình và giảm nhiễu.",
    PASS_READY: "Kết quả an toàn. Người dùng có thể nhấn START để mở khóa.",
    FAIL_LOCKED: "Phát hiện vượt ngưỡng. Xe tiếp tục bị khóa và còi cảnh báo có thể đang bật.",
    RUNNING: "Servo đã mở khóa và demo đang ở trạng thái vận hành.",
    RETEST_REQUIRED: "Periodic retest is due. Vehicle remains running, but the user must press TEST again.",
    RETEST_SAMPLING: "Periodic retest sampling is running while the vehicle remains unlocked.",
    ERROR_LOCKED: "System entered safe-lock because of a critical fault such as OLED init failure, sensor timeout, or retest timeout."
  };

  return map[telemetry.state] || "Chưa có mô tả trạng thái.";
}

function computeRiskLevel(telemetry) {
  if (
    (telemetry.result || "").toUpperCase() === "FAIL" ||
    telemetry.state === "ERROR_LOCKED" ||
    telemetry.state === "RETEST_REQUIRED" ||
    telemetry.state === "RETEST_SAMPLING" ||
    telemetry.retestRequired ||
    telemetry.liveAdc >= telemetry.threshold ||
    (telemetry.sensorWarning || "") !== "NONE"
  ) {
    return "high";
  }
  if ((telemetry.result || "").toUpperCase() === "PASS") {
    return "pass";
  }
  return "safe";
}

function computeRiskLabel(telemetry) {
  if (telemetry.retestRequired || telemetry.state === "RETEST_REQUIRED" || telemetry.state === "RETEST_SAMPLING") {
    return "RETEST";
  }

  const riskLevel = computeRiskLevel(telemetry);
  if (riskLevel === "high") {
    return "HIGH";
  }
  if (riskLevel === "pass") {
    return "PASS";
  }
  return "SAFE";
}

function formatTimerValue(telemetry) {
  if (telemetry.state === "PREHEAT") {
    return `${Math.ceil((telemetry.preheatRemainingMs || 0) / 1000)}s`;
  }

  if (telemetry.state === "RUNNING") {
    return `Retest ${Math.ceil((telemetry.retestRemainingMs || 0) / 1000)}s`;
  }

  if (telemetry.state === "RETEST_REQUIRED") {
    return `Grace ${Math.ceil((telemetry.retestGraceRemainingMs || 0) / 1000)}s`;
  }

  if (telemetry.state === "RETEST_SAMPLING") {
    return `Sample ${Math.ceil((telemetry.sampleElapsedMs || 0) / 1000)}s`;
  }

  return "0s";
}

function formatDate(value) {
  if (!value || value === "--") {
    return "--";
  }

  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }

  return date.toLocaleTimeString("vi-VN", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });
}

function pushHistory(value) {
  if (typeof value !== "number") {
    return;
  }

  history.push(value);
  if (history.length > 80) {
    history.shift();
  }
}

function drawHistoryChart() {
  const canvas = elements.historyChart;
  const ctx = canvas.getContext("2d");
  const width = canvas.width;
  const height = canvas.height;
  const padding = 24;

  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = "rgba(7, 9, 12, 0.92)";
  ctx.fillRect(0, 0, width, height);

  ctx.strokeStyle = "rgba(255,255,255,0.08)";
  ctx.lineWidth = 1;
  for (let i = 0; i < 4; i += 1) {
    const y = padding + (i * (height - padding * 2)) / 3;
    ctx.beginPath();
    ctx.moveTo(padding, y);
    ctx.lineTo(width - padding, y);
    ctx.stroke();
  }

  const threshold = state.telemetry?.threshold ?? 2000;
  const thresholdY = height - padding - (threshold / 4095) * (height - padding * 2);
  ctx.strokeStyle = "rgba(255, 94, 91, 0.5)";
  ctx.setLineDash([8, 6]);
  ctx.beginPath();
  ctx.moveTo(padding, thresholdY);
  ctx.lineTo(width - padding, thresholdY);
  ctx.stroke();
  ctx.setLineDash([]);

  ctx.fillStyle = "rgba(255, 209, 102, 0.8)";
  ctx.font = '12px "Bahnschrift", "Trebuchet MS", sans-serif';
  ctx.fillText(`Threshold ${threshold}`, width - padding - 90, thresholdY - 8);

  if (!history.length) {
    return;
  }

  const stepX = (width - padding * 2) / Math.max(1, history.length - 1);
  ctx.strokeStyle = "#ffb347";
  ctx.lineWidth = 3;
  ctx.beginPath();

  history.forEach((value, index) => {
    const x = padding + index * stepX;
    const y = height - padding - (value / 4095) * (height - padding * 2);
    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });

  ctx.stroke();
}

async function requestNotifications() {
  if (!("Notification" in window)) {
    elements.notificationsBtn.textContent = "Trình duyệt không hỗ trợ";
    return;
  }

  const permission = await Notification.requestPermission();
  state.notificationsEnabled = permission === "granted";
  elements.notificationsBtn.textContent = state.notificationsEnabled ? "Đã bật thông báo" : "Thông báo bị chặn";
}

async function triggerNotificationTest() {
  await fetch("/api/dev/notify-test", {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify({
      severity: "info",
      message: "Dashboard notification test"
    })
  });
}

function maybeNotify(event) {
  if (!state.snapshotLoaded || !state.notificationsEnabled || !("Notification" in window)) {
    return;
  }

  const shouldNotify = ["warning", "critical", "error", "success"].includes(event.severity) ||
    event.event === "sample_result";
  if (!shouldNotify) {
    return;
  }

  const title = `${event.event} • ${event.state || "Alcohol Interlock"}`;
  const body = event.message || "Có cập nhật mới từ thiết bị.";
  new Notification(title, {
    body,
    silent: false
  });
}

function escapeHtml(value) {
  return value
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");
}

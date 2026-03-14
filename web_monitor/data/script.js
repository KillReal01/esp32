const authTokenInput = document.getElementById('authToken');
const authHint = document.getElementById('authHint');
const notice = document.getElementById('notice');

const sysFirmware = document.getElementById('sysFirmware');
const sysUptime = document.getElementById('sysUptime');
const sysHeap = document.getElementById('sysHeap');
const sysFlash = document.getElementById('sysFlash');
const sysHealth = document.getElementById('sysHealth');
const sysUpdated = document.getElementById('sysUpdated');
const logsOutput = document.getElementById('logsOutput');
const scanBtn = document.getElementById('scanBtn');
const scanSpinner = document.getElementById('scanSpinner');
const lastScan = document.getElementById('lastScan');
const bleScanBtn = document.getElementById('bleScanBtn');
const bleScanSpinner = document.getElementById('bleScanSpinner');
const bleLastScan = document.getElementById('bleLastScan');

let scannedNetworks = [];
let sortState = { field: 'rssi', dir: 'desc' };
let bleDevices = [];
let bleSortState = { field: 'rssi', dir: 'desc' };

const tokenRegex = /^[A-Za-z0-9_-]{12,64}$/;
authTokenInput.value = localStorage.getItem('esp32_token') || '';

const setHint = (message) => authHint.textContent = message;
const notify = (message, type = '') => {
  notice.textContent = message;
  const typeClass = type ? ` ${type}` : '';
  notice.className = `notice${typeClass} show`;
};

const request = async (path, options = {}) => {
  const token = authTokenInput.value.trim();
  const headers = { ...(options.headers || {}) };
  if (token) headers.Authorization = `Bearer ${token}`;

  const response = await fetch(path, { ...options, headers });
  if (response.status === 401) {
    setHint('Ошибка 401: токен не прошел проверку.');
    notify('Токен невалиден или отсутствует. Проверьте токен и повторите запрос.', 'error');
  }
  return response;
};

const renderEmpty = (selector, message, colspan) => {
  document.querySelector(`${selector} tbody`).innerHTML = `<tr><td colspan="${colspan}" class="empty">${message}</td></tr>`;
};

const sortData = () => {
  const { field, dir } = sortState;
  const m = dir === 'asc' ? 1 : -1;
  scannedNetworks.sort((a, b) => {
    const av = a[field] ?? '';
    const bv = b[field] ?? '';
    if (typeof av === 'number' && typeof bv === 'number') return (av - bv) * m;
    return String(av).localeCompare(String(bv)) * m;
  });
};

const renderScanTable = () => {
  const tbody = document.querySelector('#scanTable tbody');
  tbody.innerHTML = '';

  if (!scannedNetworks.length) return renderEmpty('#scanTable', 'Сети не найдены', 7);

  for (const item of scannedNetworks) {
    const row = document.createElement('tr');
    row.innerHTML = `
      <td>${item.ssid || '(hidden)'}</td>
      <td>${item.bssid || '-'}</td>
      <td>${item.rssi ?? '-'}</td>
      <td>${item.chan ?? '-'}</td>
      <td>${item.auth || '-'}</td>
      <td>${item.hidden ? 'yes' : 'no'}</td>
      <td><button class="small connect-btn" data-ssid="${item.ssid || ''}">Подключиться</button></td>`;
    tbody.appendChild(row);
  }

  document.querySelectorAll('.connect-btn').forEach((btn) => {
    btn.onclick = async () => {
      const ssid = btn.dataset.ssid;
      const password = prompt(`Пароль для ${ssid}`, '');
      if (password === null) return;

      const r = await request('/api/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, password }),
      });
      if (!r.ok) return;
      const data = await r.json();
      alert(data.status === 'connecting' ? 'Подключение запущено' : 'Ошибка подключения');
    };
  });
};

const sortBleData = () => {
  const { field, dir } = bleSortState;
  const m = dir === 'asc' ? 1 : -1;
  bleDevices.sort((a, b) => {
    const av = a[field] ?? '';
    const bv = b[field] ?? '';
    if (typeof av === 'number' && typeof bv === 'number') return (av - bv) * m;
    return String(av).localeCompare(String(bv)) * m;
  });
};

const renderBleTable = () => {
  const tbody = document.querySelector('#bleTable tbody');
  tbody.innerHTML = '';

  if (!bleDevices.length) return renderEmpty('#bleTable', 'Устройства не найдены', 5);

  for (const item of bleDevices) {
    const row = document.createElement('tr');
    row.innerHTML = `
      <td>${item.name || '-'}</td>
      <td>${item.addr || '-'}</td>
      <td>${item.rssi ?? '-'}</td>
      <td>${item.type ?? '-'}</td>
      <td>${item.scanRsp ? 'yes' : 'no'}</td>`;
    tbody.appendChild(row);
  }
};

const renderSysinfo = (data) => {
  sysFirmware.textContent = data.firmware || '—';
  sysUptime.textContent = data.uptimeSec != null ? `${data.uptimeSec} сек` : '—';
  sysHeap.textContent = data.freeHeapKb != null ? `${data.freeHeapKb} KB` : '—';
  sysFlash.textContent = data.flashKb != null ? `${data.flashKb} KB` : '—';
  sysHealth.textContent = data.healthScore != null ? `${data.healthScore}` : '—';
  sysUpdated.textContent = new Date().toLocaleTimeString();
};

const setScanLoading = (isLoading) => {
  scanBtn.disabled = isLoading;
  scanBtn.classList.toggle('loading', isLoading);
  scanSpinner.classList.toggle('active', isLoading);
  lastScan.textContent = isLoading ? 'сканирование…' : lastScan.textContent;
};

const setBleScanLoading = (isLoading) => {
  bleScanBtn.disabled = isLoading;
  bleScanBtn.classList.toggle('loading', isLoading);
  bleScanSpinner.classList.toggle('active', isLoading);
  bleLastScan.textContent = isLoading ? 'сканирование…' : bleLastScan.textContent;
};

const escapeHtml = (value) =>
  value
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#39;');

const formatLogs = (text) => {
  if (!text.trim()) return '—';
  const lines = text.split('\n');
  return lines
    .map((line) => {
      if (!line) return '';
      let cls = 'log-info';
      if (/^E\s/.test(line)) cls = 'log-error';
      else if (/^W\s/.test(line)) cls = 'log-warn';
      else if (/^D\s/.test(line)) cls = 'log-debug';
      return `<span class="log-line ${cls}">${escapeHtml(line)}</span>`;
    })
    .join('');
};

const loadSysinfo = async () => {
  const r = await request('/api/sysinfo');
  if (!r.ok) return;
  const data = await r.json();
  renderSysinfo(data || {});
};

const loadLogs = async () => {
  const r = await request('/api/logs');
  if (!r.ok) return;
  const text = await r.text();
  logsOutput.innerHTML = formatLogs(text || '');
};

const doScan = async () => {
  setScanLoading(true);
  const r = await request('/api/scan');
  if (!r.ok) {
    setScanLoading(false);
    lastScan.textContent = 'ошибка';
    return;
  }
  scannedNetworks = await r.json();
  sortData();
  renderScanTable();
  lastScan.textContent = new Date().toLocaleTimeString();
  setScanLoading(false);
};

scanBtn.onclick = doScan;

const doBleScan = async () => {
  setBleScanLoading(true);
  const r = await request('/api/ble/scan');
  if (!r.ok) {
    setBleScanLoading(false);
    bleLastScan.textContent = 'ошибка';
    return;
  }
  bleDevices = await r.json();
  sortBleData();
  renderBleTable();
  bleLastScan.textContent = new Date().toLocaleTimeString();
  setBleScanLoading(false);
};

bleScanBtn.onclick = doBleScan;
document.getElementById('refreshSysinfo').onclick = loadSysinfo;
document.getElementById('refreshLogs').onclick = loadLogs;
document.getElementById('rebootBtn').onclick = async () => {
  const ok = confirm('Перезагрузить устройство сейчас?');
  if (!ok) return;
  const r = await request('/api/reboot', { method: 'POST' });
  if (!r.ok) return;
  const data = await r.json();
  notify(data.status === 'ok' ? 'Устройство перезагружается.' : 'Ошибка при перезагрузке.', data.status === 'ok' ? 'ok' : 'error');
};

document.querySelectorAll('#scanTable th[data-sort]').forEach((th) => {
  th.onclick = () => {
    const field = th.dataset.sort;
    sortState = {
      field,
      dir: sortState.field === field && sortState.dir === 'asc' ? 'desc' : 'asc',
    };
    sortData();
    renderScanTable();
  };
});

document.querySelectorAll('#bleTable th[data-sort]').forEach((th) => {
  th.onclick = () => {
    const field = th.dataset.sort;
    bleSortState = {
      field,
      dir: bleSortState.field === field && bleSortState.dir === 'asc' ? 'desc' : 'asc',
    };
    sortBleData();
    renderBleTable();
  };
});

document.getElementById('saveToken').onclick = () => {
  const token = authTokenInput.value.trim();
  if (!tokenRegex.test(token)) {
    setHint('Формат токена неверный. Разрешены A-Za-z0-9_- длиной 12..64.');
    return;
  }
  localStorage.setItem('esp32_token', token);
  setHint('Токен сохранен.');
};

document.getElementById('validateToken').onclick = async () => {
  const token = authTokenInput.value.trim();
  const r = await fetch('/api/token/validate', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ token }),
  });

  const data = await r.json();
  if (data.valid) {
    setHint('Токен валиден.');
    notify('Токен прошел проверку.', 'ok');
  } else {
    setHint(`Токен отклонен: ${data.reason}`);
    notify(`Токен отклонен: ${data.reason}`, 'error');
  }
};

document.getElementById('loadApClients').onclick = async () => {
  const bssid = document.getElementById('bssidInput').value.trim();
  const r = await request(`/api/ap-clients?bssid=${encodeURIComponent(bssid)}`);
  if (!r.ok) return;
  const data = await r.json();
  const tbody = document.querySelector('#clientsTable tbody');
  tbody.innerHTML = '';

  if (!data.stations || !data.stations.length) return renderEmpty('#clientsTable', 'Нет клиентов', 2);

  data.stations.forEach((s) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `<td>${s.mac}</td><td>${s.aid}</td>`;
    tbody.appendChild(tr);
  });
};

loadSysinfo();
loadLogs();

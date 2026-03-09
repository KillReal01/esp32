const authTokenInput = document.getElementById('authToken');
const authHint = document.getElementById('authHint');

let scannedNetworks = [];
let sortState = { field: 'rssi', dir: 'desc' };

const tokenRegex = /^[A-Za-z0-9_-]{12,64}$/;
authTokenInput.value = localStorage.getItem('esp32_token') || '';

const setHint = (message) => authHint.textContent = message;

const request = async (path, options = {}) => {
  const token = authTokenInput.value.trim();
  const headers = { ...(options.headers || {}) };
  if (token) headers.Authorization = `Bearer ${token}`;

  const response = await fetch(path, { ...options, headers });
  if (response.status === 401) setHint('Ошибка 401: токен не прошел проверку.');
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
      const data = await r.json();
      alert(data.status === 'connecting' ? 'Подключение запущено' : 'Ошибка подключения');
    };
  });
};

const doScan = async () => {
  document.getElementById('lastScan').textContent = '...';
  const r = await request('/api/scan');
  if (!r.ok) return;

  scannedNetworks = await r.json();
  sortData();
  renderScanTable();
  document.getElementById('lastScan').textContent = new Date().toLocaleTimeString();
};

document.getElementById('scanBtn').onclick = doScan;

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
  setHint(data.valid ? 'Токен валиден.' : `Токен отклонен: ${data.reason}`);
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

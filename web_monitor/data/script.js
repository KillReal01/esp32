const authTokenInput = document.getElementById('authToken');
const authHint = document.getElementById('authHint');

const tokenFromStorage = localStorage.getItem('esp32_token') || '';
authTokenInput.value = tokenFromStorage;

const setHint = (message) => {
  authHint.textContent = message;
};

document.getElementById('saveToken').onclick = () => {
  localStorage.setItem('esp32_token', authTokenInput.value.trim());
  setHint('Токен сохранён в локальном хранилище браузера.');
};

document.querySelectorAll('button.tab').forEach((btn) => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('button.tab').forEach((tab) => tab.classList.remove('active'));
    document.querySelectorAll('.panel').forEach((panel) => panel.classList.add('hidden'));
    btn.classList.add('active');
    document.getElementById(btn.dataset.tab).classList.remove('hidden');
  });
});

const request = async (path, options = {}) => {
  const token = authTokenInput.value.trim();
  const headers = {
    ...(options.headers || {}),
  };

  if (token) {
    headers.Authorization = `Bearer ${token}`;
  }

  const response = await fetch(path, { ...options, headers });
  if (response.status === 401) {
    setHint('Ошибка 401: введите корректный токен авторизации.');
    return null;
  }

  return response;
};

const getJson = async (path) => {
  try {
    const response = await request(path);
    if (!response || !response.ok) return null;
    return await response.json();
  } catch {
    return null;
  }
};

function renderEmpty(tableSelector, message, colspan) {
  const tbody = document.querySelector(`${tableSelector} tbody`);
  tbody.innerHTML = `<tr><td colspan="${colspan}" class="empty">${message}</td></tr>`;
}

async function doScan() {
  document.getElementById('lastScan').textContent = '...';
  const data = await getJson('/api/scan');
  const tbody = document.querySelector('#scanTable tbody');
  tbody.innerHTML = '';

  if (!data || !data.length) {
    renderEmpty('#scanTable', 'Сети не найдены', 3);
  } else {
    data.sort((a, b) => (b.rssi || 0) - (a.rssi || 0));
    for (const item of data) {
      const row = document.createElement('tr');
      row.innerHTML = `<td>${item.ssid || '-'}</td><td>${item.rssi ?? '-'}</td><td>${item.chan ?? '-'}</td>`;
      tbody.appendChild(row);
    }
  }

  document.getElementById('lastScan').textContent = new Date().toLocaleTimeString();
}

document.getElementById('scanBtn').onclick = doScan;

document.getElementById('refreshClients').onclick = async () => {
  const data = await getJson('/api/stations');
  document.getElementById('clientsCount').textContent = data?.length || 0;

  const tbody = document.querySelector('#clientsTable tbody');
  tbody.innerHTML = '';

  if (!data || !data.length) {
    renderEmpty('#clientsTable', 'Нет подключённых клиентов', 3);
    return;
  }

  for (const item of data) {
    const row = document.createElement('tr');
    row.innerHTML = `<td>${item.mac || '-'}</td><td>${item.ip || '-'}</td><td>${item.last || '-'}</td>`;
    tbody.appendChild(row);
  }
};

document.getElementById('loadLogs').onclick = async () => {
  const response = await request('/api/logs');
  const logs = response && response.ok ? await response.text() : 'Логи недоступны.';
  document.getElementById('logsContent').textContent = logs;
  document.getElementById('logsSize').textContent = `${new Blob([logs]).size} B`;
};

document.getElementById('downloadLogs').onclick = () => {
  const text = document.getElementById('logsContent').textContent || '';
  const blob = new Blob([text], { type: 'text/plain' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'esp32.log';
  document.body.appendChild(a);
  a.click();
  URL.revokeObjectURL(url);
  a.remove();
};

document.getElementById('refreshSys').onclick = async () => {
  const data = await getJson('/api/sysinfo');
  if (!data) return;

  document.getElementById('fwVer').textContent = data.firmware || '-';
  document.getElementById('uptime').textContent = data.uptime || '-';
  document.getElementById('freeHeap').textContent = data.freeHeap || '-';
  document.getElementById('flashInfo').textContent = data.flash || '-';
};

document.getElementById('rebootBtn').onclick = async () => {
  if (!confirm('Перезагрузить устройство?')) return;

  await request('/api/reboot', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: '{}',
  });

  alert('Команда перезагрузки отправлена.');
};

window.addEventListener('load', () => {
  document.getElementById('refreshSys').click();
});

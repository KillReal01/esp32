// --- tabs
document.querySelectorAll('button.tab').forEach(btn=>{
  btn.addEventListener('click', ()=>{
    document.querySelectorAll('button.tab').forEach(b=>b.classList.remove('active'));
    btn.classList.add('active');
    document.querySelectorAll('.panel').forEach(p=>p.style.display='none');
    document.getElementById(btn.dataset.tab).style.display='block';
  });
});

// --- API wrapper
const API = {
  async get(path){ 
    try{
      const r = await fetch(path);
      if(!r.ok) throw new Error('HTTP error '+r.status);
      return r.json();
    }catch(e){
      console.warn('API GET failed:', path, e);
      return null;
    } 
  },
  async post(path, body){ 
    try{
      const r = await fetch(path, {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify(body)
      });
      return r.ok;
    }catch(e){
      console.warn('API POST failed:', path, e);
      return false;
    } 
  }
};

// --- scan
const scanBtn = document.getElementById('scanBtn');
const lastScan = document.getElementById('lastScan');

async function doScan(){ 
  lastScan.textContent='...';
  const data = await API.get('/api/scan');
  renderScan(data || []);
  lastScan.textContent = new Date().toLocaleTimeString();
}
scanBtn.onclick = doScan;

function renderScan(list){ 
  const tbody = document.querySelector('#scanTable tbody'); 
  tbody.innerHTML = '';
  if(!list.length){ 
    tbody.innerHTML = '<tr><td colspan="3" class="empty">Нет сетей</td></tr>'; 
    return;
  }
  list.sort((a,b)=>(b.rssi||0)-(a.rssi||0));
  for(const s of list){ 
    const tr = document.createElement('tr'); 
    tr.innerHTML = `<td>${s.ssid}</td><td>${s.rssi}</td><td>${s.chan}</td>`; 
    tbody.appendChild(tr); 
  }
}

// --- clients
document.getElementById('refreshClients').onclick = async ()=>{
  const data = await API.get('/api/stations');
  renderClients(data || []);
};
function renderClients(list){ 
  document.getElementById('clientsCount').textContent = list.length;
  const tbody = document.querySelector('#clientsTable tbody'); 
  tbody.innerHTML = '';
  if(!list.length){ 
    tbody.innerHTML = '<tr><td colspan="3" class="empty">Нет клиентов</td></tr>'; 
    return;
  }
  for(const c of list){ 
    const tr = document.createElement('tr'); 
    tr.innerHTML = `<td>${c.mac}</td><td>${c.ip}</td><td>${c.last}</td>`; 
    tbody.appendChild(tr); 
  }
}

// --- logs
const logsContent = document.getElementById('logsContent');
document.getElementById('loadLogs').onclick = async ()=>{
  const data = await fetch('/api/logs')
    .then(r=>r.ok ? r.text() : null)
    .catch(()=>null);
  logsContent.textContent = data || 'Логи недоступны';
  document.getElementById('logsSize').textContent = new Blob([logsContent.textContent]).size+' B';
};

document.getElementById('downloadLogs').onclick = ()=>{
  const text = logsContent.textContent || '';
  const blob = new Blob([text], {type:'text/plain'});
  const a = document.createElement('a'); 
  a.href = URL.createObjectURL(blob); 
  a.download = 'system.log'; 
  document.body.appendChild(a); 
  a.click(); 
  a.remove();
};

// --- system
document.getElementById('refreshSys').onclick = async ()=>{
  const data = await API.get('/api/sysinfo');
  if(!data) return;
  document.getElementById('fwVer').textContent = data.firmware;
  document.getElementById('uptime').textContent = data.uptime;
  document.getElementById('freeHeap').textContent = data.freeHeap;
  document.getElementById('flashInfo').textContent = data.flash;
};

// document.getElementById('rebootBtn').onclick = async ()=>{
//   if(!confirm('Перезагрузить устройство?')) return;
//   const ok = await API.post('/api/reboot', {});
//   alert(ok ? 'Reboot command sent' : 'Reboot failed');
// };

document.getElementById('rebootBtn').onclick = () => {
    if (!confirm('Перезагрузить устройство?')) return;
    fetch('/api/reboot', { method: 'POST', body: '{}' })
        .catch(e => console.log('Failed to send reboot command', e));
    alert('Reboot command sent');
};


// --- initial load
(async()=>{
  // await doScan(); 
  // document.getElementById('refreshClients').click(); 
  document.getElementById('refreshSys').click(); 
})();

let ws;
let lastHeartbeat = Date.now();
let sentinelTimer = null;

function connectWebSocket() {
    ws = new WebSocket(`ws://${window.location.hostname}:8080`);

    ws.onopen = () => {
        const status = document.getElementById('conn-status');
        status.textContent = 'Connected';
        status.classList.add('connected');
        startSentinel();
    };

    ws.onmessage = (event) => {
        const packet = JSON.parse(event.data);
        
        if (packet.type === 'HEARTBEAT') {
            lastHeartbeat = Date.now();
            const data = packet.payload;
            
            // Safely update UI elements if the data exists in the payload
            if (data.os_uptime !== undefined) {
                document.getElementById('os-runtime').textContent = data.os_uptime + 's';
            }
            if (data.daemon_uptime !== undefined) {
                document.getElementById('app-runtime').textContent = data.daemon_uptime + 's';
            }
            if (data.cpu_temp !== undefined) {
                document.getElementById('cpu-temp').textContent = data.cpu_temp.toFixed(1) + '°C';
            }
        } 
        else if (packet.type === 'COMMAND_RESPONSE') {
            if (window.handleCommandResponse) {
                window.handleCommandResponse(packet.payload);
            }
        }
    };

    ws.onclose = () => {
        handleDisconnect();
        setTimeout(connectWebSocket, 2000); // Attempt reconnect every 2s
    };

    ws.onerror = (error) => {
        console.error('WebSocket Error:', error);
    };
}

function handleDisconnect() {
    const status = document.getElementById('conn-status');
    status.textContent = 'Disconnected';
    status.classList.remove('connected');
    
    // Clear out UI values so stale data isn't displayed
    document.getElementById('os-runtime').textContent = '--s';
    document.getElementById('app-runtime').textContent = '--s';
    document.getElementById('cpu-temp').textContent = '--°C';
}

function startSentinel() {
    if (sentinelTimer) clearInterval(sentinelTimer);
    
    // Check every 1s, trigger if no heartbeat for 3s
    sentinelTimer = setInterval(() => {
        const now = Date.now();
        if (now - lastHeartbeat > 3000) { 
            console.warn("Sentinel Watchdog: Connection lost (Timeout threshold reached).");
            handleDisconnect();
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.close();
            }
        }
    }, 1000);
}
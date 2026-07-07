let ws;
let lastHeartbeat = Date.now();
let sentinelTimer = null;

function connectWebSocket() 
{
    ws = new WebSocket(`ws://${window.location.hostname}:8080`);

    ws.onopen = () => 
    {
        const status = document.getElementById('conn-status');
        status.textContent = 'Connected';
        status.classList.add('connected');
        startSentinel();
    };

    ws.onmessage = (event) => 
    {
        const packet = JSON.parse(event.data);
        
        if (packet.type === 'HEARTBEAT') 
        {
            lastHeartbeat = Date.now();
            const payload = packet.payload;
            
            if (payload.os_uptime !== undefined) 
            {
                document.getElementById('os-runtime').textContent = payload.os_uptime + 's';
            }
            if (payload.daemon_uptime !== undefined) 
            {
                document.getElementById('app-runtime').textContent = payload.daemon_uptime + 's';
            }
            if (payload.cpu_temp !== undefined) 
            {
                document.getElementById('cpu-temp').textContent = payload.cpu_temp.toFixed(1) + '°C';
            }
            if (payload.load_avg !== undefined) 
            {
                document.getElementById('load-avg').textContent = payload.load_avg.toFixed(2);
            }
            if (payload.ram_usage !== undefined) 
            {
                document.getElementById('ram-usage').textContent = payload.ram_usage + '%';
            }

            // Route live telemetry data to the chart workspace if the modal is currently open
            if (window.activeMetricType && window.updateActiveChart) 
            {
                if (window.activeMetricType === 'cpu' && payload.cpu_temp !== undefined) 
                {
                    window.updateActiveChart(payload.cpu_temp);
                }
                else if (window.activeMetricType === 'ram' && payload.ram_usage !== undefined) 
                {
                    window.updateActiveChart(payload.ram_usage);
                }
            }
        } 
        else if (packet.type === 'COMMAND_RESPONSE') 
        {
            if (window.handleCommandResponse) 
            {
                window.handleCommandResponse(packet.payload);
            }
        }
    };

    ws.onclose = () => 
    {
        handleDisconnect();
        setTimeout(connectWebSocket, 2000);
    };

    ws.onerror = (error) => 
    {
        console.error('WebSocket Error:', error);
    };
}

function handleDisconnect() 
{
    const status = document.getElementById('conn-status');
    status.textContent = 'Disconnected';
    status.classList.remove('connected');
    
    document.getElementById('os-runtime').textContent = '--s';
    document.getElementById('app-runtime').textContent = '--s';
    document.getElementById('cpu-temp').textContent = '--°C';
    document.getElementById('load-avg').textContent = '--';
    document.getElementById('ram-usage').textContent = '--%';
}

function startSentinel() 
{
    if (sentinelTimer) 
    {
        clearInterval(sentinelTimer);
    }
    
    sentinelTimer = setInterval(() => 
    {
        const now = Date.now();
        if (now - lastHeartbeat > 3000) 
        { 
            console.warn("Sentinel Watchdog: Connection lost (Timeout threshold reached).");
            handleDisconnect();
            if (ws && ws.readyState === WebSocket.OPEN) 
            {
                ws.close();
            }
        }
    }, 1000);
}
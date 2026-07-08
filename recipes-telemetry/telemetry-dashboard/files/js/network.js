let ws;
let lastHeartbeat = 0;
let sentinelTimer;

function connectWebSocket()
{
    ws = new WebSocket(`ws://${window.location.hostname}:8080`);

    ws.onopen = () =>
    {
        const status = document.getElementById('conn-status');
        status.textContent = 'Connected';
        status.classList.add('connected');
        
        const helloPacket = 
        { 
            type: 'CLIENT_HELLO', 
            timestamp: Date.now(), 
            payload: 
            {
            } 
        };
        
        ws.send(JSON.stringify(helloPacket));

        lastHeartbeat = Date.now();
        startSentinel();
    };

    ws.onmessage = (event) =>
    {
        try
        {
            const data = JSON.parse(event.data);
            
            switch(data.type)
            {
                case 'HEARTBEAT':
                {
                    lastHeartbeat = Date.now();
                    if (data.payload) 
                    {
                        if (data.payload.os_uptime !== undefined) 
                        {
                            document.getElementById('os-runtime').textContent = data.payload.os_uptime + 's';
                        }
                        if (data.payload.daemon_uptime !== undefined) 
                        {
                            document.getElementById('app-runtime').textContent = data.payload.daemon_uptime + 's';
                        }
                        
                        // Parse Under-Voltage hardware flag to toggle optimistic UI warning
                        const warnIcon = document.getElementById('power-warning-icon');
                        if (warnIcon)
                        {
                            if (data.payload.power_warn === true) 
                            {
                                warnIcon.classList.add('visible');
                            } 
                            else 
                            {
                                warnIcon.classList.remove('visible');
                            }
                        }
                    }
                    break;
                }
                case 'SYSTEM_STATE_REPORT':
                {
                    if (data.payload && data.payload.pins)
                    {
                         Object.assign(frontendGpioState, data.payload.pins);
                         
                         const openPinTitle = document.getElementById('modal-title').textContent;
                         if (document.getElementById('pin-modal-overlay').style.display === 'flex' && openPinTitle.includes('Pin'))
                         {
                             const pinIdStr = openPinTitle.split(' ')[1].replace(':', '');
                             const pinId = parseInt(pinIdStr, 10);
                             if (!isNaN(pinId)) 
                             {
                                 if (typeof window.openPinModal === 'function') 
                                 {
                                     window.openPinModal(pinId);
                                 }
                             }
                         }
                    }
                    break;
                }
                case 'COMMAND_RESPONSE':
                {
                    if (typeof window.handleCommandResponse === 'function') 
                    {
                        window.handleCommandResponse(data.payload);
                    }
                    break;
                }
                default:
                {
                    console.warn("Unknown packet type received:", data.type);
                    break;
                }
            }
        }
        catch (e) 
        {
            console.error("Failed to parse incoming WS frame:", e);
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

function handleDisconnect()
{
    const status = document.getElementById('conn-status');
    status.textContent = 'Disconnected';
    status.classList.remove('connected');
    
    document.getElementById('os-runtime').textContent = '0s';
    document.getElementById('app-runtime').textContent = '0s';
    
    if (sentinelTimer) 
    {
        clearInterval(sentinelTimer);
    }
}
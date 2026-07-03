let ws;
let lastHeartbeat = 0;
let sentinelTimer;

function connectWebSocket()
{
    ws = new WebSocket(`ws://${window.location.hostname}:8080`);

    ws.onopen = () => {
        const status = document.getElementById('conn-status');
        status.textContent = 'Connected';
        status.classList.add('connected');
        
        // Task 2.2: Send Handshake Initialization
        const helloPacket = { type: 'CLIENT_HELLO', timestamp: Date.now(), payload: {} };
        ws.send(JSON.stringify(helloPacket));

        // Task 2.5: Initialize Sentinel Watchdog
        lastHeartbeat = Date.now();
        startSentinel();
    };

    ws.onmessage = (event) => {
        try
        {
            const data = JSON.parse(event.data);
            
            // Route the packet based on our JSON Schema Specification
            switch(data.type)
            {
                case 'HEARTBEAT':
                    lastHeartbeat = Date.now();
                    break;
                    
                case 'SYSTEM_STATE_REPORT':
                    // We update the local UI state cache with the daemon's source of truth
                    // (This prepares us for Task 2.3 integration)
                    if (data.payload && data.payload.pins)
                    {
                         Object.assign(frontendGpioState, data.payload.pins);
                         
                         // If a modal is open, we need to refresh its content
                         const openPinTitle = document.getElementById('modal-title').textContent;
                         if (document.getElementById('pin-modal-overlay').style.display === 'flex' && openPinTitle.includes('Pin'))
                         {
                             const pinIdStr = openPinTitle.split(' ')[1].replace(':', '');
                             const pinId = parseInt(pinIdStr, 10);
                             if (!isNaN(pinId)) {
                                 // We call this globally from ui.js
                                 if (typeof window.openPinModal === 'function') {
                                     window.openPinModal(pinId);
                                 }
                             }
                         }
                    }
                    break;
                    
                case 'COMMAND_RESPONSE':
                    // Task 2.4: Handle UI Acknowledgement
                    handleCommandResponse(data.payload);
                    break;
                    
                default:
                    console.warn("Unknown packet type received:", data.type);
            }
        }
        catch (e) {
            console.error("Failed to parse incoming WS frame:", e);
        }
    };

    ws.onclose = () => {
        handleDisconnect();
        setTimeout(connectWebSocket, 2000);
    };

    ws.onerror = (error) => {
        console.error('WebSocket Error:', error);
    };
}

function startSentinel()
{
    if (sentinelTimer) clearInterval(sentinelTimer);
    
    // Check every 2 seconds if the daemon has died
    sentinelTimer = setInterval(() => {
        const now = Date.now();
        // If we haven't seen a heartbeat in 10 seconds, force a disconnect
        if (now - lastHeartbeat > 10000) {
            console.warn("Sentinel Watchdog: Daemon timeout detected.");
            handleDisconnect();
            // Force the socket closed so onclose can attempt reconnection
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.close();
            }
        }
    }, 2000);
}

function handleDisconnect()
{
    const status = document.getElementById('conn-status');
    status.textContent = 'Disconnected';
    status.classList.remove('connected');
    if (sentinelTimer) clearInterval(sentinelTimer);
}

// Stub for Task 2.4 (To be implemented in ui.js)
function handleCommandResponse(payload)
{
    if (payload.status === "SUCCESS") {
        console.log(`Pin ${payload.pin} operation successful.`);
        // Remove PENDING class logic here
    }
    else {
        console.error(`Pin ${payload.pin} operation failed: ${payload.message}`);
        // Revert UI toggle logic here
    }
}
// network.js
let sysWs;
let camWs;
let lastHeartbeat = 0;
let sentinelTimer;
let frameCount = 0;
let lastFpsTime = Date.now();

function connectWebSockets()
{
    connectSystemSocket();
    connectCameraSocket();
}

function connectSystemSocket()
{
    sysWs = new WebSocket('ws://' + window.location.hostname + ':8080');

    sysWs.onopen = () =>
    {
        const status = document.getElementById('conn-status');
        if (status)
        {
            status.textContent = 'System Connected';
            status.classList.add('connected');
        }
        
        const helloPacket = 
        { 
            type: 'CLIENT_HELLO', 
            timestamp: Date.now(), 
            payload: 
            {
            } 
        };
        
        sysWs.send(JSON.stringify(helloPacket));
        lastHeartbeat = Date.now();
        startSentinel();
    };

    sysWs.onmessage = (event) =>
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
                        if (data.payload.cpu_temp !== undefined)
                        {
                            document.getElementById('cpu-temp').textContent = data.payload.cpu_temp.toFixed(1) + '°C';
                        }
                        if (data.payload.load_avg !== undefined)
                        {
                            document.getElementById('load-avg').textContent = data.payload.load_avg.toFixed(2);
                        }
                        if (data.payload.ram_usage !== undefined)
                        {
                            document.getElementById('ram-usage').textContent = data.payload.ram_usage + '%';
                        }

                        if (typeof window.updateMetricChart === 'function')
                        {
                            window.updateMetricChart(data.payload);
                        }

                        const warnIcon = document.getElementById('power-warning-icon');
                        const uvText = document.getElementById('undervoltage');
                        if (warnIcon && uvText)
                        {
                            if (data.payload.power_warn === true) 
                            {
                                warnIcon.classList.add('visible');
                                uvText.textContent = "ACTIVE";
                            } 
                            else 
                            {
                                warnIcon.classList.remove('visible');
                                uvText.textContent = "NORMAL";
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
                        if (document.getElementById('pin-modal-overlay') && document.getElementById('pin-modal-overlay').style.display === 'flex' && openPinTitle.includes('Pin'))
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
                case 'ERROR_REPORT':
                {
                    console.error(`[SYSTEM DAEMON ERROR] ${data.payload.source} (${data.payload.code}): ${data.payload.message}`);
                    break;
                }
                default:
                {
                    console.warn("Unknown packet type received on system socket:", data.type);
                    break;
                }
            }
        }
        catch (e) 
        {
            console.error("Failed to parse incoming system WS frame:", e);
        }
    };

    sysWs.onclose = () => 
    {
        handleDisconnect();
        setTimeout(connectSystemSocket, 2000);
    };

    sysWs.onerror = (error) => 
    {
        console.error('System WebSocket Error:', error);
    };
}

function connectCameraSocket()
{
    camWs = new WebSocket('ws://' + window.location.hostname + ':8081');

    camWs.onopen = () =>
    {
        console.log('Camera WebSocket Connected');
    };

    camWs.onmessage = async (event) =>
    {
        if (event.data instanceof Blob)
        {
            const arrayBuffer = await event.data.arrayBuffer();
            handleBinaryFrame(arrayBuffer);
            return;
        }

        try
        {
            const data = JSON.parse(event.data);
            
            switch(data.type)
            {
                case 'COMMAND_RESPONSE':
                {
                    if (typeof window.handleCommandResponse === 'function') 
                    {
                        window.handleCommandResponse(data.payload);
                    }
                    break;
                }
                case 'RECORDING_STATE_UPDATE':
                {
                    console.log(`[CAMERA DAEMON] Recording active: ${data.payload.elapsed_seconds}s - ${data.payload.file_path}`);
                    break;
                }
                case 'ERROR_REPORT':
                {
                    console.error(`[CAMERA DAEMON ERROR] ${data.payload.source} (${data.payload.code}): ${data.payload.message}`);
                    break;
                }
                default:
                {
                    console.warn("Unknown packet type received on camera socket:", data.type);
                    break;
                }
            }
        }
        catch (e) 
        {
            console.error("Failed to parse incoming camera WS frame:", e);
        }
    };

    camWs.onclose = () => 
    {
        console.warn('Camera WebSocket Disconnected');
        setTimeout(connectCameraSocket, 2000);
    };

    camWs.onerror = (error) => 
    {
        console.error('Camera WebSocket Error:', error);
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
            console.warn("Sentinel Watchdog: System connection lost (Timeout threshold reached).");
            handleDisconnect();
            if (sysWs && sysWs.readyState === WebSocket.OPEN) 
            {
                sysWs.close();
            }
        }
    }, 1000);
}

function handleDisconnect()
{
    const status = document.getElementById('conn-status');
    if (status)
    {
        status.textContent = 'Disconnected';
        status.classList.remove('connected');
    }
    
    const osRuntime = document.getElementById('os-runtime');
    const appRuntime = document.getElementById('app-runtime');
    
    if (osRuntime)
    {
        osRuntime.textContent = '0s';
    }
    if (appRuntime)
    {
        appRuntime.textContent = '0s';
    }
    
    if (sentinelTimer) 
    {
        clearInterval(sentinelTimer);
    }
}

function handleBinaryFrame(arrayBuffer)
{
    if (arrayBuffer.byteLength < 8)
    {
        return;
    }

    const dataView = new DataView(arrayBuffer);
    const frameIndex = dataView.getUint32(0, false);
    const timeDelta = dataView.getUint32(4, false);

    const jpegBlob = new Blob([arrayBuffer.slice(8)], { type: 'image/jpeg' });
    const imageUrl = URL.createObjectURL(jpegBlob);

    const canvas = document.getElementById('viewfinder-canvas');
    if (canvas)
    {
        const ctx = canvas.getContext('2d');
        const img = new Image();
        
        img.onload = () =>
        {
            ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
            
            URL.revokeObjectURL(imageUrl);
            
            const camStatus = document.getElementById('cam-status');
            if (camStatus)
            {
                camStatus.textContent = "Live";
                camStatus.className = "status-badge connected";
            }
            
            frameCount++;
            const now = Date.now();
            
            if (now - lastFpsTime >= 1000)
            {
                const fps = Math.round((frameCount * 1000) / (now - lastFpsTime));
                const fpsCounter = document.getElementById('cam-fps');
                if (fpsCounter)
                {
                    fpsCounter.textContent = `${fps} FPS / 16:9`;
                }
                frameCount = 0;
                lastFpsTime = now;
            }
        };
        img.src = imageUrl;
    }
}
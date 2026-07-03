let ws;

function connectWebSocket() {
    ws = new WebSocket(`ws://${window.location.hostname}:8080`);

    ws.onopen = () => {
        const status = document.getElementById('conn-status');
        status.textContent = 'Connected';
        status.classList.add('connected');
    };

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        document.getElementById('os-runtime').textContent = data.os_runtime + 's';
        document.getElementById('app-runtime').textContent = data.app_runtime + 's';
    };

    ws.onclose = () => {
        const status = document.getElementById('conn-status');
        status.textContent = 'Disconnected';
        status.classList.remove('connected');
        setTimeout(connectWebSocket, 2000);
    };

    ws.onerror = (error) => {
        console.error('WebSocket Error:', error);
    };
}

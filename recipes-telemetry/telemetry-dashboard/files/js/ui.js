function renderPinout()
{
    const grid = document.getElementById('pin-grid');
    grid.innerHTML = ''; 
    
    const evenPins = piPinout.filter(p => p.pin % 2 === 0);
    const oddPins = piPinout.filter(p => p.pin % 2 !== 0);

    const renderRow = (pins) => {
        pins.forEach(pinData => {
            const pinDiv = document.createElement('div');
            
            let classes = `pin ${pinData.type}`;
            if (pinData.highlight) classes += ' highlight';
            pinDiv.className = classes;
            
            pinDiv.textContent = pinData.pin;
            pinDiv.title = `Pin ${pinData.pin}: ${pinData.name}`;
            pinDiv.onclick = () => window.openPinModal(pinData.pin);
            
            grid.appendChild(pinDiv);
        });
    };

    renderRow(evenPins);
    renderRow(oddPins);
}

window.openPinModal = function(pinId)
{
    const pinData = piPinout.find(p => p.pin === pinId);
    if (!pinData) return;

    const title = document.getElementById('modal-title');
    const bodyContent = document.getElementById('modal-body-content');
    
    // Narrow modal for GPIO
    document.getElementById('modal-card-container').style.maxWidth = '420px';

    title.textContent = `Pin ${pinData.pin}: ${pinData.name}`;

    let htmlBuilder = `<p class="modal-desc">`;

    if (pinData.type === 'power5v' || pinData.type === 'power3v3' || pinData.type === 'gnd' || pinData.type === 'reserved')
    {
        if (pinData.type === 'power5v') htmlBuilder += "5V power rail. Connected directly to main system power input.";
        if (pinData.type === 'power3v3') htmlBuilder += "3.3V power rail sourced from the onboard PMIC. Maximum combined current draw must not exceed 50mA.";
        if (pinData.type === 'gnd') htmlBuilder += "0V reference plane for digital signals and return currents.";
        if (pinData.type === 'reserved') htmlBuilder += "Reserved I2C EEPROM interface for HAT auto-probing. Manipulation may corrupt boot sequence.";
        
        htmlBuilder += `</p><div class="read-only-badge">SYSTEM HARD-WIRED (READ-ONLY)</div>`;
    }
    else {
        htmlBuilder += `Configurable GPIO pin interfaced via the RP1 southbridge. Max source/sink current is 15mA per pin.</p>`;
        
        if (!frontendGpioState[pinId]) {
            frontendGpioState[pinId] = { mode: 'IN', val: 0 };
        }
        const state = frontendGpioState[pinId];

        htmlBuilder += `
            <div id="modal-controls" style="opacity: 1; pointer-events: auto;">
                <div class="control-group">
                    <label>Direction</label>
                    <div class="segmented-control">
                        <button class="${state.mode === 'IN' ? 'active' : ''}" onclick="updatePinMode(${pinId}, 'IN')">INPUT</button>
                        <button class="${state.mode === 'OUT' ? 'active' : ''}" onclick="updatePinMode(${pinId}, 'OUT')">OUTPUT</button>
                    </div>
                </div>
                <div class="control-group" id="state-toggle-group" style="display: ${state.mode === 'OUT' ? 'flex' : 'none'};">
                    <label>Voltage State</label>
                    <label class="switch">
                        <input type="checkbox" ${state.val === 1 ? 'checked' : ''} onchange="updatePinState(${pinId}, this.checked)">
                        <span class="slider"></span>
                    </label>
                    <span id="pin-state-label-${pinId}" class="state-label">${state.val === 1 ? 'HIGH (3.3V)' : 'LOW (0V)'}</span>
                </div>
            </div>
        `;
    }
    
    bodyContent.innerHTML = htmlBuilder;
    document.getElementById('shared-modal-overlay').style.display = 'flex';
};


// --- Metric Drill-Down Logic ---

let drillDownChart = null;
let drillDownInterval = null;

window.openMetricModal = function(metricType) {
    const title = document.getElementById('modal-title');
    const bodyContent = document.getElementById('modal-body-content');
    
    // Widen modal for the Chart
    document.getElementById('modal-card-container').style.maxWidth = '600px';

    if (metricType === 'cpu') {
        title.textContent = "CPU Temperature Trends";
    } else if (metricType === 'ram') {
        title.textContent = "RAM Usage Trends";
    }

    // Inject canvas
    bodyContent.innerHTML = `
        <div class="modal-graph-container">
            <canvas id="drillDownCanvas"></canvas>
        </div>
    `;

    document.getElementById('shared-modal-overlay').style.display = 'flex';
    
    initDrillDownChart(metricType);
};

function initDrillDownChart(metricType) {
    const ctx = document.getElementById('drillDownCanvas').getContext('2d');
    
    const isLight = document.documentElement.classList.contains('light-theme');
    const textColor = isLight ? '#5f6368' : '#a0a0a0';
    const gridColor = isLight ? '#dadce0' : '#333333';
    
    Chart.defaults.color = textColor;
    Chart.defaults.font.family = 'system-ui, -apple-system, sans-serif';

    let config = {
        type: 'line',
        data: {
            labels: Array(60).fill(''),
            datasets: [{
                label: metricType === 'cpu' ? 'Temp (°C)' : 'Usage (%)',
                borderColor: metricType === 'cpu' ? '#e57373' : '#bb86fc',
                backgroundColor: metricType === 'cpu' ? 'rgba(229, 115, 115, 0.1)' : 'rgba(187, 134, 252, 0.1)',
                borderWidth: 2,
                pointRadius: 0,
                pointHitRadius: 10,
                tension: 0.4,
                data: Array(60).fill(null)
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: { display: false },
                tooltip: { intersect: false, mode: 'index' }
            },
            scales: {
                x: { display: false },
                y: {
                    type: 'linear', display: true, position: 'left',
                    min: metricType === 'cpu' ? 30 : 0, 
                    max: metricType === 'cpu' ? 90 : 100,
                    grid: { color: gridColor }
                }
            }
        }
    };

    drillDownChart = new Chart(ctx, config);
    
    // Start mock data feed for this specific modal
    if (!drillDownInterval) {
        drillDownInterval = setInterval(() => {
            if (!drillDownChart) return;
            const ds = drillDownChart.data.datasets[0];
            const lastVal = ds.data[59] || (metricType === 'cpu' ? 45 : 25);
            const newVal = Math.min(config.options.scales.y.max, Math.max(config.options.scales.y.min, lastVal + (Math.random() * 2 - 1)));
            ds.data.shift();
            ds.data.push(newVal);
            drillDownChart.update('none');
        }, 1000);
    }
}

window.closeModal = function() {
    document.getElementById('shared-modal-overlay').style.display = 'none';
    
    // CRITICAL: Lifecycle Management for Chart.js
    if (drillDownChart) {
        drillDownChart.destroy();
        drillDownChart = null;
    }
    if (drillDownInterval) {
        clearInterval(drillDownInterval);
        drillDownInterval = null;
    }
};

window.handleOverlayClick = function(event) {
    if (event.target.id === 'shared-modal-overlay') {
        closeModal();
    }
};

function sendGpioCommand(pinId, mode, val)
{
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        console.error("Cannot execute command: Backend daemon is currently disconnected.");
        window.openPinModal(pinId); // Re-render to revert toggle UI visually
        return;
    }

    // 1. Lock the UI (Pending State)
    const controls = document.getElementById('modal-controls');
    if (controls) {
        controls.style.opacity = '0.5';
        controls.style.pointerEvents = 'none';
    }
    
    // Optimistic State Update
    // Save what the user requested so the UI updates instantly on success
    frontendGpioState[pinId] = { mode: mode, val: val };

    // 2. Transmit the JSON Schema Request
    const commandFrame = {
        type: 'WRITE_GPIO_REQUEST',
        timestamp: Date.now(),
        payload: {
            pin: pinId,
            mode: mode,
            val: val
        }
    };
    ws.send(JSON.stringify(commandFrame));
}

function updatePinMode(pinId, mode)
{
    const currentVal = frontendGpioState[pinId] ? frontendGpioState[pinId].val : 0;
    sendGpioCommand(pinId, mode, currentVal);
}

function updatePinState(pinId, isHigh)
{
    const currentMode = frontendGpioState[pinId] ? frontendGpioState[pinId].mode : 'OUT';
    const val = isHigh ? 1 : 0;
    sendGpioCommand(pinId, currentMode, val);
}

window.handleCommandResponse = function(payload)
{
    const controls = document.getElementById('modal-controls');
    
    // 3. Unlock the UI
    if (controls) {
        controls.style.opacity = '1';
        controls.style.pointerEvents = 'auto';
    }

    if (payload.status === "ERROR") {
        console.error(`Hardware Interlock Triggered for Pin ${payload.pin}: ${payload.message}`);
    }
    
    // Re-render the modal to reflect the new state immediately
    window.openPinModal(payload.pin);
};

function switchTab(tabId)
{
    document.querySelectorAll('.tab-content').forEach(tab => tab.classList.remove('active'));
    document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
    
    document.getElementById(`tab-${tabId}`).classList.add('active');
    if (window.event && window.event.currentTarget) {
        window.event.currentTarget.classList.add('active');
    }
    else {
        const btn = document.querySelector(`button[onclick="switchTab('${tabId}')"]`);
        if (btn) btn.classList.add('active');
    }
}

// --- Chart.js & Advanced View Logic ---
let telemetryChart = null;
let mockDataInterval = null;

function toggleAdvancedView(isAdvanced) {
    const container = document.getElementById('advanced-graph-container');
    if (isAdvanced) {
        container.classList.remove('hidden');
        if (!telemetryChart) initChart();
        
        // Start mock data for UI preview purposes
        if (!mockDataInterval) {
            mockDataInterval = setInterval(feedMockData, 1000);
        }
    } else {
        container.classList.add('hidden');
        if (mockDataInterval) {
            clearInterval(mockDataInterval);
            mockDataInterval = null;
        }
    }
}

function initChart() {
    const ctx = document.getElementById('telemetryChart').getContext('2d');
    
    Chart.defaults.color = '#a0a0a0';
    Chart.defaults.font.family = 'system-ui, -apple-system, sans-serif';

    telemetryChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: Array(60).fill(''), // 60 seconds rolling window
            datasets: [
                {
                    label: 'CPU Temp (°C)',
                    borderColor: '#e57373', // Matches power5v red
                    backgroundColor: 'rgba(229, 115, 115, 0.1)',
                    borderWidth: 2,
                    pointRadius: 0,
                    pointHitRadius: 10,
                    tension: 0.4,
                    yAxisID: 'y',
                    data: Array(60).fill(null)
                },
                {
                    label: 'RAM Usage (%)',
                    borderColor: '#bb86fc', // Matches accent purple
                    backgroundColor: 'rgba(187, 134, 252, 0.1)',
                    borderWidth: 2,
                    pointRadius: 0,
                    pointHitRadius: 10,
                    tension: 0.4,
                    yAxisID: 'y1',
                    data: Array(60).fill(null)
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            interaction: { mode: 'index', intersect: false },
            plugins: {
                legend: { position: 'top', labels: { usePointStyle: true, boxWidth: 8 } },
                tooltip: { backgroundColor: '#1e1e1e', titleColor: '#e0e0e0', bodyColor: '#e0e0e0', borderColor: '#333', borderWidth: 1 }
            },
            scales: {
                x: { display: false, grid: { display: false } },
                y: { 
                    type: 'linear', display: true, position: 'left',
                    title: { display: true, text: 'Temp (°C)' },
                    min: 30, max: 90,
                    grid: { color: '#333' }
                },
                y1: {
                    type: 'linear', display: true, position: 'right',
                    title: { display: true, text: 'RAM (%)' },
                    min: 0, max: 100,
                    grid: { drawOnChartArea: false }, // Prevent overlapping grid lines
                }
            }
        }
    });
}

// Temporary function to preview graph animation
function feedMockData() {
    if (!telemetryChart) return;
    
    const datasets = telemetryChart.data.datasets;
    const lastTemp = datasets[0].data[59] || 45;
    const lastRam = datasets[1].data[59] || 25;
    
    // Generate realistic fluctuating data
    const newTemp = Math.min(85, Math.max(35, lastTemp + (Math.random() * 2 - 1)));
    const newRam = Math.min(100, Math.max(10, lastRam + (Math.random() * 4 - 2)));

    // Shift data left and push new values
    datasets[0].data.shift();
    datasets[0].data.push(newTemp);
    
    datasets[1].data.shift();
    datasets[1].data.push(newRam);
    
    // Update chart without resetting animation for smooth rolling
    telemetryChart.update('none'); 
}

function toggleTheme(forceLight = null) {
    const root = document.documentElement;
    const isLight = forceLight !== null ? forceLight : !root.classList.contains('light-theme');
    
    if (isLight) {
        root.classList.add('light-theme');
        document.getElementById('theme-icon-sun').style.display = 'block';
        document.getElementById('theme-icon-moon').style.display = 'none';
    } else {
        root.classList.remove('light-theme');
        document.getElementById('theme-icon-sun').style.display = 'none';
        document.getElementById('theme-icon-moon').style.display = 'block';
    }
    localStorage.setItem('theme', isLight ? 'light' : 'dark');

    // Update active modal chart if it exists
    if (drillDownChart) {
        const textColor = isLight ? '#5f6368' : '#a0a0a0';
        const gridColor = isLight ? '#dadce0' : '#333333';
        Chart.defaults.color = textColor;
        drillDownChart.options.scales.y.grid.color = gridColor;
        drillDownChart.update('none');
    }
}

function initTheme() {
    const savedTheme = localStorage.getItem('theme');
    if (savedTheme === 'light') {
        toggleTheme(true);
    }
}

window.onload = () => {
    initTheme();
    renderPinout();
    if (typeof connectWebSocket === 'function') {
        connectWebSocket();
    }
};
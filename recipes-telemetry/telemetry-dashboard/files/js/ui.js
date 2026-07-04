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
    const desc = document.getElementById('modal-desc');
    const controls = document.getElementById('modal-controls');

    title.textContent = `Pin ${pinData.pin}: ${pinData.name}`;

    // Reset any pending visual states
    controls.style.opacity = '1';
    controls.style.pointerEvents = 'auto';

    if (pinData.type === 'power5v' || pinData.type === 'power3v3' || pinData.type === 'gnd' || pinData.type === 'reserved')
    {
        if (pinData.type === 'power5v') desc.textContent = "5V power rail. Connected directly to main system power input.";
        if (pinData.type === 'power3v3') desc.textContent = "3.3V power rail sourced from the onboard PMIC. Maximum combined current draw must not exceed 50mA.";
        if (pinData.type === 'gnd') desc.textContent = "0V reference plane for digital signals and return currents.";
        if (pinData.type === 'reserved') desc.textContent = "Reserved I2C EEPROM interface for HAT auto-probing. Manipulation may corrupt boot sequence.";
        
        controls.innerHTML = `<div class="read-only-badge">SYSTEM HARD-WIRED (READ-ONLY)</div>`;
    }
    else {
        desc.textContent = "Configurable GPIO pin interfaced via the RP1 southbridge. Max source/sink current is 15mA per pin.";
        
        if (!frontendGpioState[pinId]) {
            frontendGpioState[pinId] = { mode: 'IN', val: 0 };
        }
        const state = frontendGpioState[pinId];

        controls.innerHTML = `
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
        `;
    }

    document.getElementById('pin-modal-overlay').style.display = 'flex';
};

function closePinModal()
{
    document.getElementById('pin-modal-overlay').style.display = 'none';
}

function handleOverlayClick(event)
{
    if (event.target.id === 'pin-modal-overlay') {
        closePinModal();
    }
}

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

window.onload = () => {
    renderPinout();
    if (typeof connectWebSocket === 'function') {
        connectWebSocket();
    }
};
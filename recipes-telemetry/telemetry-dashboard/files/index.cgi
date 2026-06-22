#!/bin/bash

# Required CGI HTTP header
echo "Content-type: text/html"
echo ""

# Execute SNMPv3 loopback queries
# -Oqv: strips the MIB tree prefix, returning only the raw value
# tr -d '"': removes double quotes from the SNMP string output
OS_RUNTIME=$(snmpget -v3 -l authPriv -u admin -a SHA -A "authpassword123" -x AES -X "privpassword123" -Oqv 127.0.0.1 .1.3.6.1.4.1.99999.2.0 | tr -d '"')
APP_RUNTIME=$(snmpget -v3 -l authPriv -u admin -a SHA -A "authpassword123" -x AES -X "privpassword123" -Oqv 127.0.0.1 .1.3.6.1.4.1.99999.1.0 | tr -d '"')

# Query RP1 Southbridge GPIO Table for Pin 14 (Index 15 in our MIB table)
# xargs: strips leading/trailing whitespaces from the stdout
GPIO_14_VAL=$(snmpget -v3 -l authPriv -u admin -a SHA -A "authpassword123" -x AES -X "privpassword123" -Oqv 127.0.0.1 .1.3.6.1.4.1.99999.3.1.3.15 | tr -d '"' | xargs)

# Handle failure states if the SNMP agent is down or timing out
if [ -z "$OS_RUNTIME" ]; then OS_RUNTIME="ERR_TIMEOUT"; fi
if [ -z "$APP_RUNTIME" ]; then APP_RUNTIME="ERR_TIMEOUT"; fi

# Map the raw GPIO value to dynamic UI CSS classes and textual indicators
if [ "$GPIO_14_VAL" = "1" ]; then
    GPIO_CLASS="status-high"
    GPIO_TEXT="HIGH (1)"
elif [ "$GPIO_14_VAL" = "0" ]; then
    GPIO_CLASS="status-low"
    GPIO_TEXT="LOW (0)"
else
    GPIO_CLASS="status-error"
    GPIO_TEXT="OFFLINE"
fi

# Generate HTML payload with clean, responsive modern styling
cat <<EOF
<!DOCTYPE html>
<html>
<head>
    <title>Telemetry Status</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { 
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; 
            background-color: #f4f5f7; 
            color: #202124; 
            margin: 0;
            padding: 40px 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 80vh;
        }
        .telemetry-container { 
            background-color: #ffffff; 
            border: 1px solid #e1e4e8; 
            padding: 32px; 
            border-radius: 12px; 
            width: 100%;
            max-width: 480px; 
            box-shadow: 0 4px 12px rgba(0,0,0,0.05);
        }
        h2 { 
            margin-top: 0; 
            margin-bottom: 24px;
            font-size: 1.5rem;
            font-weight: 700;
            border-bottom: 1px solid #f0f1f2; 
            padding-bottom: 16px;
        }
        .metric-row { 
            margin-bottom: 20px; 
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .metric-label { 
            font-weight: 500; 
            color: #4a4a4a;
        }
        .metric-value { 
            font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, "Liberation Mono", monospace; 
            font-size: 1.05em; 
            color: #0f52ba;
            font-weight: 600;
        }
        .status-badge {
            padding: 6px 14px;
            border-radius: 50px;
            font-size: 0.85em;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.05em;
        }
        .status-high {
            background-color: #e6f6ec;
            color: #137333;
            border: 1px solid #c2e7d9;
        }
        .status-low {
            background-color: #f1f3f4;
            color: #5f6368;
            border: 1px solid #dadce0;
        }
        .status-error {
            background-color: #fce8e6;
            color: #c5221f;
            border: 1px solid #fad2cf;
        }
        .divider {
            height: 1px;
            background-color: #f0f1f2;
            margin: 24px 0;
            border: none;
        }
    </style>
</head>
<body>
    <div class="telemetry-container">
        <h2>System Telemetry</h2>
        <div class="metric-row">
            <span class="metric-label">Operating System Runtime:</span>
            <span class="metric-value">${OS_RUNTIME}</span>
        </div>
        <div class="metric-row">
            <span class="metric-label">Application Runtime:</span>
            <span class="metric-value">${APP_RUNTIME}</span>
        </div>
        <div class="metric-row">
            <span class="metric-label">Navigation Mode:</span>
            <span class="status-badge status-high">Active</span>
        </div>
        
        <hr class="divider">
        
        <div class="metric-row">
            <span class="metric-label">GPIO Pin 14 (Active Lock):</span>
            <span class="status-badge ${GPIO_CLASS}">${GPIO_TEXT}</span>
        </div>
    </div>
</body>
</html>
EOF
/*
 * main.c
 * AgentX Subagent Wrapper
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <sys/sysinfo.h> // Required for kernel sysinfo() system call
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "appUptimeSeconds.h"
#include "gpioTable.h" // Injected RP1 hardware telemetry matrix

static int keep_running = 1;

// Graceful shutdown handler to release the Unix domain socket
void stop_server(int signum)
{
    keep_running = 0;
}

/* * Handler for OS Kernel Uptime 
 * OID: .1.3.6.1.4.1.99999.2.0 
 */
int handle_osUptimeSeconds(netsnmp_mib_handler *handler,
                           netsnmp_handler_registration *reginfo,
                           netsnmp_agent_request_info *reqinfo,
                           netsnmp_request_info *requests) {
    struct sysinfo info;
    long os_uptime = 0;

    switch (reqinfo->mode) 
    {
        case MODE_GET:
            // sysinfo() directly queries the Linux kernel for system statistics.
            // Populates the struct and extracts the raw uptime member in seconds.
            if (sysinfo(&info) == 0) {
                os_uptime = info.uptime; 
            } 
            else {
                snmp_log(LOG_ERR, "AgentX subagent failed to read kernel sysinfo.\n");
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
                return SNMP_ERR_NOERROR;
            }

            // Pack the extracted integer into the Net-SNMP ASN.1 payload buffer
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char *)&os_uptime, sizeof(os_uptime));
            break;
        default:
            return SNMP_ERR_GENERR;
    }
    return SNMP_ERR_NOERROR;
}

void init_osUptimeSeconds(void) 
{
    // Define the base OID: .1.3.6.1.4.1.99999.2
    const oid osUptime_oid[] = {1, 3, 6, 1, 4, 1, 99999, 2};
    netsnmp_register_scalar(
        netsnmp_create_handler_registration(
            "osUptimeSeconds", handle_osUptimeSeconds,
            osUptime_oid, OID_LENGTH(osUptime_oid),
            HANDLER_CAN_RONLY
        )
    );
}

int main(int argc, char **argv) 
{
    // Bind OS signals to release the IPC socket on termination
    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);

    // FAILURE MODE: If not set, the process will attempt to bind to UDP 161 (Master)
    // instead of the /var/agentx/master Unix domain socket.
    netsnmp_enable_subagent();

    // Initialize syslog for daemon debugging
    snmp_enable_stderrlog();

    // Initialize the AgentX framework with a unique session name
    init_agent("telemetry_agent");

    // Initialize your specific custom OID branch from the generated header
    init_appUptimeSeconds();
    
    // Initialize the OS kernel uptime scalar
    init_osUptimeSeconds();

    // Initialize the hardware telemetry GPIO table
    init_gpioTable();

    // Read the snmpd.conf files and complete the AgentX socket handshake
    init_snmp("telemetry_agent");

    snmp_log(LOG_INFO, "Telemetry AgentX Subagent initialized and listening.\n");

    // The Net-SNMP Event Loop
    // agent_check_and_process(1) blocks until a GET/SET request arrives over the socket.
    // Passing 0 makes it non-blocking, which would require manual sleep() throttling
    // to prevent 100% CPU utilization.
    while(keep_running) {
        agent_check_and_process(1); 
    }

    // Cleanup memory allocations and close the socket
    snmp_shutdown("telemetry_agent");
    snmp_log(LOG_INFO, "Telemetry AgentX Subagent terminated.\n");

    return 0;
}
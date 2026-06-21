/*
 * appUptimeSeconds.c
 * Implements the AgentX scalar handler for .1.3.6.1.4.1.99999.1
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <time.h>
#include <stdint.h>
#include "appUptimeSeconds.h"

// Statically allocate the initialization timestamp in memory using the POSIX timespec struct
static struct timespec agent_start_time = {0, 0};

/** Initializes the appUptimeSeconds module */
void init_appUptimeSeconds(void)
{
    // Query the hardware monotonic clock. This isolates the subagent from NTP time jumps.
    if (clock_gettime(CLOCK_MONOTONIC, &agent_start_time) != 0) {
        snmp_log(LOG_ERR, "Failed to initialize CLOCK_MONOTONIC\n");
    }

    const oid appUptimeSeconds_oid[] = { 1,3,6,1,4,1,99999,1 };

    DEBUGMSGTL(("appUptimeSeconds", "Initializing custom uptime scalar\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("appUptimeSeconds", handle_appUptimeSeconds,
                               appUptimeSeconds_oid, OID_LENGTH(appUptimeSeconds_oid),
                               HANDLER_CAN_RONLY
        ));
}

int handle_appUptimeSeconds(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    switch(reqinfo->mode) {
        case MODE_GET: {
            struct timespec current_time;
            
            if (clock_gettime(CLOCK_MONOTONIC, &current_time) != 0) {
                return SNMP_ERR_GENERR;
            }

            // ARM64 (aarch64) uses the LP64 data model where 'long' is 64 bits.
            // ASN_INTEGER strictly requires a 32-bit signed integer.
            // We cast the delta to int32_t to prevent memory misalignment during Net-SNMP byte encoding.
            int32_t uptime_seconds = (int32_t)(current_time.tv_sec - agent_start_time.tv_sec);

            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                     (u_char *)&uptime_seconds,
                                     sizeof(uptime_seconds));
            break;
        }

        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_appUptimeSeconds\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
/*
 * appUptimeSeconds.c
 * Implements the AgentX scalar handler for .1.3.6.1.4.1.99999.1
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <time.h>
#include "appUptimeSeconds.h"

// Statically allocate the initialization timestamp in memory
static time_t agent_start_time = 0;

/** Initializes the appUptimeSeconds module */
void init_appUptimeSeconds(void)
{
    // Record the exact Unix epoch time when the AgentX subagent boots
    agent_start_time = time(NULL);

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
            // Calculate the delta between current time and initialization time
            time_t current_time = time(NULL);
            long uptime_seconds = (long)difftime(current_time, agent_start_time);

            // Net-SNMP requires the data pointer to be cast to (u_char *) regardless of the actual ASN type.
            // ASN_INTEGER tells the daemon how to interpret the bytes at that memory address.
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

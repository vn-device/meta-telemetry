/*
 * Refactored gpioTable.c
 * Hardened for direct libgpiod v2 kernel polling on Raspberry Pi 5 (RP1 Southbridge)
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <gpiod.h>
#include <string.h> /* Required for strcmp */
#include <stdio.h>  /* Required for snprintf */
#include "gpioTable.h"

/** Initializes the gpioTable module */
void init_gpioTable(void) {
    initialize_table_gpioTable();
}

/* Typical data structure for a row entry */
struct gpioTable_entry {
    long gpioIndex;
    long gpioBcmNumber;
    int valid;
    struct gpioTable_entry *next;
};

struct gpioTable_entry *gpioTable_head = NULL;

/* create a new row in the table */
struct gpioTable_entry *gpioTable_createEntry(long gpioIndex, long gpioBcmNumber) {
    struct gpioTable_entry *entry;

    entry = SNMP_MALLOC_TYPEDEF(struct gpioTable_entry);
    if (!entry)
        return NULL;

    entry->gpioIndex = gpioIndex;
    entry->gpioBcmNumber = gpioBcmNumber;
    entry->next = gpioTable_head;
    gpioTable_head = entry;
    return entry;
}

/** Initialize the gpioTable table by defining its contents and how it's structured */
void initialize_table_gpioTable(void) {
    const oid gpioTable_oid[] = {1, 3, 6, 1, 4, 1, 99999, 3};
    const size_t gpioTable_oid_len = OID_LENGTH(gpioTable_oid);
    netsnmp_handler_registration *reg;
    netsnmp_iterator_info *iinfo;
    netsnmp_table_registration_info *table_info;

    DEBUGMSGTL(("gpioTable:init", "initializing table gpioTable\n"));

    reg = netsnmp_create_handler_registration(
              "gpioTable", gpioTable_handler,
              gpioTable_oid, gpioTable_oid_len,
              HANDLER_CAN_RONLY
              );

    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, 0); /* index: gpioIndex */
    table_info->min_column = COLUMN_GPIOBCMNUMBER;
    table_info->max_column = COLUMN_GPIOLOCKSTATUS;
    
    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    iinfo->get_first_data_point = gpioTable_get_first_data_point;
    iinfo->get_next_data_point  = gpioTable_get_next_data_point;
    iinfo->table_reginfo        = table_info;
    
    netsnmp_register_table_iterator(reg, iinfo);

    /* Construct the 28-row hardware routing matrix */
    /* Iterating backwards ensures the linked list order matches the 1-28 sequence */
    for (int i = 28; i >= 1; i--) {
        gpioTable_createEntry(i, i - 1);
    }
}

/* Iterator hook routines */
netsnmp_variable_list *
gpioTable_get_first_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata) {
    *my_loop_context = gpioTable_head;
    return gpioTable_get_next_data_point(my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list *
gpioTable_get_next_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata) {
    struct gpioTable_entry *entry = (struct gpioTable_entry *)*my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

    if (entry) {
        snmp_set_var_typed_integer(idx, ASN_INTEGER, entry->gpioIndex);
        idx = idx->next_variable;
        *my_data_context = (void *)entry;
        *my_loop_context = (void *)entry->next;
        return put_index_data;
    } else {
        return NULL;
    }
}

/** handles requests for the gpioTable table */
int gpioTable_handler(
    netsnmp_mib_handler *handler,
    netsnmp_handler_registration *reginfo,
    netsnmp_agent_request_info *reqinfo,
    netsnmp_request_info *requests) {

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    struct gpioTable_entry *table_entry;

    DEBUGMSGTL(("gpioTable:handler", "Processing request (%d)\n", reqinfo->mode));

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (struct gpioTable_entry *)netsnmp_extract_iterator_context(request);
            table_info  = netsnmp_extract_table_info(request);
    
            switch (table_info->colnum) {
            case COLUMN_GPIOBCMNUMBER:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                    continue;
                }
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER, table_entry->gpioBcmNumber);
                break;

            case COLUMN_GPIOLOCKSTATUS:
                if (!table_entry) {
                    netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                    continue;
                }
                
                int is_used = 0;
                struct gpiod_chip *chip = NULL;
                
                /* libgpiod v2 API: Iterate through character devices to locate RP1 */
                for (int i = 0; i < 8; i++) {
                    char path[32];
                    snprintf(path, sizeof(path), "/dev/gpiochip%d", i);
                    chip = gpiod_chip_open(path);
                    if (chip) {
                        struct gpiod_chip_info *cinfo = gpiod_chip_get_info(chip);
                        if (cinfo) {
                            if (strcmp(gpiod_chip_info_get_label(cinfo), "pinctrl-rp1") == 0) {
                                gpiod_chip_info_free(cinfo);
                                break; /* Found the RP1 southbridge */
                            }
                            gpiod_chip_info_free(cinfo);
                        }
                        gpiod_chip_close(chip);
                        chip = NULL;
                    }
                }

                if (chip) {
                    struct gpiod_line_info *linfo = gpiod_chip_get_line_info(chip, table_entry->gpioBcmNumber);
                    if (linfo) {
                        is_used = gpiod_line_info_is_used(linfo);
                        gpiod_line_info_free(linfo); /* Prevent memory leak */
                    }
                    gpiod_chip_close(chip); /* Free the chip resource */
                }

                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER, is_used ? 1 : 0);
                break;

            default:
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHOBJECT);
                break;
            }
        }
        break;
    }
    return SNMP_ERR_NOERROR;
}
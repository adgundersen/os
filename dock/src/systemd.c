#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include "systemd.h"

#define SYSTEMD_DEST  "org.freedesktop.systemd1"
#define SYSTEMD_PATH  "/org/freedesktop/systemd1"
#define MANAGER_IFACE "org.freedesktop.systemd1.Manager"
#define UNIT_IFACE    "org.freedesktop.systemd1.Unit"

static sd_bus *open_bus(void)
{
    sd_bus *bus = NULL;
    if (sd_bus_open_system(&bus) < 0) return NULL;
    return bus;
}

int systemd_is_active(const char *unit)
{
    sd_bus         *bus   = open_bus();
    sd_bus_error    error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    const char     *path;
    char           *state = NULL;
    int             result = 0;

    if (!bus) return -1;

    /* GetUnit → object path */
    if (sd_bus_call_method(bus, SYSTEMD_DEST, SYSTEMD_PATH, MANAGER_IFACE,
                           "GetUnit", &error, &reply, "s", unit) < 0)
        goto done;  /* unit doesn't exist = not running */

    if (sd_bus_message_read(reply, "o", &path) < 0) goto done;

    /* Read ActiveState property */
    if (sd_bus_get_property_string(bus, SYSTEMD_DEST, path, UNIT_IFACE,
                                   "ActiveState", &error, &state) < 0)
        goto done;

    result = (strcmp(state, "active") == 0) ? 1 : 0;

done:
    free(state);
    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return result;
}

int systemd_start(const char *unit)
{
    sd_bus         *bus   = open_bus();
    sd_bus_error    error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    int             result = -1;

    if (!bus) return -1;

    if (sd_bus_call_method(bus, SYSTEMD_DEST, SYSTEMD_PATH, MANAGER_IFACE,
                           "StartUnit", &error, &reply,
                           "ss", unit, "replace") >= 0)
        result = 0;

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return result;
}

int systemd_stop(const char *unit)
{
    sd_bus         *bus   = open_bus();
    sd_bus_error    error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    int             result = -1;

    if (!bus) return -1;

    if (sd_bus_call_method(bus, SYSTEMD_DEST, SYSTEMD_PATH, MANAGER_IFACE,
                           "StopUnit", &error, &reply,
                           "ss", unit, "replace") >= 0)
        result = 0;

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return result;
}

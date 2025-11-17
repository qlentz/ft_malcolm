#include "ft_malcolm.h"

int mac_string_to_bytes(const char *mac_str, uint8_t *mac_bytes) {
    return sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                  &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
                  &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]) == 6;
}


void mac_bytes_to_string(const uint8_t *mac_bytes, char *mac_str) {
    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_bytes[0], mac_bytes[1], mac_bytes[2],
            mac_bytes[3], mac_bytes[4], mac_bytes[5]);
}

int is_valid_mac_address(const char *mac_str) {
    int values[6];
    int i;
    int consumed;

    if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x%n",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5], &consumed) != 6) {
        return ERROR; // Invalid format
    }
    if (mac_str[consumed] != '\0') {
        return ERROR; // Trailing characters
    }

    for (i = 0; i < 6; ++i) {
        if (values[i] < 0 || values[i] > 255) {
            return ERROR; // Byte value out of range
        }
    }

    return SUCCESS; // Valid MAC address
}

int mac_equal(uint8_t *m1, uint8_t *m2) {
    int i;

    i = 0;
    while (i < 6) {
        if (m1[i] != m2[i])
            return (ERROR);
        i++;
    }
    return (SUCCESS);
}

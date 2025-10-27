/**
 * @file test_channel_selection.c
 * @brief Test program for MAC-based channel selection algorithm
 * 
 * Compile: gcc -o test_channel_selection test_channel_selection.c
 * Run: ./test_channel_selection
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Select a WiFi channel (1, 6, or 11) based on MAC address
 * 
 * Uses the last byte of the MAC address modulo 3. The last byte
 * typically has good variance and this simple approach provides
 * reasonable distribution for most MAC address patterns.
 * 
 * @param mac 6-byte MAC address
 * @return WiFi channel (1, 6, or 11)
 */
static int select_channel_from_mac(const uint8_t mac[6]) {
    // Use last byte modulo 3
    int channel_index = mac[5] % 3;
    const int channels[] = {1, 6, 11};
    
    return channels[channel_index];
}

/**
 * @brief Parse MAC address from string format "XX:XX:XX:XX:XX:XX"
 */
static int parse_mac(const char *mac_str, uint8_t mac[6]) {
    return sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

int main() {
    // Test MAC addresses provided by user. Change this for actual MAC addresses to test with.
    const char *test_macs[] = {
        "0c:4e:a0:00:00:01",
        "0c:4e:a0:00:00:02",
        "0c:4e:a0:00:00:03"
    };
    
    int num_macs = sizeof(test_macs) / sizeof(test_macs[0]);
    int channel_counts[3] = {0, 0, 0};  // counts for channels 1, 6, 11
    
    printf("Testing MAC-based channel selection algorithm\n");
    printf("==============================================\n\n");
    
    printf("%-20s -> Channel\n", "MAC Address");
    printf("----------------------------------\n");
    
    for (int i = 0; i < num_macs; i++) {
        uint8_t mac[6];
        if (parse_mac(test_macs[i], mac) == 6) {
            int channel = select_channel_from_mac(mac);
            printf("%-20s -> %d\n", test_macs[i], channel);
            
            // Count distribution
            switch (channel) {
                case 1:  channel_counts[0]++; break;
                case 6:  channel_counts[1]++; break;
                case 11: channel_counts[2]++; break;
            }
        } else {
            printf("ERROR: Failed to parse MAC: %s\n", test_macs[i]);
        }
    }
    
    printf("\n");
    printf("Distribution Summary\n");
    printf("====================\n");
    printf("Channel  1: %2d devices (%.1f%%)\n", channel_counts[0], 
           100.0 * channel_counts[0] / num_macs);
    printf("Channel  6: %2d devices (%.1f%%)\n", channel_counts[1],
           100.0 * channel_counts[1] / num_macs);
    printf("Channel 11: %2d devices (%.1f%%)\n", channel_counts[2],
           100.0 * channel_counts[2] / num_macs);
    printf("Total:      %2d devices\n", num_macs);
    
    printf("\n");
    
    // Check if distribution is reasonably even (allow some variance)
    int min_expected = num_macs / 3 - 2;  // Allow ±2 variance
    int max_expected = num_macs / 3 + 2;
    
    int balanced = 1;
    for (int i = 0; i < 3; i++) {
        if (channel_counts[i] < min_expected || channel_counts[i] > max_expected) {
            balanced = 0;
            break;
        }
    }
    
    if (balanced) {
        printf("✓ Distribution is reasonably balanced!\n");
        return 0;
    } else {
        printf("✗ Distribution is unbalanced (expected ~%d per channel)\n", num_macs / 3);
        return 1;
    }
}

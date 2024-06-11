#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s -i <interface> [-m <mac_address> | -r | -gcm | -p]\n", prog_name);
    printf("Options:\n");
    printf("  -i, --interface        Interface to change its MAC address\n");
    printf("  -m, --mac              Set a custom MAC address\n");
    printf("  -r, --random           Generate random MAC address\n");
    printf("  -gcm, --get-current-mac Get current MAC address\n");
    printf("  -p, --permanent        Reset the MAC address to the permanent\n");
}

int validate_mac_address(const char *mac_address) {
    return (mac_address && strlen(mac_address) == 17 && mac_address[2] == ':' && mac_address[5] == ':' && 
            mac_address[8] == ':' && mac_address[11] == ':' && mac_address[14] == ':');
}

void get_current_mac(const char *interface) {
    struct ifreq ifr;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    strcpy(ifr.ifr_name, interface);
    if (ioctl(s, SIOCGIFHWADDR, &ifr) == -1) {
        perror("ioctl");
        close(s);
        exit(EXIT_FAILURE);
    }

    close(s);

    unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    printf("Current MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void generate_random_mac(char *mac) {
    for (int i = 0; i < 6; ++i) {
        sprintf(mac + 3 * i, "%02x", rand() % 256);
        if (i < 5) {
            mac[3 * i + 2] = ':';
        }
    }
}

void change_mac(const char *interface, const char *mac_address) {
    struct ifreq ifr;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    strcpy(ifr.ifr_name, interface);

    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    sscanf(mac_address, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &ifr.ifr_hwaddr.sa_data[0], &ifr.ifr_hwaddr.sa_data[1], &ifr.ifr_hwaddr.sa_data[2],
           &ifr.ifr_hwaddr.sa_data[3], &ifr.ifr_hwaddr.sa_data[4], &ifr.ifr_hwaddr.sa_data[5]);

    if (ioctl(s, SIOCSIFHWADDR, &ifr) == -1) {
        perror("ioctl");
        close(s);
        exit(EXIT_FAILURE);
    }

    close(s);
    printf("MAC address changed to %s\n", mac_address);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char *interface = NULL;
    char *mac_address = NULL;
    int random_mac_flag = 0;
    int get_current_mac_flag = 0;
    int permanent_mac_flag = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interface") == 0) {
            if (i + 1 < argc) {
                interface = argv[++i];
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mac") == 0) {
            if (i + 1 < argc) {
                mac_address = argv[++i];
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--random") == 0) {
            random_mac_flag = 1;
        } else if (strcmp(argv[i], "-gcm") == 0 || strcmp(argv[i], "--get-current-mac") == 0) {
            get_current_mac_flag = 1;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--permanent") == 0) {
            permanent_mac_flag = 1;
        } else {
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (!interface) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (get_current_mac_flag) {
        get_current_mac(interface);
        exit(EXIT_SUCCESS);
    }

    if (random_mac_flag) {
        char random_mac[18] = {0};
        generate_random_mac(random_mac);
        change_mac(interface, random_mac);
    } else if (mac_address) {
        if (!validate_mac_address(mac_address)) {
            fprintf(stderr, "Invalid MAC address format.\n");
            exit(EXIT_FAILURE);
        }
        change_mac(interface, mac_address);
    } else if (permanent_mac_flag) {
        fprintf(stderr, "Permanent MAC address change is not implemented in this example.\n");
        exit(EXIT_FAILURE);
    } else {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}

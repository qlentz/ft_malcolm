#include "ft_malcolm.h"


int	find_inteface(t_sockinfos *sockinfos) {
	struct ifaddrs *ifa;
	struct ifaddrs *tofree;
	
	if (getifaddrs(&ifa) == -1) {
        perror("getifaddrs");
        return (ERROR);
    }
	tofree = ifa;
	while (ifa->ifa_next != NULL) {
			if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) && (ifa->ifa_addr->sa_family == AF_INET) && !(ifa->ifa_flags & IFF_LOOPBACK)) {
				printf("Interface found: %s\n", ifa->ifa_name);
				sockinfos->interface_name = ft_strdup(ifa->ifa_name);
				return (SUCCESS);
			}
		ifa = ifa->ifa_next;
	}
	freeifaddrs(tofree);
	return (ERROR);
}

int	parse_user_inputs(char **av, t_targets * targets) {
	
	if (!inet_pton(AF_INET, av[1], &targets->source_ip) || !inet_pton(AF_INET, av[3], &targets->target_ip))
		return (ERROR);
	else if (!is_valid_mac_address(av[2]) || !is_valid_mac_address(av[4]))
		return (ERROR);
	mac_string_to_bytes(av[2], targets->source_mac);
	mac_string_to_bytes(av[4], targets->target_mac);
	return (SUCCESS);
}

int is_arp_target(arp_packet *arp, t_targets *targets) {
	uint32_t sender_ip = ntohl(arp->arp_hdr.sender_ip);
	if (memcmp(arp->arp_hdr.sender_mac, targets->target_mac, 6) == 0) {
		printf("sender m = target m ");
	}
	if (sender_ip == targets->target_ip) {
		printf("sender ip = target ip ");
	}
	return (0);
}

void print_arp_packet(const arp_packet *packet) {
    printf("Ethernet Header:\n");
    printf("  Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           packet->eth_hdr.dest_mac[0], packet->eth_hdr.dest_mac[1], packet->eth_hdr.dest_mac[2],
           packet->eth_hdr.dest_mac[3], packet->eth_hdr.dest_mac[4], packet->eth_hdr.dest_mac[5]);
    printf("  Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           packet->eth_hdr.src_mac[0], packet->eth_hdr.src_mac[1], packet->eth_hdr.src_mac[2],
           packet->eth_hdr.src_mac[3], packet->eth_hdr.src_mac[4], packet->eth_hdr.src_mac[5]);
    printf("  Ethernet Type: 0x%04x\n", ntohs(packet->eth_hdr.eth_type));

    printf("ARP Header:\n");
    printf("  Hardware Type: %d\n", ntohs(packet->arp_hdr.hw_type));
    printf("  Protocol Type: 0x%04x\n", ntohs(packet->arp_hdr.proto_type));
    printf("  Hardware Size: %d\n", packet->arp_hdr.hw_size);
    printf("  Protocol Size: %d\n", packet->arp_hdr.proto_size);
    printf("  Opcode: %d\n", ntohs(packet->arp_hdr.opcode));
    printf("  Sender MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           packet->arp_hdr.sender_mac[0], packet->arp_hdr.sender_mac[1], packet->arp_hdr.sender_mac[2],
           packet->arp_hdr.sender_mac[3], packet->arp_hdr.sender_mac[4], packet->arp_hdr.sender_mac[5]);
    printf("  Sender IP: %s\n", inet_ntoa(*(struct in_addr *)&packet->arp_hdr.sender_ip));
    printf("  Target MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           packet->arp_hdr.target_mac[0], packet->arp_hdr.target_mac[1], packet->arp_hdr.target_mac[2],
           packet->arp_hdr.target_mac[3], packet->arp_hdr.target_mac[4], packet->arp_hdr.target_mac[5]);
    printf("  Target IP: %s\n", inet_ntoa(*(struct in_addr *)&packet->arp_hdr.target_ip));
}

int main(int ac, char **av)
{
	t_sockinfos	sockinfos;
	unsigned char netbuff[NETBUFFSIZE];
	t_targets targets;

	if (ac != 5) {
		dprintf(2, "Error: Wrong number of arguments\n");
		exit(1);
	}
	if (!parse_user_inputs(av, &targets)) {
		dprintf(2, "Error: Wrong IP(s) or MAC(s)\n");
		exit(1);
	}
	if (!find_inteface(&sockinfos)) {
		dprintf(2, "Error: No network Interface Found\n");
		exit(1);
	}

	int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockfd == -1) {
		dprintf(2, "Error: socket()");
		exit(1);
	}

	while (true) {
		recvfrom(sockfd, netbuff, NETBUFFSIZE, 0, NULL, NULL);
		arp_packet *arp = (arp_packet *)netbuff;
		if (is_arp_request(arp)) {
			print_arp_packet(arp);
			write(1, "rec arp: ", strlen("rec arp: "));
		}
		if (is_arp_target(arp, &targets)) {
			continue;
		}
		printf("\n");
	}
	

	return(0);
}

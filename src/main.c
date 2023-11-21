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
	if (memcmp(arp->arp_hdr.sender_mac, targets->target_mac, 6) == 0)
		printf("sender m = target m ");
	if (sender_ip == targets->target_ip) {
		printf("sender ip = target ip ");
	}
	return (0);
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

		if (is_arp_request(arp))
			write(1, "rec arp: ", strlen("rec arp: "));
		if (is_arp_target(arp, &targets)) {
			continue;
		}
		printf("\n");
	}
	

	return(0);
}

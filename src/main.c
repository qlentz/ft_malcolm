#include "ft_malcolm.h"

static volatile sig_atomic_t	g_stop = 0;

static void	handle_sigint(int signum)
{
	(void)signum;
	g_stop = 1;
}

unsigned int	get_interface_index(const char *interface_name)
{
	unsigned int	ifindex;

	ifindex = if_nametoindex(interface_name);
	if (ifindex == 0)
		perror("if_nametoindex");
	return (ifindex);
}

int	bind_socket_to_interface(t_sockinfos *sock, const char *interface)
{
	struct ifreq	ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
	if (setsockopt(sock->sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr,
			sizeof(ifr)) < 0)
	{
		perror("setsockopt SO_BINDTODEVICE");
		return (ERROR);
	}
	return (SUCCESS);
}

int	find_inteface(t_sockinfos *sockinfos)
{
	struct ifaddrs	*ifa;
	struct ifaddrs	*cursor;
	int				result;
	unsigned int	ifindex;

	if (getifaddrs(&ifa) == -1)
	{
		perror("getifaddrs");
		return (ERROR);
	}
	result = ERROR;
	cursor = ifa;
	while (cursor)
	{
		if ((cursor->ifa_flags & (IFF_UP | IFF_RUNNING))
			&& !(cursor->ifa_flags & IFF_LOOPBACK)
			&& cursor->ifa_addr
			&& cursor->ifa_addr->sa_family == AF_PACKET)
		{
			ifindex = get_interface_index(cursor->ifa_name);
			if (ifindex == 0)
			{
				cursor = cursor->ifa_next;
				continue ;
			}
			sockinfos->if_index = ifindex;
			if (bind_socket_to_interface(sockinfos, cursor->ifa_name))
			{
				printf("Interface found: %s\n", cursor->ifa_name);
				result = SUCCESS;
				break ;
			}
		}
		cursor = cursor->ifa_next;
	}
	freeifaddrs(ifa);
	return (result);
}

int	parse_user_inputs(char **av, t_targets *targets)
{
	if (!inet_pton(AF_INET, av[1], &targets->source_ip)
		|| !inet_pton(AF_INET, av[3], &targets->target_ip))
		return (ERROR);
	if (!is_valid_mac_address(av[2]) || !is_valid_mac_address(av[4]))
		return (ERROR);
	mac_string_to_bytes(av[2], targets->source_mac);
	mac_string_to_bytes(av[4], targets->target_mac);
	return (SUCCESS);
}

int	main(int ac, char **av)
{
	t_sockinfos		sockinfos;
	unsigned char	netbuff[NETBUFFSIZE];
	t_targets		targets;
	arp_packet		*arp;
	struct sigaction sa;
	ssize_t			bytes;

	if (ac != 5)
	{
		dprintf(2, "Error: Wrong number of arguments\n");
		return (1);
	}
	if (!parse_user_inputs(av, &targets))
	{
		dprintf(2, "Error: Wrong IP(s) or MAC(s)\n");
		return (1);
	}
	sockinfos.sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockinfos.sock == -1)
	{
		perror("socket");
		return (1);
	}
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigint;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("sigaction");
		close(sockinfos.sock);
		return (1);
	}
	if (!find_inteface(&sockinfos))
	{
		dprintf(2, "Error: No network Interface Found\n");
		close(sockinfos.sock);
		return (1);
	}
	printf("Waiting for ARP request from the target\n");
	while (!g_stop)
	{
		bytes = recvfrom(sockinfos.sock, netbuff, NETBUFFSIZE, 0, NULL, NULL);
		if (bytes < 0)
		{
			if (errno == EINTR)
				continue ;
			perror("recvfrom");
			close(sockinfos.sock);
			return (1);
		}
		if ((size_t)bytes < sizeof(arp_packet))
			continue ;
		arp = (arp_packet *)netbuff;
		if (!is_arp_request(arp))
			continue ;
		print_arp_packet(arp);
		if (is_arp_target(arp, &targets))
			break ;
	}
	if (g_stop)
	{
		close(sockinfos.sock);
		return (0);
	}
	arp_packet reply;
	create_arp_reply(&reply, &targets);
	struct sockaddr_ll socket_address;

	memset(&socket_address, 0, sizeof(struct sockaddr_ll));
	socket_address.sll_family = AF_PACKET;
	socket_address.sll_ifindex = sockinfos.if_index;
	socket_address.sll_halen = ETH_ALEN;
	socket_address.sll_protocol = htons(ETH_P_ARP);
	memcpy(socket_address.sll_addr, targets.target_mac, ETH_ALEN);
	bytes = sendto(sockinfos.sock, &reply, sizeof(reply), 0,
			(struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
	if (bytes < 0 || (size_t)bytes != sizeof(reply))
	{
		perror("sendto failed");
		close(sockinfos.sock);
		return (1);
	}
	printf("ARP reply sent.\n");
	print_arp_packet(&reply);
	close(sockinfos.sock);
	return (0);
}

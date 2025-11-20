#include "ft_malcolm.h"

static volatile sig_atomic_t	g_stop = 0;

static void	handle_sigint(int signum)
{
	(void)signum;
	g_stop = 1;
}

static int	open_raw_socket(t_sockinfos *sockinfos)
{
	sockinfos->sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockinfos->sock == -1)
	{
		perror("socket");
		return (ERROR);
	}
	return (SUCCESS);
}

static int	configure_signal(void)
{
	struct sigaction	sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigint;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("sigaction");
		return (ERROR);
	}
	return (SUCCESS);
}

static int	process_incoming_packet(unsigned char *netbuff, ssize_t bytes,
		t_targets *targets)
{
	arp_packet	*arp;

	if ((size_t)bytes < sizeof(arp_packet))
		return (ERROR);
	arp = (arp_packet *)netbuff;
	if (!is_arp_request(arp))
		return (ERROR);
	print_arp_packet(arp);
	if (is_arp_target(arp, targets))
		return (SUCCESS);
	return (ERROR);
}

static int	wait_for_target_request(t_sockinfos *sockinfos, t_targets *targets)
{
	unsigned char	netbuff[NETBUFFSIZE];
	ssize_t			bytes;

	while (!g_stop)
	{
		bytes = recvfrom(sockinfos->sock, netbuff, NETBUFFSIZE, 0, NULL, NULL);
		if (bytes < 0)
		{
			if (errno == EINTR)
				continue ;
			perror("recvfrom");
			return (ERROR);
		}
		if (process_incoming_packet(netbuff, bytes, targets))
			return (SUCCESS);
	}
	return (ERROR);
}

static int	send_arp_reply(t_sockinfos *sockinfos, t_targets *targets)
{
	arp_packet			reply;
	struct sockaddr_ll	socket_address;
	ssize_t				bytes;

	create_arp_reply(&reply, targets);
	memset(&socket_address, 0, sizeof(struct sockaddr_ll));
	socket_address.sll_family = AF_PACKET;
	socket_address.sll_ifindex = sockinfos->if_index;
	socket_address.sll_halen = ETH_ALEN;
	socket_address.sll_protocol = htons(ETH_P_ARP);
	memcpy(socket_address.sll_addr, targets->target_mac, ETH_ALEN);
	bytes = sendto(sockinfos->sock, &reply, sizeof(reply), 0,
			(struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
	if (bytes < 0 || (size_t)bytes != sizeof(reply))
	{
		perror("sendto failed");
		return (ERROR);
	}
	printf("ARP reply sent.\n");
	print_arp_packet(&reply);
	return (SUCCESS);
}

static int	print_error(const char *message)
{
	dprintf(2, "%s\n", message);
	return (1);
}

static int	close_and_return(t_sockinfos *sockinfos, int status)
{
	close(sockinfos->sock);
	return (status);
}

static int	interface_error(t_sockinfos *sockinfos)
{
	print_error("Error: No network Interface Found");
	return (close_and_return(sockinfos, 1));
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

int iterate_interfaces(t_sockinfos *sockinfos, struct ifaddrs *ifa) {
	struct ifaddrs	*cursor;
	unsigned int	ifindex;
	int				result;


	cursor = ifa;
	ifindex = 0;
	result = ERROR;
	while (cursor)
	{
		if ((cursor->ifa_flags & (IFF_UP | IFF_RUNNING))
			&& !(cursor->ifa_flags & IFF_LOOPBACK) && cursor->ifa_addr
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
	return (result);
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
			&& !(cursor->ifa_flags & IFF_LOOPBACK) && cursor->ifa_addr
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
	if (!inet_pton(AF_INET, av[1], &targets->source_ip) || !inet_pton(AF_INET,
			av[3], &targets->target_ip))
		return (ERROR);
	if (!is_valid_mac_address(av[2]) || !is_valid_mac_address(av[4]))
		return (ERROR);
	mac_string_to_bytes(av[2], targets->source_mac);
	mac_string_to_bytes(av[4], targets->target_mac);
	return (SUCCESS);
}

int	main(int ac, char **av)
{
	t_sockinfos	sockinfos;
	t_targets	targets;

	if (ac != 5)
		return (print_error("Error: Wrong number of arguments"));
	if (!parse_user_inputs(av, &targets))
		return (print_error("Error: Wrong IP(s) or MAC(s)"));
	if (!open_raw_socket(&sockinfos))
		return (1);
	if (!configure_signal())
		return (close_and_return(&sockinfos, 1));
	if (!find_inteface(&sockinfos))
		return (interface_error(&sockinfos));
	printf("Waiting for ARP request from the target\n");
	if (!wait_for_target_request(&sockinfos, &targets))
		return (close_and_return(&sockinfos, g_stop ? 0 : 1));
	if (!send_arp_reply(&sockinfos, &targets))
		return (close_and_return(&sockinfos, 1));
	close(sockinfos.sock);
	return (0);
}

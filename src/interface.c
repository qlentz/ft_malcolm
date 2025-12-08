#include <ft_malcolm.h>

static unsigned int	get_interface_index(const char *interface_name)
{
	unsigned int	ifindex;

	ifindex = if_nametoindex(interface_name);
	if (ifindex == 0)
		perror("if_nametoindex");
	return (ifindex);
}

static bool	is_candidate_interface(struct ifaddrs *ifa)
{
	return ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING))
		&& !(ifa->ifa_flags & IFF_LOOPBACK) && ifa->ifa_addr
		&& ifa->ifa_addr->sa_family == AF_PACKET);
}

static int	bind_socket_to_interface(t_sockinfos *sock, const char *interface)
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

static int	iterate_interfaces(t_sockinfos *sockinfos, struct ifaddrs *ifa)
{
	struct ifaddrs	*cursor;
	unsigned int	ifindex;

	for (cursor = ifa; cursor; cursor = cursor->ifa_next)
	{
		if (!is_candidate_interface(cursor))
			continue ;
		ifindex = get_interface_index(cursor->ifa_name);
		if (ifindex == 0)
			continue ;
		sockinfos->if_index = ifindex;
		if (bind_socket_to_interface(sockinfos, cursor->ifa_name) != SUCCESS)
			continue ;
		printf("Interface found: %s\n", cursor->ifa_name);
		return (SUCCESS);
	}
	return (ERROR);
}

int	find_inteface(t_sockinfos *sockinfos)
{
	struct ifaddrs	*ifa;
	int				result;

	if (getifaddrs(&ifa) == -1)
	{
		perror("getifaddrs");
		return (ERROR);
	}
	result = ERROR;
	result = iterate_interfaces(sockinfos, ifa);
	freeifaddrs(ifa);
	return (result);
}

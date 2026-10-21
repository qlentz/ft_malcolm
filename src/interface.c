/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   interface.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 12:04:16 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <ft_malcolm.h>

static unsigned int	get_interface_index(const char *interface_name)
{
	unsigned int	ifindex;

	ifindex = if_nametoindex(interface_name);
	if (ifindex == 0)
		dprintf(2, "if_nametoindex: %s\n", strerror(errno));
	return (ifindex);
}

static bool	is_candidate_interface(struct ifaddrs *ifa)
{
	const struct sockaddr_ll	*address;

	if (!ifa->ifa_name || !ifa->ifa_addr
		|| ifa->ifa_addr->sa_family != AF_PACKET
		|| (ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING)
		|| (ifa->ifa_flags & IFF_LOOPBACK))
		return (false);
	address = (const struct sockaddr_ll *)ifa->ifa_addr;
	return (address->sll_hatype == ARPHRD_ETHER
		&& address->sll_halen == ETH_ALEN);
}

static int	iterate_interfaces(t_sockinfos *sockinfos, struct ifaddrs *ifa,
		int wanted_index)
{
	unsigned int	ifindex;

	while (ifa)
	{
		if (is_candidate_interface(ifa))
		{
			ifindex = get_interface_index(ifa->ifa_name);
			if (ifindex && (!wanted_index || ifindex == (unsigned)wanted_index))
			{
				if (wanted_index)
				{
					sockinfos->if_index = ifindex;
					printf("Interface selected: %s\n", ifa->ifa_name);
				}
				return (SUCCESS);
			}
		}
		ifa = ifa->ifa_next;
	}
	return (ERROR);
}

int	find_inteface(t_sockinfos *sockinfos, int if_index)
{
	struct ifaddrs	*ifa;
	int				result;

	if (getifaddrs(&ifa) == -1)
	{
		dprintf(2, "getifaddrs: %s\n", strerror(errno));
		return (-1);
	}
	result = iterate_interfaces(sockinfos, ifa, if_index);
	freeifaddrs(ifa);
	return (result);
}

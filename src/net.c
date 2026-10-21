/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   net.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 12:04:16 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <ft_malcolm.h>

static int	is_valid_packet_source(const struct sockaddr_ll *source,
		socklen_t source_len)
{
	return (source_len >= offsetof(struct sockaddr_ll, sll_addr) + ETH_ALEN
		&& source->sll_family == AF_PACKET && source->sll_ifindex > 0
		&& source->sll_hatype == ARPHRD_ETHER && source->sll_halen == ETH_ALEN
		&& source->sll_protocol == htons(ETH_P_ARP)
		&& source->sll_pkttype == PACKET_BROADCAST);
}

int	process_incoming_packet(unsigned char *netbuff, ssize_t bytes,
		t_targets *targets)
{
	arp_packet	arp;

	if (!netbuff || bytes < 0 || (size_t)bytes < sizeof(arp_packet))
		return (ERROR);
	ft_memcpy(&arp, netbuff, sizeof(arp));
	if (!is_arp_request(&arp))
		return (ERROR);
	if (is_arp_target(&arp, targets))
		return (SUCCESS);
	return (ERROR);
}

int	receive_target_packet(t_sockinfos *sockinfos, t_targets *targets)
{
	unsigned char		netbuff[NETBUFFSIZE];
	struct sockaddr_ll	source;
	socklen_t			source_len;
	ssize_t				bytes;

	ft_memset(&source, 0, sizeof(source));
	source_len = sizeof(source);
	bytes = recvfrom(sockinfos->sock, netbuff, sizeof(netbuff), 0,
			(struct sockaddr *)&source, &source_len);
	if (bytes < 0)
	{
		if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			return (ERROR);
		dprintf(2, "recvfrom: %s\n", strerror(errno));
		return (-1);
	}
	if (!is_valid_packet_source(&source, source_len)
		|| !process_incoming_packet(netbuff, bytes, targets))
		return (ERROR);
	return (find_inteface(sockinfos, source.sll_ifindex));
}

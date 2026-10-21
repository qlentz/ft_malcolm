/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reply.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 12:04:16 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malcolm.h"

void	create_arp_reply(t_arp_packet *reply, const t_targets *tar)
{
	ft_memcpy(reply->eth_hdr.dest_mac, tar->target_mac, 6);
	ft_memcpy(reply->eth_hdr.src_mac, tar->source_mac, 6);
	reply->eth_hdr.eth_type = htons(ETH_P_ARP);
	reply->arp_hdr.hw_type = htons(ARPHRD_ETHER);
	reply->arp_hdr.proto_type = htons(ETH_P_IP);
	reply->arp_hdr.hw_size = ETH_ALEN;
	reply->arp_hdr.proto_size = sizeof(uint32_t);
	reply->arp_hdr.opcode = htons(ARPOP_REPLY);
	ft_memcpy(reply->arp_hdr.sender_mac, tar->source_mac, 6);
	reply->arp_hdr.sender_ip = tar->source_ip;
	ft_memcpy(reply->arp_hdr.target_mac, tar->target_mac, 6);
	reply->arp_hdr.target_ip = tar->target_ip;
}

static int	check_send_result(ssize_t bytes, size_t expected)
{
	if (bytes < 0)
	{
		dprintf(2, "sendto failed: %s\n", strerror(errno));
		return (ERROR);
	}
	if ((size_t)bytes != expected)
	{
		dprintf(2, "sendto: incomplete ARP reply (%zd/%zu bytes)\n",
			bytes, expected);
		return (ERROR);
	}
	return (SUCCESS);
}

int	send_arp_reply(t_sockinfos *sockinfos, t_targets *targets)
{
	t_arp_packet		reply;
	struct sockaddr_ll	socket_address;
	ssize_t				bytes;

	create_arp_reply(&reply, targets);
	ft_memset(&socket_address, 0, sizeof(struct sockaddr_ll));
	socket_address.sll_family = AF_PACKET;
	socket_address.sll_ifindex = sockinfos->if_index;
	socket_address.sll_halen = ETH_ALEN;
	socket_address.sll_protocol = htons(ETH_P_ARP);
	ft_memcpy(socket_address.sll_addr, targets->target_mac, ETH_ALEN);
	bytes = sendto(sockinfos->sock, &reply, sizeof(reply), 0,
			(struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll));
	if (!check_send_result(bytes, sizeof(reply)))
		return (ERROR);
	printf("ARP reply sent.\n");
	return (SUCCESS);
}

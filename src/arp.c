/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arp.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 12:04:16 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malcolm.h"

static int	is_broadcast_mac(const uint8_t *mac)
{
	int	i;

	i = 0;
	while (i < ETH_ALEN)
	{
		if (mac[i] != 0xff)
			return (ERROR);
		i++;
	}
	return (SUCCESS);
}

int	is_arp_request(arp_packet *pack)
{
	return (pack && ntohs(pack->eth_hdr.eth_type) == ETH_P_ARP
		&& is_broadcast_mac(pack->eth_hdr.dest_mac)
		&& ntohs(pack->arp_hdr.hw_type) == ARPHRD_ETHER
		&& ntohs(pack->arp_hdr.proto_type) == ETH_P_IP
		&& pack->arp_hdr.hw_size == ETH_ALEN
		&& pack->arp_hdr.proto_size == sizeof(uint32_t)
		&& ntohs(pack->arp_hdr.opcode) == ARPOP_REQUEST
		&& ft_memcmp(pack->eth_hdr.src_mac, pack->arp_hdr.sender_mac,
			ETH_ALEN) == 0);
}

int	is_arp_target(arp_packet *arp, t_targets *targets)
{
	if (ft_memcmp(arp->arp_hdr.sender_mac, targets->target_mac, 6) == 0
		&& arp->arp_hdr.sender_ip == targets->target_ip
		&& arp->arp_hdr.target_ip == targets->source_ip)
	{
		printf("ARP request received\n");
		return (1);
	}
	return (0);
}

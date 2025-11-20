#include "ft_malcolm.h"

int	is_arp_request(arp_packet *pack)
{
	if (ntohs(pack->arp_hdr.opcode) == 1
		&& ntohs(pack->eth_hdr.eth_type) == ETH_P_ARP)
		return (SUCCESS);
	return (ERROR);
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

void	create_arp_reply(arp_packet *reply, const t_targets *tar)
{
	memcpy(reply->eth_hdr.dest_mac, tar->target_mac, 6);
	memcpy(reply->eth_hdr.src_mac, tar->source_mac, 6);
	reply->eth_hdr.eth_type = htons(ETH_P_ARP);
	reply->arp_hdr.hw_type = htons(1);
	reply->arp_hdr.proto_type = htons(ETH_P_IP);
	reply->arp_hdr.hw_size = 6;
	reply->arp_hdr.proto_size = 4;
	reply->arp_hdr.opcode = htons(2);
	memcpy(reply->arp_hdr.sender_mac, tar->source_mac, 6);
	reply->arp_hdr.sender_ip = tar->source_ip;
	memcpy(reply->arp_hdr.target_mac, tar->target_mac, 6);
	reply->arp_hdr.target_ip = tar->target_ip;
}

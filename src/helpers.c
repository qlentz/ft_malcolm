#include <ft_malcolm.h>

void	print_arp_packet(const arp_packet *packet)
{
	char	sender_ip[INET_ADDRSTRLEN];
	char	target_ip[INET_ADDRSTRLEN];

	if (!inet_ntop(AF_INET, &packet->arp_hdr.sender_ip, sender_ip,
			sizeof(sender_ip)))
	{
		strncpy(sender_ip, "invalid", sizeof(sender_ip));
		sender_ip[sizeof(sender_ip) - 1] = '\0';
	}
	if (!inet_ntop(AF_INET, &packet->arp_hdr.target_ip, target_ip,
			sizeof(target_ip)))
	{
		strncpy(target_ip, "invalid", sizeof(target_ip));
		target_ip[sizeof(target_ip) - 1] = '\0';
	}
	printf("Ethernet Header:\\n");
	printf("  Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\\n",
		packet->eth_hdr.dest_mac[0], packet->eth_hdr.dest_mac[1],
		packet->eth_hdr.dest_mac[2], packet->eth_hdr.dest_mac[3],
		packet->eth_hdr.dest_mac[4], packet->eth_hdr.dest_mac[5]);
	printf("  Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\\n",
		packet->eth_hdr.src_mac[0], packet->eth_hdr.src_mac[1],
		packet->eth_hdr.src_mac[2], packet->eth_hdr.src_mac[3],
		packet->eth_hdr.src_mac[4], packet->eth_hdr.src_mac[5]);
	printf("  Ethernet Type: 0x%04x\\n", ntohs(packet->eth_hdr.eth_type));
	printf("ARP Header:\\n");
	printf("  Hardware Type: %d\\n", ntohs(packet->arp_hdr.hw_type));
	printf("  Protocol Type: 0x%04x\\n", ntohs(packet->arp_hdr.proto_type));
	printf("  Hardware Size: %d\\n", packet->arp_hdr.hw_size);
	printf("  Protocol Size: %d\\n", packet->arp_hdr.proto_size);
	printf("  Opcode: %d\\n", ntohs(packet->arp_hdr.opcode));
	printf("  Sender MAC: %02x:%02x:%02x:%02x:%02x:%02x\\n",
		packet->arp_hdr.sender_mac[0], packet->arp_hdr.sender_mac[1],
		packet->arp_hdr.sender_mac[2], packet->arp_hdr.sender_mac[3],
		packet->arp_hdr.sender_mac[4], packet->arp_hdr.sender_mac[5]);
	printf("  Sender IP: %s\\n", sender_ip);
	printf("  Target MAC: %02x:%02x:%02x:%02x:%02x:%02x\\n",
		packet->arp_hdr.target_mac[0], packet->arp_hdr.target_mac[1],
		packet->arp_hdr.target_mac[2], packet->arp_hdr.target_mac[3],
		packet->arp_hdr.target_mac[4], packet->arp_hdr.target_mac[5]);
	printf("  Target IP: %s\\n", target_ip);
}

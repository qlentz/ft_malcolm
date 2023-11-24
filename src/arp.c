#include "ft_malcolm.h"

int	is_arp_request(arp_packet *pack) {
	if (ntohs(pack->arp_hdr.opcode) == 1 && ntohs(pack->eth_hdr.eth_type) == ETH_P_ARP)
		return (SUCCESS);
	return (ERROR);
}

int is_arp_target(arp_packet *arp, t_targets *targets) {
	if (ft_memcmp(arp->arp_hdr.sender_mac, targets->target_mac, 6) == 0 && 
		arp->arp_hdr.sender_ip == targets->target_ip &&
              arp->arp_hdr.target_ip == targets->source_ip) {
			printf("ARP request received\n");
			return (1);
	}
	return (0);
}

void print_arp_packet(const arp_packet *packet) {
    printf("Ethernet Header:\n");
    printf("  Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           packet->eth_hdr.dest_mac[0], packet->eth_hdr.dest_mac[1], packet->eth_hdr.dest_mac[2],
           packet->eth_hdr.dest_mac[3], packet->eth_hdr.dest_mac[4], packet->eth_hdr.dest_mac[5]);
    printf("  Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           packet->eth_hdr.src_mac[0], packet->eth_hdr.src_mac[1], packet->eth_hdr.src_mac[2],
           packet->eth_hdr.src_mac[3], packet->eth_hdr.src_mac[4], packet->eth_hdr.src_mac[5]);
    printf("  Ethernet Type: 0x%04x\n", ntohs(packet->eth_hdr.eth_type));

    printf("ARP Header:\n");
    printf("  Hardware Type: %d\n", ntohs(packet->arp_hdr.hw_type));
    printf("  Protocol Type: 0x%04x\n", ntohs(packet->arp_hdr.proto_type));
    printf("  Hardware Size: %d\n", packet->arp_hdr.hw_size);
    printf("  Protocol Size: %d\n", packet->arp_hdr.proto_size);
    printf("  Opcode: %d\n", ntohs(packet->arp_hdr.opcode));
    printf("  Sender MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           packet->arp_hdr.sender_mac[0], packet->arp_hdr.sender_mac[1], packet->arp_hdr.sender_mac[2],
           packet->arp_hdr.sender_mac[3], packet->arp_hdr.sender_mac[4], packet->arp_hdr.sender_mac[5]);
    printf("  Sender IP: %s\n", inet_ntoa(*(struct in_addr *)&packet->arp_hdr.sender_ip));
    printf("  Target MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           packet->arp_hdr.target_mac[0], packet->arp_hdr.target_mac[1], packet->arp_hdr.target_mac[2],
           packet->arp_hdr.target_mac[3], packet->arp_hdr.target_mac[4], packet->arp_hdr.target_mac[5]);
    printf("  Target IP: %s\n", inet_ntoa(*(struct in_addr *)&packet->arp_hdr.target_ip));
}

void create_arp_reply(arp_packet *reply, const t_targets *tar) {
    arp_packet
    memcpy(reply->eth_hdr.dest_mac, tar->target_mac, 6);  // Target MAC
    memcpy(reply->eth_hdr.src_mac, tar->source_mac, 6);       // My MAC
    reply->eth_hdr.eth_type = htons(ETH_P_ARP);

    // Set ARP header
    reply->arp_hdr.hw_type = htons(1);               // Ethernet
    reply->arp_hdr.proto_type = htons(ETH_P_IP);     // IP
    reply->arp_hdr.hw_size = 6;                      // MAC size
    reply->arp_hdr.proto_size = 4;                   // IP size
    reply->arp_hdr.opcode = htons(2);                // ARP reply

    memcpy(reply->arp_hdr.sender_mac, tar->source_mac, 6);    // My MAC
    reply->arp_hdr.sender_ip = tar->source_ip;                // My IP
    memcpy(reply->arp_hdr.target_mac, tar->target_mac, 6);// Target MAC
    reply->arp_hdr.target_ip = tar->target_ip;            // Target IP
}

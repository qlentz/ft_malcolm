#ifndef ARP_H
#define ARP_H

	#include <stdint.h>
	
	// Ethernet header
	typedef struct eth_header {
		uint8_t  dest_mac[6];  // 	Dest MAC address
		uint8_t  src_mac[6];   // 	Src MAC address
		uint16_t eth_type;     // 	Ethernet Type
	} __attribute__((packed)) eth_header; // 	Pour que le compilo ajoute pas de padding

	// ARP header
	typedef struct arp_header {
		uint16_t hw_type;      	// 	Hardware Type
		uint16_t proto_type;   	// 	Protocol Type
		uint8_t  hw_size;      	// 	Hardware Size
		uint8_t  proto_size;   	// 	Protocol Size
		uint16_t opcode;       	// 	ARP Opcode
		uint8_t  sender_mac[6];	// 	Sender's MAC address
		uint32_t sender_ip;    	// 	Sender's IP address
		uint8_t  target_mac[6]; // 	Target's MAC address
		uint32_t target_ip;    	// 	Target's IP address
	} __attribute__((packed)) arp_header; 	

	// ARP packet
	typedef struct arp_packet {
		struct eth_header eth_hdr; // Ethernet Header
		struct arp_header arp_hdr; // ARP Header
	} __attribute__((packed)) arp_packet;

#endif
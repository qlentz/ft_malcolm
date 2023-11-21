#ifndef MALCOLM_H
#define MALCOLM_H

#	include <sys/socket.h>
#	include "arp.h"
#	include <stdint.h>
#	include <stdint.h>
#	include <sys/socket.h>
#	include <ifaddrs.h>
#	include <stdlib.h>
#	include <stdio.h>
#	include <net/if.h> 
#	include <sys/ioctl.h>
#	include "libft.h"
#	include <arpa/inet.h>
#	include <stdbool.h>
#	include <linux/if_ether.h>
# 	define SUCCESS		1
#	define ERROR		0
#	define NETBUFFSIZE 1500 //MTU par defaut sur bcp de systems

typedef struct s_sockinfos {
	char 	*interface_name;
	int		sock;
}				t_sockinfos;

typedef struct s_targets {
	uint32_t	target_ip;
	uint32_t	source_ip;
	uint8_t		target_mac[6];
	uint8_t		source_mac[6];
}				t_targets;

int		mac_string_to_bytes(const char *mac_str, uint8_t *mac_bytes);
void	mac_bytes_to_string(const uint8_t *mac_bytes, char *mac_str);
int		is_valid_mac_address(const char *mac_str);
int		mac_equal(uint8_t *m1, uint8_t *m2);
int		is_arp_request(struct arp_packet *pack);
int		is_arp_target(arp_packet *arp, t_targets *targets);
void	print_arp_packet(const arp_packet *packet);
#endif
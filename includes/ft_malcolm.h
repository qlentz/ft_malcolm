#ifndef MALCOLM_H
#define MALCOLM_H

# include <arpa/inet.h>
# include <errno.h>
# include <ifaddrs.h>
# include <linux/if_ether.h>
# include <linux/if_packet.h>
# include <net/if.h>
# include <signal.h>
# include <stdbool.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/ioctl.h>
# include <sys/socket.h>
# include <unistd.h>

# include "arp.h"
# include "libft.h"

# define SUCCESS				1
# define ERROR					0
# define NETBUFFSIZE			1500

typedef struct s_sockinfos {
	int		sock;
	int		if_index;
}				t_sockinfos;

typedef struct s_targets {
	uint32_t	target_ip;
	uint32_t	source_ip;
	uint8_t		target_mac[6];
	uint8_t		source_mac[6];
}				t_targets;

int				mac_string_to_bytes(const char *mac_str, uint8_t *mac_bytes);
void			mac_bytes_to_string(const uint8_t *mac_bytes, char *mac_str);
int				is_valid_mac_address(const char *mac_str);
int				mac_equal(uint8_t *m1, uint8_t *m2);
int				is_arp_request(struct arp_packet *pack);
int				is_arp_target(arp_packet *arp, t_targets *targets);
void			print_arp_packet(const arp_packet *packet);
void			create_arp_reply(arp_packet *reply, const t_targets *tar);
unsigned int	get_interface_index(const char *interface_name);
int				bind_socket_to_interface(t_sockinfos *sock, const char *interface);
int				find_inteface(t_sockinfos *sockinfos);
int				parse_user_inputs(char **av, t_targets *targets);

#endif

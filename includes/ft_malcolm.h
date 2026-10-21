/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malcolm.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 17:02:08 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALCOLM_H
# define FT_MALCOLM_H

# include <arpa/inet.h>
# include <errno.h>
# include <ifaddrs.h>
# include <netinet/if_ether.h>
# include <linux/if_packet.h>
# include <net/if.h>
# include <net/if_arp.h>
# include <signal.h>
# include <stdbool.h>
# include <stdint.h>
# include <stddef.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <unistd.h>

# include "arp.h"
# include "libft.h"

# define SUCCESS				1
# define ERROR					0
# define NETBUFFSIZE			1500

typedef struct s_sockinfos
{
	int		sock;
	int		if_index;
}				t_sockinfos;

typedef struct s_targets
{
	uint32_t	target_ip;
	uint32_t	source_ip;
	uint8_t		target_mac[6];
	uint8_t		source_mac[6];
}				t_targets;

int				mac_string_to_bytes(const char *mac_str, uint8_t *mac_bytes);
void			mac_bytes_to_string(const uint8_t *mac_bytes, char *mac_str);
int				is_valid_mac_address(const char *mac_str);
int				mac_equal(uint8_t *m1, uint8_t *m2);
int				is_arp_request(t_arp_packet *pack);
int				is_arp_target(t_arp_packet *arp, t_targets *targets);
void			print_arp_packet(const t_arp_packet *packet);
void			create_arp_reply(t_arp_packet *reply, const t_targets *tar);
int				open_raw_socket(t_sockinfos *sockinfos);
int				print_error(const char *message);
int				close_and_return(t_sockinfos *sockinfos, int status);
int				interface_error(t_sockinfos *sockinfos);
/* Index 0 checks availability; a positive index selects the receiving adapter.
 * Result: 1 = usable, 0 = no match, -1 = enumeration error.
 */
int				find_inteface(t_sockinfos *sockinfos, int if_index);
int				receive_target_packet(t_sockinfos *sockinfos,
					t_targets *targets);
int				parse_user_inputs(char **av, t_targets *targets);
int				send_arp_reply(t_sockinfos *sockinfos, t_targets *targets);
int				process_incoming_packet(unsigned char *netbuff, ssize_t bytes,
					t_targets *targets);

#endif

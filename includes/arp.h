/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arp.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 17:02:08 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ARP_H
# define ARP_H

# include <stdint.h>

typedef struct s_eth_header
{
	uint8_t		dest_mac[6];
	uint8_t		src_mac[6];
	uint16_t	eth_type;
} __attribute__((packed))	t_eth_header;

typedef struct s_arp_header
{
	uint16_t	hw_type;
	uint16_t	proto_type;
	uint8_t		hw_size;
	uint8_t		proto_size;
	uint16_t	opcode;
	uint8_t		sender_mac[6];
	uint32_t	sender_ip;
	uint8_t		target_mac[6];
	uint32_t	target_ip;
} __attribute__((packed))	t_arp_header;

typedef struct s_arp_packet
{
	t_eth_header	eth_hdr;
	t_arp_header	arp_hdr;
} __attribute__((packed))	t_arp_packet;

#endif

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 17:02:08 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <ft_malcolm.h>

void	print_arp_packet(const t_arp_packet *packet)
{
	char	sender_ip[INET_ADDRSTRLEN];
	char	target_ip[INET_ADDRSTRLEN];

	if (!inet_ntop(AF_INET, &packet->arp_hdr.sender_ip, sender_ip,
			sizeof(sender_ip)))
	{
		ft_strlcpy(sender_ip, "invalid", sizeof(sender_ip));
	}
	if (!inet_ntop(AF_INET, &packet->arp_hdr.target_ip, target_ip,
			sizeof(target_ip)))
	{
		ft_strlcpy(target_ip, "invalid", sizeof(target_ip));
	}
}

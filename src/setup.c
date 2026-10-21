/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   setup.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 17:02:08 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malcolm.h"

int	open_raw_socket(t_sockinfos *sockinfos)
{
	struct timeval	timeout;

	sockinfos->if_index = 0;
	sockinfos->sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (sockinfos->sock == -1)
	{
		dprintf(2, "socket: %s\n", strerror(errno));
		return (ERROR);
	}
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (setsockopt(sockinfos->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout,
			sizeof(timeout)) == -1)
	{
		dprintf(2, "setsockopt SO_RCVTIMEO: %s\n", strerror(errno));
		close(sockinfos->sock);
		return (ERROR);
	}
	return (SUCCESS);
}

int	parse_user_inputs(char **av, t_targets *targets)
{
	if (inet_pton(AF_INET, av[1], &targets->source_ip) != 1
		|| inet_pton(AF_INET, av[3], &targets->target_ip) != 1)
		return (ERROR);
	if (!mac_string_to_bytes(av[2], targets->source_mac)
		|| !mac_string_to_bytes(av[4], targets->target_mac))
		return (ERROR);
	return (SUCCESS);
}

int	print_error(const char *message)
{
	dprintf(2, "%s\n", message);
	return (1);
}

int	close_and_return(t_sockinfos *sockinfos, int status)
{
	close(sockinfos->sock);
	return (status);
}

int	interface_error(t_sockinfos *sockinfos)
{
	print_error("Error: No network Interface Found");
	return (close_and_return(sockinfos, 1));
}

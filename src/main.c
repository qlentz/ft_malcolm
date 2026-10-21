/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 17:02:08 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malcolm.h"

static volatile sig_atomic_t	*stop_flag(void)
{
	static volatile sig_atomic_t	stop = 0;

	return (&stop);
}

static void	handle_sigint(int signum)
{
	(void)signum;
	*stop_flag() = 1;
}

/* Linux uses an all-zero representation for an empty signal mask. */
static int	configure_signal(void)
{
	struct sigaction	sa;

	ft_memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigint;
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		dprintf(2, "sigaction: %s\n", strerror(errno));
		return (ERROR);
	}
	return (SUCCESS);
}

static int	wait_for_target_request(t_sockinfos *sockinfos, t_targets *targets)
{
	int	result;

	while (!*stop_flag())
	{
		result = receive_target_packet(sockinfos, targets);
		if (result < 0)
			return (ERROR);
		if (result == SUCCESS)
			return (SUCCESS);
	}
	return (ERROR);
}

int	main(int ac, char **av)
{
	t_sockinfos	sockinfos;
	t_targets	targets;

	if (ac != 5)
		return (print_error("Error: Wrong number of arguments"));
	if (!parse_user_inputs(av, &targets))
		return (print_error("Error: Wrong IP(s) or MAC(s)"));
	if (!open_raw_socket(&sockinfos))
		return (1);
	if (!configure_signal())
		return (close_and_return(&sockinfos, 1));
	if (find_inteface(&sockinfos, 0) != SUCCESS)
		return (interface_error(&sockinfos));
	printf("Waiting for ARP request from the target\n");
	if (!wait_for_target_request(&sockinfos, &targets))
		return (close_and_return(&sockinfos, !*stop_flag()));
	if (*stop_flag())
		return (close_and_return(&sockinfos, 0));
	if (!send_arp_reply(&sockinfos, &targets))
		return (close_and_return(&sockinfos, 1));
	close(sockinfos.sock);
	return (0);
}

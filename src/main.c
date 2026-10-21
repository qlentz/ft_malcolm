#include "ft_malcolm.h"

static volatile sig_atomic_t	g_stop = 0;

static void	handle_sigint(int signum)
{
	(void)signum;
	g_stop = 1;
}

static int	open_raw_socket(t_sockinfos *sockinfos)
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

static int	configure_signal(void)
{
	struct sigaction	sa;

	/* On Linux, zeroing sa also initializes sa_mask to an empty set. */
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

	while (!g_stop)
	{
		result = receive_target_packet(sockinfos, targets);
		if (result < 0)
			return (ERROR);
		if (result == SUCCESS)
			return (SUCCESS);
	}
	return (ERROR);
}

static int	print_error(const char *message)
{
	dprintf(2, "%s\n", message);
	return (1);
}

static int	close_and_return(t_sockinfos *sockinfos, int status)
{
	close(sockinfos->sock);
	return (status);
}

static int	interface_error(t_sockinfos *sockinfos)
{
	print_error("Error: No network Interface Found");
	return (close_and_return(sockinfos, 1));
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
		return (close_and_return(&sockinfos, g_stop ? 0 : 1));
	if (g_stop)
		return (close_and_return(&sockinfos, 0));
	if (!send_arp_reply(&sockinfos, &targets))
		return (close_and_return(&sockinfos, 1));
	close(sockinfos.sock);
	return (0);
}

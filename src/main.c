#include "ft_malcolm.h"

static volatile sig_atomic_t	g_stop = 0;

static void	handle_sigint(int signum)
{
	(void)signum;
	g_stop = 1;
}

static int	open_raw_socket(t_sockinfos *sockinfos)
{
	sockinfos->sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockinfos->sock == -1)
	{
		perror("socket");
		return (ERROR);
	}
	return (SUCCESS);
}

static int	configure_signal(void)
{
	struct sigaction	sa;

	ft_memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_sigint;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("sigaction");
		return (ERROR);
	}
	return (SUCCESS);
}

static int	wait_for_target_request(t_sockinfos *sockinfos, t_targets *targets)
{
	unsigned char	netbuff[NETBUFFSIZE];
	ssize_t			bytes;

	while (!g_stop)
	{
		bytes = recvfrom(sockinfos->sock, netbuff, NETBUFFSIZE, 0, NULL, NULL);
		if (bytes < 0)
		{
			if (errno == EINTR)
				continue ;
			perror("recvfrom");
			return (ERROR);
		}
		if (process_incoming_packet(netbuff, bytes, targets))
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
	if (!inet_pton(AF_INET, av[1], &targets->source_ip) || !inet_pton(AF_INET,
			av[3], &targets->target_ip))
		return (ERROR);
	if (!is_valid_mac_address(av[2]) || !is_valid_mac_address(av[4]))
		return (ERROR);
	mac_string_to_bytes(av[2], targets->source_mac);
	mac_string_to_bytes(av[4], targets->target_mac);
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
	if (!find_inteface(&sockinfos))
		return (interface_error(&sockinfos));
	printf("Waiting for ARP request from the target\n");
	if (!wait_for_target_request(&sockinfos, &targets))
		return (close_and_return(&sockinfos, g_stop ? 0 : 1));
	if (!send_arp_reply(&sockinfos, &targets))
		return (close_and_return(&sockinfos, 1));
	close(sockinfos.sock);
	return (0);
}

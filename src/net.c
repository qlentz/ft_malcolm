#include <ft_malcolm.h>

int	process_incoming_packet(unsigned char *netbuff, ssize_t bytes,
		t_targets *targets)
{
	arp_packet	*arp;

	if ((size_t)bytes < sizeof(arp_packet))
		return (ERROR);
	arp = (arp_packet *)netbuff;
	if (!is_arp_request(arp))
		return (ERROR);
//	print_arp_packet(arp);
	if (is_arp_target(arp, targets))
		return (SUCCESS);
	return (ERROR);
}

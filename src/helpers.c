#include <ft_malcolm.h>

void	print_arp_packet(const arp_packet *packet)
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

#include <ft_malcolm.h>

void	print_arp_packet(const arp_packet *packet)
{
	char	sender_ip[INET_ADDRSTRLEN];
	char	target_ip[INET_ADDRSTRLEN];

	if (!inet_ntop(AF_INET, &packet->arp_hdr.sender_ip, sender_ip,
			sizeof(sender_ip)))
	{
		strncpy(sender_ip, "invalid", sizeof(sender_ip));
		sender_ip[sizeof(sender_ip) - 1] = '\0';
	}
	if (!inet_ntop(AF_INET, &packet->arp_hdr.target_ip, target_ip,
			sizeof(target_ip)))
	{
		strncpy(target_ip, "invalid", sizeof(target_ip));
		target_ip[sizeof(target_ip) - 1] = '\0';
	}
}

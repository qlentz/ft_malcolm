#include "ft_malcolm.h"

int	is_arp_request(arp_packet *pack) {
	if (pack->arp_hdr.opcode == 1)
		return (SUCCESS);
	return (ERROR);
}

int	is_target_ip(arp_packet *pack, t_targets *targets) {
	if (!mac_equal(pack->arp_hdr.sender_mac, targets->target_mac)
			|| pack->arp_hdr.sender_ip != targets->target_ip)
		return (ERROR);
	return (SUCCESS);
}

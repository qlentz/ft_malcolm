/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mac_addr.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qlentz <marvin@42lausanne.ch>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/21 17:02:08 by qlentz            #+#    #+#             */
/*   Updated: 2026/10/21 17:02:08 by qlentz           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malcolm.h"

static int	hex_value(unsigned char c)
{
	if (ft_isdigit(c))
		return (c - '0');
	c = ft_tolower(c);
	if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	return (-1);
}

int	mac_string_to_bytes(const char *mac_str, uint8_t *mac_bytes)
{
	int	i;
	int	high;
	int	low;

	if (!mac_str || !mac_bytes || ft_strlen(mac_str) != 17)
		return (ERROR);
	i = 0;
	while (i < 6)
	{
		high = hex_value(mac_str[i * 3]);
		low = hex_value(mac_str[i * 3 + 1]);
		if (high < 0 || low < 0 || (i < 5 && mac_str[i * 3 + 2] != ':'))
			return (ERROR);
		mac_bytes[i] = high * 16 + low;
		i++;
	}
	return (SUCCESS);
}

void	mac_bytes_to_string(const uint8_t *mac_bytes, char *mac_str)
{
	sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac_bytes[0],
		mac_bytes[1], mac_bytes[2], mac_bytes[3], mac_bytes[4], mac_bytes[5]);
}

int	is_valid_mac_address(const char *mac_str)
{
	uint8_t	mac_bytes[6];

	return (mac_string_to_bytes(mac_str, mac_bytes));
}

int	mac_equal(uint8_t *m1, uint8_t *m2)
{
	int	i;

	i = 0;
	while (i < 6)
	{
		if (m1[i] != m2[i])
			return (ERROR);
		i++;
	}
	return (SUCCESS);
}

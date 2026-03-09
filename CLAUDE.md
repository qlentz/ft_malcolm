# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ft_malcolm is an ARP spoofing tool written in C. It listens for ARP requests on the network and responds with forged ARP replies to poison the ARP cache of a target machine.

Usage: `./ft_malcolm <source_ip> <source_mac> <target_ip> <target_mac>`

Requires root or `CAP_NET_ADMIN` + `CAP_NET_RAW` capabilities to open raw AF_PACKET sockets.

## Build Commands

```bash
make          # Build ft_malcolm (also builds libft)
make re       # Full rebuild
make clean    # Remove object files
make fclean   # Remove object files and binaries
```

## Test Environment

Testing requires Docker to simulate a two-host network:

```bash
make env_up   # Start attacker (192.168.0.2) and victim (192.168.0.3) containers
make env_down # Stop and remove containers
```

The attacker container auto-builds ft_malcolm on startup. There are no unit tests — validation is done manually by running the tool in the Docker network and observing ARP table changes on the victim container.

## Architecture

### Attack Flow

1. Parse 4 CLI args: source IP/MAC (what to spoof) and target IP/MAC (who to attack)
2. Open `AF_PACKET SOCK_RAW` socket with `ETH_P_ALL`
3. Discover a network interface that is UP, RUNNING, non-loopback, and has an `AF_PACKET` address
4. Bind socket to that interface
5. Loop: receive packets, filter for ARP requests from the target IP/MAC
6. On match, construct and send a spoofed ARP reply (opcode=2) claiming source IP maps to source MAC
7. Exit on SIGINT

### Key Structures (includes/)

- `ft_malcolm.h`: `s_sockinfos` (raw socket + interface index) and `s_targets` (parsed source/target IPs and MACs)
- `arp.h`: `eth_header` and `arp_header` with `__attribute__((packed))` — these map directly onto wire bytes

### Source Modules (src/)

| File | Purpose |
|------|---------|
| `main.c` | Entry point, SIGINT handler, socket setup, main recv loop |
| `arp.c` | `is_arp_request()`, `is_arp_target()`, `create_arp_reply()`, `send_arp_reply()` |
| `mac_addr.c` | MAC string↔bytes conversion and validation |
| `interface.c` | Interface enumeration via `getifaddrs()`, socket binding |
| `net.c` | `process_incoming_packet()` — size check, ARP filter, target match |
| `helpers.c` | Debug print utilities |

### libft

Custom C standard library in `libft/`. Provides string, memory, I/O, linked list utilities, `ft_printf`, and `get_next_line`. Compiled into `libft/libft.a` and linked into the main binary.

## Code Style Notes

This project follows 42 School norm conventions. Recent commits are labeled "norm" or "norm fixes" — these are style compliance commits. The function name `find_inteface` (missing an 'r') is intentional/consistent throughout the codebase — do not rename it.

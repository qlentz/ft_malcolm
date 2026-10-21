# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ft_malcolm is an ARP spoofing tool written in C. It listens for ARP requests on the network and responds with forged ARP replies to poison the ARP cache of a target machine.

Usage: `./ft_malcolm <source_ip> <source_mac> <target_ip> <target_mac>`

Opening raw AF_PACKET sockets requires `CAP_NET_RAW`, usually supplied by running
as root. Setting up veth links for tests also needs network administration rights.

## Build Commands

```bash
git submodule update --init --recursive # Initialize the pinned libft dependency
make          # Build ft_malcolm (also builds libft)
make re       # Full rebuild
make clean    # Remove object files
make fclean   # Remove object files and binaries
norminette src includes # Check all project C sources and headers
make check    # Mandatory parsing, runtime, and build regressions
make check_kernel # Live exchange in isolated attacker/target network namespaces
```

## Test Environment

The Docker Compose demo simulates a two-host network:

```bash
make env_up   # Start attacker (192.168.0.2) and victim (192.168.0.3) containers
make env_down # Stop and remove containers
python3 tests/check_docker.py --sudo # Verify the running demo on this host
```

The Compose lab builds local sources using `test_env/Dockerfile`; the attacker
entrypoint exits if compilation fails. `make env_up` rebuilds the images.
This account needs `sudo make env_up` and `sudo make env_down` for Docker access.
The tested manual evaluation walkthrough is in `test_env/DEMO.md`.
Run `make check` for mandatory regressions and
`python3 tests/check_interfaces.py --valgrind` for memory checks on mocked flows.
`make check_kernel` checks real sockets between independent attacker/target
network namespaces, including invalid broadcasts, reply routing, the target ARP
cache, idle receive timeouts, and SIGINT. It needs permission to create those
namespaces, veth links, and packet sockets. See `test_env/VERIFICATION.md` for the
VM/kernel information and recorded results.

## Architecture

### Attack Flow

1. Parse 4 CLI args: source IP/MAC (what to spoof) and target IP/MAC (who to attack)
2. Open an unbound `AF_PACKET SOCK_RAW` socket with `ETH_P_ARP`
3. Check that at least one non-loopback Ethernet interface is both UP and RUNNING
4. Receive frames and their `sockaddr_ll` with a 1-second timeout; accept incoming Ethernet broadcasts carrying ARP
5. Validate Ethernet/IPv4 ARP types, lengths, opcode, and sender-MAC consistency; match the target IP/MAC and source IP, then select the receiving interface after checking it is still usable
6. Send one spoofed ARP reply (opcode=2) through that receiving interface, claiming source IP maps to source MAC
7. Exit on SIGINT

### Key Structures (includes/)

- `ft_malcolm.h`: `s_sockinfos` (raw socket + interface index) and `s_targets` (parsed source/target IPs and MACs)
- `arp.h`: `t_eth_header`, `t_arp_header`, and `t_arp_packet` use `__attribute__((packed))` to map directly onto wire bytes

### Source Modules (src/)

| File | Purpose |
|------|---------|
| `main.c` | Entry point, SIGINT handler, and main recv loop |
| `setup.c` | Socket setup, input parsing, error output, and socket cleanup |
| `arp.c` | Broadcast/header validation and target matching |
| `reply.c` | Construct and send the reply; report failed and short sends separately |
| `mac_addr.c` | MAC string↔bytes conversion and validation |
| `interface.c` | Interface availability and selection by receiving index via `getifaddrs()` |
| `net.c` | Receive metadata, packet filtering, and selection of the reply interface |
| `helpers.c` | Debug print utilities |

### libft

Custom C standard library maintained at https://github.com/qlentz/libft.git.
`libft/` is a Git submodule pinned to a published library commit. It provides
string, memory, I/O, linked list utilities, `ft_printf`, and `get_next_line`.
Compiled into `libft/libft.a` and linked into the main binary. Library fixes
belong in its own repository; update this project's submodule pointer afterward.
Run `norminette libft` and `python3 libft/tests/check.py --valgrind` for its checks.

## Code Style Notes

This project follows 42 School norm conventions. Recent commits are labeled "norm" or "norm fixes" — these are style compliance commits. The function name `find_inteface` (missing an 'r') is intentional/consistent throughout the codebase — do not rename it.

All eight project C sources and both headers pass `norminette src includes`.
The command exits 0 without notices. A private accessor in `main.c` owns the
single function-local static `volatile sig_atomic_t` flag used by the signal
handler and receive loop; there are no file-scope global variables.

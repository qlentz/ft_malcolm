# Mandatory verification

Recorded on 2026-10-21. The original findings remain in [review.md](../review.md);
the current checklist is [TODO.md](../TODO.md).

## Environment

The actual development/test environment reports:

```text
systemd-detect-virt --vm: microsoft
uname -r: 6.18.33.2-microsoft-standard-WSL2
```

The VM is detected as Microsoft virtualization, and the kernel is greater than
3.14. The live test creates independent attacker and target network namespaces
inside that VM. Their veth links and addresses have no connection to the host
network. The subject requires work in a VM; separate VMs for the two endpoints
are not an additional stated requirement.

## Reproducible checks

Initialize the pinned library and run:

```sh
git submodule update --init --recursive
make check
python3 tests/check_interfaces.py --valgrind
make check_kernel
```

The kernel check requires permission to create user/network namespaces, veth
links, and raw sockets. Other checks need GCC, Make, Python 3, and binutils;
Valgrind is needed only for its explicit memory-check command.

| Check | Result |
| --- | --- |
| Packet parsing and filtering | 398 cases passed, including truncated/negative lengths, invalid types/lengths/opcodes, unicast/multicast destinations, sender mismatches, and valid padded requests. |
| Reply construction | All 42 bytes match an independently built expected reply, with buffer guards intact. |
| MAC parser | 7,437 cases passed, including every octet value, character mutations, overflow, signs, whitespace, and prefixes. |
| CLI rejection | 39 wrong-argument, invalid IPv4, and invalid MAC cases fail before opening a socket. |
| Runtime/interface behavior | 58 mocked scenarios passed, including missing/down adapters, receive metadata, I/O failures, short sends, idle timeouts, and SIGINT before receive or after matching. |
| Memory checks | The 58 mocked scenarios pass Valgrind without memory errors or leaks. |
| Builds | Clean/parallel/direct-target builds, parallel `re`, header/source/archive updates, missing library recovery, unchanged builds, and cleanup passed. Cleanup needs no initialized submodule; Make initializes a missing library before compilation. |
| Function/global contract | Production imports use the allowed functions plus compiler/runtime symbols; project C files define only the `g_stop` global variable. |
| Compiler analysis | GCC analyzer passed all project C sources. |
| Style of rewritten modules | `arp.c`, `reply.c`, `net.c`, and `interface.c` pass Norminette. Whole-project style cleanup remains optional under subject v2.2. |

## Live exchange

`make check_kernel` uses two independent network stacks. The attacker also has
earlier unrelated adapters, so choosing the first available adapter would fail.

The target sends 13 invalid/unicast frames; none causes a reply. It then creates
a real pending neighbor lookup and broadcasts an ARP request using its kernel.
The program selects the receiving `right0` adapter and sends one exact reply.
The target's independent neighbor table learns:

```text
192.0.2.1 lladdr 02:00:00:00:00:01
```

The program exits successfully. A separate run remains alive through an idle
receive timeout, then exits on SIGINT without sending a reply. The receive
timeout also bounds the wait if SIGINT arrives immediately before `recvfrom`.

## Container fixture

The Dockerfile builds the initialized local checkout rather than cloning a
possibly different GitHub revision. Image installation/compilation failures
stop the build; the entrypoint stops before its command if `make` fails.

Docker's CLI is unavailable here. The same attacker Dockerfile target was built
successfully using Podman with disposable storage under `/tmp`, confirming the
project compiles against Alpine/musl. Real container smoke tests passed for
startup, network-tool availability, invalid CLI arguments, and an injected make
failure that prevented the requested command from starting. Compose YAML and
local build contexts were also checked. Running the complete Compose lab through
Docker remains an optional host-specific smoke test.

The pinned libft revision is `834a5babf035e914d32f30eae92baff14238db82`.
Its prior regression suite, memory checks, and Norminette results are recorded in
the TODO and its own repository. ASan/UBSan runtimes were unavailable for the
earlier library check; no sanitizer success is claimed.

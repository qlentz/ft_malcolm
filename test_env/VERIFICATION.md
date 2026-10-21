# Project verification

Recorded on 2026-10-21. The original findings remain in [review.md](../review.md);
the current checklist is [TODO.md](../TODO.md).

The mandatory implementation and Docker evaluation demo pass the checks below.
The Norm and demo follow-ups and their reproducible checks are included in this
repository.

## Environment

The actual development/test environment reports:

```text
systemd-detect-virt --vm: microsoft
uname -r: 6.18.33.2-microsoft-standard-WSL2
Docker Engine: 29.9.2
Docker Compose: v5.6.1
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
norminette src includes libft
make check
python3 tests/check_interfaces.py --valgrind
make check_kernel
python3 libft/tests/check.py --valgrind --buffer-size 1
python3 libft/tests/check.py --valgrind --buffer-size 100
python3 libft/tests/check.py --valgrind --buffer-size 4096
sudo make env_up
python3 tests/check_docker.py --sudo
sudo make env_down
```

The kernel check requires permission to create user/network namespaces, veth
links, and raw sockets. Other checks need GCC, Make, Python 3, and binutils;
Valgrind is needed only for its explicit memory-check command.
This account needs `sudo` for Docker access. See [DEMO.md](DEMO.md) for the
manual evaluation commands and expected output.

| Check | Result |
| --- | --- |
| Packet parsing and filtering | 398 cases passed, including truncated/negative lengths, invalid types/lengths/opcodes, unicast/multicast destinations, sender mismatches, and valid padded requests. |
| Reply construction | All 42 bytes match an independently built expected reply, with buffer guards intact. |
| MAC parser | 7,437 cases passed, including every octet value, character mutations, overflow, signs, whitespace, and prefixes. |
| CLI rejection | 39 wrong-argument, invalid IPv4, and invalid MAC cases fail before opening a socket. |
| Runtime/interface behavior | 58 mocked scenarios passed, including missing/down adapters, receive metadata, I/O failures, short sends, idle timeouts, and SIGINT before receive or after matching. |
| Memory checks | All 58 mocked scenarios pass Valgrind. The library passes 24,966 checks at each of buffer sizes 1, 100, and 4096 under Valgrind, without errors or leaks. |
| Project sanitizers | All 58 runtime scenarios pass ASan/UBSan in a disposable Alpine container, with project and linked library objects instrumented and leak detection enabled. |
| Builds | Clean/parallel/direct-target builds, parallel `re`, header/source/archive updates, missing library recovery, unchanged builds, and cleanup passed. Cleanup needs no initialized submodule; Make initializes a missing library before compilation. |
| Function/state contract | Production imports use the allowed functions plus compiler/runtime symbols; project C files define one persistent signal flag and no file-scope global variables. |
| Compiler analysis | GCC analyzer passed all project C sources. |
| Project style | All eight C sources and both headers pass `norminette src includes` with exit status 0 and no notices. |
| Dependency style | All 58 library C/header files pass Norminette. |
| Checkout completeness | A fresh non-recursive clone with the final-source changes and new files overlaid initializes the pinned submodule, builds with parallel `re`, and passes Norminette and `make check`. |

The Norm follow-up moves socket setup, input parsing, and shared error/cleanup
helpers into `src/setup.c` to meet the per-file function limit. Packet types use
the `t_`/`s_` naming conventions while retaining their packed wire layout. The
Makefile and test harness include the new source; the parallel rebuild and full
`make check` regression suite pass after the changes.

The final test pass caught that Norminette 3.3.59 returns 1 for the global-variable
notice even when every file says OK. The signal flag now lives as a local static
`volatile sig_atomic_t` behind a private accessor; the handler only assigns that
flag. Plain Norminette now exits 0 without suppressing any checks.

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

The current Docker Compose lab builds and starts successfully. Verification
checks the image's C/header files and Makefiles against this checkout, both
container IP/MAC addresses, and the required network tools. Real container tests
pass for invalid arguments, denied raw-socket access after dropping capabilities,
and an injected make failure that prevents the requested command from starting.

The victim kernel broadcasts a request for the reserved lab address
`192.168.0.42`. Packet capture confirms exactly one reply, with all 42
Ethernet/ARP bytes matching the independently constructed expected frame. The
victim's neighbor table learns `192.168.0.42 -> 02:42:aa:aa:00:42`; the program
exits with status 0. The demo IP/MAC is unassigned, so no ICMP echo response is
expected from the ping used to trigger ARP.

The container SIGINT check stays alive through an idle timeout, exits with
status 0, and produces no reply. An additional interactive `docker exec -it`
test confirms that typing Ctrl+C exits with status 0. Both documented Make
targets pass; `env_down` removes the demo containers and their network. The lab
was stopped after verification, with its images cached for the next run.

`tests/check_docker.py` reproduces the automated checks against the running lab.
It clears only its test neighbor entry and stops only the processes it creates.

The pinned libft revision is `834a5babf035e914d32f30eae92baff14238db82`.
Its regression suite and memory/style checks pass as recorded above.

## Additional diagnostic limits

The host lacks the required ASan/UBSan runtime files. Project sanitizer checks
therefore ran successfully in a disposable Alpine container instead.

The standalone library suite compares `ft_printf` with the platform's printf.
On Alpine/musl, its null-pointer comparison fails because libc prints `0` and
libft prints `(nil)`, matching the GNU libc convention. This is a library
portability difference; the production program does not call `ft_printf` or
`ft_printptr`. No library source or expected result was changed to hide it.

A separate Debian/glibc library sanitizer attempt emitted repeated
`AddressSanitizer:DEADLYSIGNAL` messages and timed out without an actionable
report. That additional diagnostic remains inconclusive; no standalone library
ASan success is claimed. The complete library Valgrind checks and the project
ASan/UBSan checks both pass.

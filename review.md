# ft_malcolm review

Reviewed on 2026-10-21 against subject version 2.2, transcribed in [subject.md](subject.md).

**Assessment: the main algorithm is implemented, but the checkout is not ready for mandatory evaluation.** The submitted build fails, several mandatory calls are outside the function whitelist, and incoming-packet validation is incomplete. After bypassing the build problems in a temporary copy, the normal control flow behaves as intended in a mocked-network test.

All six project C files, both project headers, all 57 libft C files and its header, both Makefiles, and the Docker test configuration were read. No implementation files were changed during this review. Diagnostic builds and test harnesses were kept under `/tmp`.

## Findings to address first

### 1. The packet-socket header prevents compilation

Location: `includes/ft_malcolm.h:8`.

The header includes `<netinet/if_packet.h>`. That header is unavailable in the current Linux development environment. An ordinary `make` stops here:

```text
fatal error: netinet/if_packet.h: No such file or directory
```

`<netpacket/packet.h>` is available here and supplies the needed packet-socket structures. Substituting it only in the temporary review copy allowed the compiler to reach the next build defect. The Linux packet-socket documentation also documents `<linux/if_packet.h>` as a supported header: [packet(7)](https://man7.org/linux/man-pages/man7/packet.7.html).

### 2. The Makefile omits a required source and has broken library dependencies

Locations: `Makefile:3`, `Makefile:18`, `Makefile:20`, and `Makefile:41`.

- `src/main.c` calls `process_incoming_packet()`, which is defined in `src/net.c`. The source list omits `src/net.c`, so linking fails with `undefined reference to process_incoming_packet` once the header issue is bypassed.
- `make re` invokes `fclean` and then the executable target. `fclean` deletes `libft/libft.a`, but the executable target does not build that archive. The diagnostic run failed with `cannot find -lft`.
- The executable does not depend on the library archive. A change to libft can leave the executable using its old contents, and a parallel build can attempt to link before libft is ready.
- Header dependencies are absent, so editing a header alone does not rebuild affected object files.

The first two failures were reproduced. The incremental and parallel-build issues follow from the missing dependency relationships.

### 3. Mandatory code uses functions outside the subject's whitelist

Examples in the active source:

| Function | Location |
| --- | --- |
| `sscanf` | `src/mac_addr.c:5`, `src/mac_addr.c:23` |
| `memcpy` | `src/arp.c:25` and other reply-field copies |
| `memset` | `src/interface.c:24` |
| `strncpy` | `src/interface.c:25` |
| `perror` | `src/main.c:16`, `src/arp.c:56`, and interface error paths |
| `sigemptyset` | `src/main.c:28` |

The subject permits printf and its family; `sscanf` belongs to the scanning family. It permits your libft routines, but these are direct calls to the standard memory/string functions. These calls need to be reconciled with the mandatory whitelist before submission.

Compiler-generated substitutions such as a `printf` becoming `puts` were not counted as source-level violations.

### 4. Interface handling is unreliable

Locations: `src/interface.c:15`, `src/interface.c:26`, and `src/interface.c:40`.

- The readiness expression checks whether **either** `IFF_UP` or `IFF_RUNNING` is set. It does not require both. Mock interfaces with only one of the flags were accepted.
- The socket is `AF_PACKET`, but interface selection uses `SO_BINDTODEVICE`. Linux explicitly documents that option as unsupported for packet sockets: [socket(7)](https://man7.org/linux/man-pages/man7/socket.7.html). Successful return from that option should not be treated as proof that packet reception is restricted to the selected interface.
- The first candidate is used without relating it to the target network. `recvfrom()` discards the incoming `sockaddr_ll`, so the program cannot use the actual receiving interface to select the reply interface. On a machine with several adapters, it can choose an unsuitable adapter or reply through a different adapter from the one that received the request.

The flag bug was reproduced. Actual kernel filtering and delivery were not tested. Any interface-handling change must also respect the subject's function whitelist; `bind` is not listed there.

### 5. The filter accepts requests that do not meet the required trigger

Locations: `src/arp.c:3`, `src/arp.c:11`, and `src/net.c:3`.

The code correctly checks minimum frame size, Ethernet ARP type, request opcode, the target's ARP sender IP/MAC, and the requested source IP.

It does not verify:

- An Ethernet broadcast destination. A matching unicast request also triggers a reply, although the mandatory part specifies a broadcast request.
- Ethernet hardware type, IPv4 protocol type, or hardware/protocol address lengths. Frames with incompatible values are still interpreted as the fixed Ethernet/IPv4 structure.
- Consistency between the Ethernet source MAC and the ARP sender MAC.

Synthetic packets confirmed each acceptance. The missing broadcast check directly departs from the mandatory trigger; validating the remaining header fields makes that trigger reliable for Ethernet/IPv4 traffic. The field meanings and Ethernet/IPv4 request layout are specified in [RFC 826](https://www.rfc-editor.org/rfc/rfc826.html).

### 6. MAC validation accepts malformed and overflowing input

Location: `src/mac_addr.c:23`.

The validator scans unrestricted hexadecimal integers, then checks their converted values. On the current C library, this invalid address was accepted:

```text
100000002:00:00:00:00:01
```

Its first component exceeds both a MAC octet and a 32-bit integer. Numeric conversion overflow is not a reliable validation strategy.

The parser also accepts `0x` prefixes, signs, internal whitespace, and single-digit components. For example:

```text
0x02:00:00:00:00:01
+02:00:00:00:00:01
02: 00:00:00:00:01
2:0:0:0:0:1
```

The PDF's ordinary invalid examples are rejected, and trailing garbage is rejected. A bounded character-by-character parser would provide predictable validation and avoid the forbidden `sscanf` dependency. The subject does not explicitly define whether every noncanonical textual form must be rejected; the overflowing component is the unambiguous defect.

## Requirement checklist

| Subject requirement | Current status |
| --- | --- |
| C implementation | Present. |
| Makefile with usual rules | Rules exist, but `make` and `make re` fail for the reasons above. |
| Executable named `ft_malcolm` | Correct name configured; current build cannot produce it. |
| At most one global variable | Met in project code: `g_stop`, a `volatile sig_atomic_t` used by the SIGINT handler. |
| Allowed mandatory functions only | Not met; see finding 3. |
| Four arguments in source IP, source MAC, target IP, target MAC order | Implemented correctly. Wrong argument counts are rejected. |
| IPv4 support | Implemented through `inet_pton(AF_INET, ...)`. Invalid IPv4 inputs are rejected. |
| Wait for the target to broadcast an ARP request for the source IP | Partially met: sender and requested IP matching work, but broadcast and header validation are incomplete. |
| Send a single ARP reply and exit | Passed the mocked successful-flow test: one receive, one send, socket close, exit 0. |
| Reply contains supplied source IP/MAC and is addressed to the target | All 42 Ethernet/ARP bytes matched an independently constructed expected packet. |
| Target ARP table contains the supplied association | Not verified on a real target. Correct constructed bytes alone do not prove delivery or cache update. |
| Exit on Ctrl+C | Passed a real SIGINT delivered while the mocked receive was blocked; exited 0, closed the socket, and sent nothing. |
| Handle errors without unexpected termination | Tested active error paths exit cleanly, but input validation still has the defect above. Unused library defects are listed below. |
| Work in a VM, Linux kernel > 3.14 | Current kernel is `6.18.33.2-microsoft-standard-WSL2`, which meets the version threshold. The required development/defense VM setup still needs to be established; a Docker configuration alone does not establish that requirement. |
| Spoof only IPs belonging to you | Operational requirement to follow in the eventual VM test. No packets were sent to a real network during this review. |
| Submit all evaluated work in the Git repository | Source and configuration are present in the checkout. `subject.md` was untracked at review time; this review document is newly created. Final submission/commit state remains to be checked. |

The PDF mentions Debian 7.0 stable as its grading reference. No additional compiler flags, exact output wording, or Norm requirement were inferred from that reference.

## Bonuses and unfinished pieces

- No hostname resolution or alternate decimal IPv4 input path is implemented; parsing uses `inet_pton` only.
- No selectable verbose mode is implemented.
- `src/helpers.c` contains `print_arp_packet()`, but it only converts IP addresses into local strings and prints nothing. It is not part of the build, and its call sites are commented out.
- MAC formatting/equality helpers exist but are not used by the main flow.
- The mandatory part must pass completely before any bonuses are evaluated.

## Test environment and style

`test_env/attacker_entrypoint.sh:16` clones the GitHub repository into the container. The Compose configuration mounts the entrypoint script, not this checkout's source. As written, the container can build a different revision from the files being reviewed or edited locally. The script also continues to its final `tail` after setup/build failures, so a running container is not evidence of a successful build.

Norminette reported errors in every project C/header file and in three libft files. Examples include missing 42 headers, a `for` loop, the ternary expression, too many functions in `main.c`, and header formatting/type names. **Version 2.2 of this subject does not explicitly demand Norm compliance**, so these are reported separately from its explicit grading requirements.

## Additional libft findings

The executable's reachable custom-library work currently uses `ft_memcmp`, `ft_memset`, and `ft_memcpy`. The following defects are in other routines and do not establish crashes in the current executable path:

| Routine | Issue and evidence |
| --- | --- |
| `ft_strtrim`, line 33 | Empty input underflows `end`; input consisting entirely of trim characters lets the backward scan pass the start. Valgrind confirmed reads before the allocation for both cases. |
| `ft_strlcat`, line 21 | Searches beyond the supplied destination size and then writes a terminator even when no terminator exists within that size. Valgrind confirmed an invalid read and write with a two-byte nonterminated destination and size 2. |
| `get_next_line`, line 80 | Checks only `fd < 0` before indexing `rest[4096]`. Any `fd >= 4096` is out of bounds. A call with `1048575` produced a Valgrind invalid read and SIGSEGV. The read loop also uses `buf[r]` after subsequent `read()` calls without checking for `r == -1`. |
| `ft_substr`, line 23 | In the `start >= slen` branch, writes `sub[0]` before checking whether allocation succeeded. |
| `ft_strjoin_gnl`, line 31 | Writes to a newly allocated string before checking allocation success. |
| `ft_calloc`, line 19 | Does not check multiplication overflow before allocating and clearing memory. |
| `ft_split`, line 77 | Does not unwind previously allocated words when a word allocation fails. |

These are relevant if those routines are reused while finishing the project. The current successful mocked main flow passed Valgrind without reported memory errors or leaks.

## Verification scope

Build failures were reproduced in a temporary copy. To exercise behavior, only that copy's packet header include was corrected and a diagnostic compile command explicitly included `src/net.c` and built libft. This does **not** mean the submitted Makefile passed.

Behavior checks used the actual parser, packet filter, reply builder, main loop, and signal handling. Wrappers replaced raw socket and interface interactions; the mocked successful send checked every reply field and destination socket address. Failure cases covered socket creation, interface enumeration, missing interfaces, interface indexing, socket options, receive, failed send, and short send. Each tested failure returned nonzero and closed the socket when one had been opened.

One diagnostic weakness was also reproduced: a short positive `sendto` result is detected, but the error path prints `sendto failed: Success` when `errno` has no error value.

Docker is unavailable in this environment, and an unprivileged raw-socket attempt returned `Operation not permitted`. Consequently, actual interface selection, Ethernet transmission, and the target's ARP-cache update remain unverified. A controlled end-to-end test between your own VMs is still needed after the blockers are fixed.

Suggested order: repair the build/dependencies; remove forbidden calls; tighten MAC and incoming-packet validation; resolve interface handling; verify the full exchange and target cache in the required VM setup. Address bonuses after the mandatory behavior is complete.

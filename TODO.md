# ft_malcolm TODO

Updated 2026-10-21 after final verification. This checklist tracks project
readiness and the remaining optional work.

## Addressed review findings

- [x] **Point 1: packet-socket header.** Use `<linux/if_packet.h>`.
- [x] **Point 2: build and dependencies.** Compile `src/net.c`, build libft before
  linking, relink when its archive changes, track header dependencies in both
  projects, and sequence cleanup before rebuilding for parallel `make re`.
  Initialize missing submodules before compiling; cleanup also works before init.
- [x] **Point 3: forbidden calls.** Use libft memory/string routines and a MAC
  parser based on libft helpers. Replace `perror` with permitted
  `dprintf`/`strerror` calls. Initialize the signal mask through the existing
  `ft_memset`; an all-zero mask is empty on the required Linux platform.
- [x] **Point 6: MAC input.** Require six colon-separated, two-digit hexadecimal
  octets. Reject overflowing components, signs, prefixes, whitespace, missing
  digits, and trailing characters. Both letter cases work.
- [x] **Point 4: interface handling.** Require usable Ethernet adapters, remove
  `SO_BINDTODEVICE`, retain receive metadata, and reply through the interface on
  which the matching request arrived. Real Linux socket checks passed in
  isolated network namespaces within the detected Microsoft VM.
- [x] **Point 5: request validation.** Require a matching Ethernet/IPv4 ARP
  broadcast with valid header lengths and consistent sender MAC addresses.

## Mandatory behavior (addressed)

### Point 4: interface handling (addressed)

- [x] Require both `IFF_UP` and `IFF_RUNNING`, an Ethernet `AF_PACKET` address,
  and a six-byte hardware address; exclude loopback and incomplete entries.
- [x] Remove `SO_BINDTODEVICE`; use an unbound ARP-only packet socket and the
  existing permitted `recvfrom`/`sendto` calls. No `bind` call was added.
- [x] Retain and validate the received `sockaddr_ll`, ignore outgoing copies,
  and select the reply interface by its received index after the packet matches.
  Recheck interface availability at that point; startup no longer chooses one.
- [x] Verify real Linux reception and transmission with multiple veth pairs;
  a request arriving through a later adapter selects that adapter, the reply's
  bytes match, and the peer kernel learns the supplied IP/MAC association.

### Point 5: incoming packet validation (addressed)

- [x] Require Ethernet destination `ff:ff:ff:ff:ff:ff` for the triggering request.
- [x] Require Ethernet hardware type, IPv4 protocol type, and address lengths
  of six and four bytes before interpreting the fixed ARP layout.
- [x] Require the Ethernet source MAC to match the ARP sender MAC.
- [x] Recheck that valid broadcasts still trigger exactly one reply and that
  matching unicast requests and malformed headers are ignored.

### Error reporting noted in the review

- [x] Give a short positive `sendto` result its own diagnostic showing the actual
  and expected byte counts; reserve `strerror(errno)` for negative results.
- [x] Add a receive timeout and retry interrupted/idle receives. Ctrl+C remains
  effective even when it arrives just before blocking; do not send if it was
  handled after a matching request was selected.

## Environment, live verification, and submission

- [x] Verify the VM and kernel requirements in the actual development environment:
  `systemd-detect-virt --vm` reports `microsoft`; Linux is
  `6.18.33.2-microsoft-standard-WSL2`, greater than 3.14. The subject requires a VM;
  it does not specify a separate VM for each test endpoint.
- [x] Build the container lab from the local checkout and initialized libft
  submodule. Compose uses the repository as its build context, and `make env_up`
  rebuilds the images.
- [x] Stop container builds/startup when installation or compilation fails. The
  entrypoint uses `set -eu` and starts its command only after a successful make.
- [x] Run the full exchange between independent owned network stacks inside the
  VM: reject invalid/unicast frames, receive the target's kernel-generated ARP
  broadcast, send exactly one correctly addressed reply, exit successfully, and
  inspect the target namespace's ARP table for the supplied source IP/MAC.
- [x] Verify Ctrl+C on a real packet socket, no reply on interruption, and continued
  waiting after an idle receive timeout.
- [x] Verify multiple-adapter selection and interface/error cleanup.
- [x] Include the Norm and Docker-demo follow-ups in the submitted Git history,
  including `src/setup.c`, `tests/check_docker.py`, and `test_env/DEMO.md`.
  A fresh clone with the final-source changes overlaid passed automatic submodule
  initialization, a parallel rebuild, Norminette, and `make check`.
  Generated files remain ignored.

## Addressed libft defects from the review

Fixed in [qlentz/libft, commit 834a5ba](https://github.com/qlentz/libft/commit/834a5babf035e914d32f30eae92baff14238db82).
This project's `libft/` now points to that revision as a Git submodule.

- [x] `ft_strtrim`: prevent underflow and reads before the string for empty input
  and input consisting entirely of trim characters.
- [x] `ft_strlcat`: respect the destination size when looking for a terminator;
  do not write when none exists within that size.
- [x] `get_next_line`: reject descriptors outside the static array bounds and
  handle every negative `read` result before indexing the buffer.
- [x] `ft_substr`: check allocation success before writing the empty substring.
- [x] `ft_strjoin_gnl`: check allocation success before writing the new string.
- [x] `ft_calloc`: check multiplication overflow before allocating and clearing.
- [x] `ft_split`: free previously allocated words if a later allocation fails.
- [x] Fix additional bounded string reads, integer/decimal parsing, list-map
  cleanup, memory-move lengths, and formatted-output error handling.
- [x] Make all library C/header files pass Norminette and add regression tests.

## Optional work and style findings

The subject does not explicitly require Norm compliance. Bonuses are evaluated
only after all mandatory requirements pass.

- [x] Address all reported project Norminette errors: add valid headers, fix
  guards/type names/formatting, remove the ternary expression, and move socket,
  input, and cleanup helpers to `src/setup.c` to meet the function limit.
  All eight project C sources and both headers pass with exit status 0 and no
  notices. The signal flag is a function-local static `volatile sig_atomic_t`
  accessed through a private helper; the interface-error helper moved to setup
  to keep both files within the function limit.
- [ ] If implementing verbose mode, finish `print_arp_packet`, add its source to
  the build, and enable its call sites only when requested by that mode. It
  currently converts addresses without printing them.
- [ ] Decide whether to use or remove the unused MAC formatting/equality helpers.
- [ ] Optional bonus: hostname resolution for IPv4 addresses.
- [ ] Optional bonus: alternate decimal IPv4 input.
- [ ] Optional library portability: `ft_printf("%p", NULL)` prints `(nil)`;
  Alpine/musl printf prints `0`. The production program does not use this
  formatter. The library suite passes on the GNU libc host, but this comparison
  fails under musl. No libft change was made in this project follow-up.
- [ ] Optional diagnostic follow-up: standalone library ASan checks remain
  inconclusive in the Debian container. Full library Valgrind checks pass, and
  all 58 project runtime scenarios pass ASan/UBSan in Alpine.

## Verification already completed

- The point 3 follow-up passed 7,437 MAC parser cases and 12 mocked runtime
  scenarios, including reply contents, error cleanup, and Ctrl+C interrupting a
  blocked receive. The zeroed Linux signal mask was checked as empty.
- The point 2 follow-up passed checks for clean and parallel builds, direct
  executable builds, parallel `re` in both projects, incremental source/header/archive
  updates, missing library artifacts, and unchanged builds without recompilation
  or relinking. `clean`/`fclean` also remove generated dependency files.
- The final live test uses independent attacker/target network namespaces in
  the detected VM. Thirteen invalid/unicast frames produce no reply; a valid
  broadcast produces one exact reply and updates the independent target cache.
- The libft follow-up passed 24,966 checks under Valgrind for each of buffer sizes
  1, 100, and 4096, with no memory errors or leaks. All 58 library C/header files
  pass Norminette; GCC's analyzer passed all 57 C sources. Sanitizer modes are
  provided by the test runner but could not link here because their runtimes are
  missing.
- The point 4 follow-up passed 41 mocked interface/runtime scenarios under
  Valgrind without errors or leaks, including
  interface flags, wrong-adapter avoidance, disappearing/down adapters, receive
  metadata validation, cleanup, and SIGINT. The real namespace test also checks
  Ctrl+C interrupting a blocked packet socket.
  `make -j8 re`, unchanged builds, and GCC analysis passed; the revised
  `interface.c` and `net.c` also pass Norminette.

## Final mandatory follow-up

- [x] `make check`: 398 packet cases, every reply byte, 7,437 MAC cases,
  39 CLI error cases, 58 mocked runtime scenarios, container entrypoint failures,
  and isolated clean/parallel/incremental build checks.
- [x] Valgrind: all 58 mocked scenarios pass without memory errors or leaks.
- [x] Live kernel test: invalid frames, one correct reply, independent target
  ARP cache, idle timeout, and Ctrl+C all pass.
- [x] GCC analyzer passes all eight current project C sources. All 68 project
  and library C/header files pass Norminette; parallel rebuilds and the full
  `make check` suite pass after the Norm follow-up.
- [x] All 58 project runtime scenarios pass ASan/UBSan in a disposable Alpine
  container with the linked library objects instrumented.
- [x] Docker Compose builds, starts, and stops the demo successfully. Real tests
  verify local-source hashes, network tools, CLI/permission/startup errors, an
  exact single reply, the victim's ARP cache, idle waiting, and SIGINT. Actual
  interactive Ctrl+C also exits 0. See `test_env/DEMO.md` for evaluation commands.

Full-project Norm work is complete; bonuses above remain optional under subject v2.2.
See [test_env/VERIFICATION.md](test_env/VERIFICATION.md) for commands and evidence.

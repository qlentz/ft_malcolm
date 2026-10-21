# ft_malcolm

An IPv4 ARP exercise written in C. See [TODO.md](TODO.md) for the current project
status and [test_env/DEMO.md](test_env/DEMO.md) for the evaluation walkthrough.

## Build

```sh
git clone --recurse-submodules https://github.com/qlentz/ft_malcolm.git
cd ft_malcolm
make
```

For an existing checkout, initialize the dependency before building:

```sh
git submodule update --init --recursive
make
```

`make` also initializes a missing libft checkout automatically. Cleanup works
before that dependency has been initialized.

`libft/` is a Git submodule of [qlentz/libft](https://github.com/qlentz/libft),
pinned to a specific commit. Its source is maintained in that repository.
Publish library fixes there, then update and commit this project's submodule
pointer to use the corrected revision.

Run `norminette libft` and `python3 libft/tests/check.py --valgrind` to check the
library. `make re` rebuilds both projects; `make fclean` removes build artifacts.

## Checks

```sh
norminette src includes
make check
python3 tests/check_interfaces.py --valgrind
make check_kernel
```

`make check` covers packet validation, MAC/IPv4 input, reply contents, error paths,
interface selection, container startup failures, and build dependencies.

The kernel check creates independent attacker and target network namespaces
connected only by temporary veth links. It checks rejection of invalid requests,
one reply through the receiving adapter, the target ARP table, and Ctrl+C. It
requires namespace, veth, and raw-socket permissions. Run development and tests
inside a VM with Linux greater than 3.14, as required by the subject.

The program accepts only matching Ethernet/IPv4 ARP broadcasts. It selects the
reply adapter from the receive metadata and requires both UP and RUNNING flags.
An idle receive timeout bounds the Ctrl+C check to one second even if the signal
arrives immediately before a receive begins. No interface binding is used.

See [test_env/VERIFICATION.md](test_env/VERIFICATION.md) for the recorded checks.

## Container lab

```sh
make env_up
make env_down
```

The optional Compose lab builds images from this checkout, including the pinned
libft submodule. `make env_up` rebuilds those images. Dependency installation and
compilation failures stop the image build; startup also exits if `make` fails.
The attacker image's working directory is `/opt/ft_malcolm`. Both images include
`ip`, `ping`, `arping`, and `tcpdump` for inspecting their owned test network.

See [test_env/DEMO.md](test_env/DEMO.md) for the tested evaluation walkthrough.
On this host use `sudo make env_up`, then
`python3 tests/check_docker.py --sudo` for automated Docker verification.
Use `sudo make env_down` to remove the demo containers and network afterward.

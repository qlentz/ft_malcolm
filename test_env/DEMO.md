# Evaluation demo

Run these commands from the repository root, inside your Linux VM. This host
needs `sudo` for Docker access; omit it if your account can access Docker directly.
The commands use only the project's owned Docker network and containers.

## Start the lab

```sh
git submodule update --init --recursive
sudo make env_up
```

The attacker is `192.168.0.2` with MAC `02:42:aa:aa:00:02`; the victim is
`192.168.0.3` with MAC `02:42:aa:aa:00:03`.
Reserve `192.168.0.42` inside this lab for the supplied source IP, and leave it
unassigned so another kernel does not answer its ARP requests. The association
we will demonstrate is `192.168.0.42 -> 02:42:aa:aa:00:42`.

## Automated verification

```sh
python3 tests/check_docker.py --sudo
```

This checks the image's sources against the checkout, container addresses and
tools, invalid arguments, raw-socket permission errors, startup build failures,
idle waiting, SIGINT, captured reply bytes/count, and the victim's ARP cache.
It leaves the lab running and clears only its test neighbor entry afterward.

## Show the exchange manually

In terminal 1, clear any previous demo entry and start the program:

```sh
sudo docker exec arp_victim ip neigh flush to 192.168.0.42 dev eth0
sudo docker exec -it arp_attacker ./ft_malcolm \
    192.168.0.42 02:42:aa:aa:00:42 \
    192.168.0.3 02:42:aa:aa:00:03
```

It prints `Waiting for ARP request from the target` and waits.

In terminal 2, trigger the victim kernel's ARP lookup and inspect its cache:

```sh
sudo docker exec arp_victim ping -c 1 -W 1 192.168.0.42
sudo docker exec arp_victim ip neigh show 192.168.0.42 dev eth0
```

The ping times out because the demo address/MAC does not provide an ICMP
responder. The successful result is the neighbor entry:

```text
192.168.0.42 dev eth0 lladdr 02:42:aa:aa:00:42 REACHABLE
```

The state can later become `STALE`; the learned IP/MAC association is what
matters. Terminal 1 reports the request, selects `eth0`, sends one reply, and
exits successfully. Clear the neighbor entry before repeating the exchange,
otherwise a cached association can prevent a new broadcast request.

For an optional packet view, start this in terminal 3 before triggering the ping:

```sh
sudo docker exec -it arp_victim tcpdump -n -e -i eth0 \
    'arp and host 192.168.0.42'
```

It shows the broadcast request and the reply advertising the supplied MAC.
Stop the capture with Ctrl+C.

## Show interruption and input rejection

Start the terminal 1 command again, then press Ctrl+C without triggering a
request. It exits successfully without sending a reply. Immediately afterward,
`echo $?` in that terminal reports `0`.

```sh
sudo docker exec arp_attacker ./ft_malcolm \
    192.168.0.42 100000002:00:00:00:00:01 \
    192.168.0.3 02:42:aa:aa:00:03
```

The malformed MAC is rejected with exit status `1` before opening a socket.

## Stop the lab

```sh
sudo make env_down
```

This removes the two demo containers and their Compose network; the built
images remain cached for the next demonstration.

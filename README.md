# ft_malcolm

A C program that waits for the target's IPv4 ARP broadcast, sends one reply
advertising the supplied source IP/MAC, then exits. Ctrl+C stops waiting.

## Build and run

```sh
make
sudo ./ft_malcolm <source_ip> <source_mac> <target_ip> <target_mac>
```

## Docker demo

Start the environment:

```sh
git submodule update --init --recursive
sudo make env_up
```

Terminal 1: clear the previous entry and start the program.

```sh
sudo docker exec arp_victim ip neigh flush to 192.168.0.42 dev eth0
sudo docker exec -it arp_attacker ./ft_malcolm \
    192.168.0.42 02:42:aa:aa:00:42 192.168.0.3 02:42:aa:aa:00:03
```

Terminal 2: trigger ARP and inspect the victim's table.

```sh
sudo docker exec arp_victim ping -c 1 -W 1 192.168.0.42
sudo docker exec arp_victim ip neigh show
```

The table should associate `192.168.0.42` with `02:42:aa:aa:00:42`.
The ping may time out; the ARP entry is the result being demonstrated.

Stop the environment:

```sh
sudo make env_down
```

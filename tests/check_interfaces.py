#!/usr/bin/env python3
"""Check receiving-interface selection with mocked or isolated Linux sockets."""

import argparse
import errno
import json
import os
from pathlib import Path
import pty
import selectors
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time


WRAPPERS = r"""
#include "ft_malcolm.h"
#include <assert.h>

static int channels[2];
static int received, sent, enumerations, outstanding;

static int scenario(const char *name)
{
    const char *value = getenv("INTERFACE_SCENARIO");
    return value && strcmp(value, name) == 0;
}

int __wrap_socket(int domain, int type, int protocol)
{
    assert(domain == AF_PACKET && type == SOCK_RAW);
    assert(protocol == htons(ETH_P_ARP));
    if (scenario("socket_failure")) { errno = EACCES; return -1; }
    assert(pipe(channels) == 0);
    return channels[0];
}

int __wrap_getifaddrs(struct ifaddrs **result)
{
    static struct ifaddrs info[2];
    static struct sockaddr_ll address[2];
    enumerations++;
    if (scenario("sigint_after_match") && enumerations > 1)
        assert(raise(SIGINT) == 0);
    assert(!outstanding);
    if (scenario("enumeration_failure")
        || (scenario("runtime_enumeration_failure") && enumerations > 1))
    { errno = ENOMEM; return -1; }
    memset(info, 0, sizeof(info));
    memset(address, 0, sizeof(address));
    for (int i = 0; i < 2; i++)
    {
        info[i].ifa_name = i ? "second0" : "first0";
        info[i].ifa_flags = IFF_UP | IFF_RUNNING;
        info[i].ifa_addr = (struct sockaddr *)&address[i];
        address[i].sll_family = AF_PACKET;
        address[i].sll_hatype = ARPHRD_ETHER;
        address[i].sll_halen = ETH_ALEN;
        address[i].sll_ifindex = i ? 11 : 7;
        if (scenario("up_only")) info[i].ifa_flags = IFF_UP;
        if (scenario("running_only")) info[i].ifa_flags = IFF_RUNNING;
        if (scenario("down")) info[i].ifa_flags = 0;
        if (scenario("loopback")) info[i].ifa_flags |= IFF_LOOPBACK;
        if (scenario("no_address")) info[i].ifa_addr = NULL;
        if (scenario("no_name")) info[i].ifa_name = NULL;
        if (scenario("ipv4_address")) address[i].sll_family = AF_INET;
        if (scenario("non_ethernet")) address[i].sll_hatype = ARPHRD_NONE;
        if (scenario("non_ethernet_length")) address[i].sll_halen = 8;
    }
    info[0].ifa_next = &info[1];
    if (scenario("mixed_candidates")) info[0].ifa_addr = NULL;
    if (scenario("runtime_down") && enumerations > 1)
        info[1].ifa_flags = IFF_UP;
    if (scenario("runtime_removed") && enumerations > 1)
        info[0].ifa_next = NULL;
    *result = scenario("no_interface") ? NULL : info;
    outstanding = 1;
    return 0;
}

void __wrap_freeifaddrs(struct ifaddrs *info)
{
    (void)info;
    assert(outstanding);
    outstanding = 0;
}

unsigned __wrap_if_nametoindex(const char *name)
{
    assert(name);
    if (scenario("index_failure")
        || (scenario("runtime_index_failure") && enumerations > 1
            && strcmp(name, "second0") == 0))
    { errno = ENODEV; return 0; }
    assert(strcmp(name, "first0") == 0 || strcmp(name, "second0") == 0);
    return strcmp(name, "first0") == 0 ? 7 : 11;
}

int __wrap_bind(int fd, const struct sockaddr *addr, socklen_t length)
{
    (void)fd; (void)addr; (void)length;
    assert(!"bind is outside the mandatory whitelist");
    return -1;
}

int __wrap_setsockopt(int fd, int level, int option,
                     const void *value, socklen_t length)
{
    const struct timeval *timeout = value;
    assert(fd == channels[0] && level == SOL_SOCKET && option == SO_RCVTIMEO);
    assert(length == sizeof(*timeout));
    assert(timeout->tv_sec == 1 && timeout->tv_usec == 0);
    if (scenario("timeout_setup_failure")) { errno = ENOPROTOOPT; return -1; }
    return 0;
}

int __real_sigaction(int, const struct sigaction *, struct sigaction *);
int __wrap_sigaction(int number, const struct sigaction *action,
                     struct sigaction *old)
{
    if (scenario("signal_failure")) { errno = EINVAL; return -1; }
    return __real_sigaction(number, action, old);
}

static void make_request(t_arp_packet *packet)
{
    const unsigned char mac[6] = {2, 0, 0, 0, 0, 2};
    memset(packet, 0, sizeof(*packet));
    memset(packet->eth_hdr.dest_mac, 255, 6);
    memcpy(packet->eth_hdr.src_mac, mac, 6);
    packet->eth_hdr.eth_type = htons(ETH_P_ARP);
    packet->arp_hdr.hw_type = htons(ARPHRD_ETHER);
    packet->arp_hdr.proto_type = htons(ETH_P_IP);
    packet->arp_hdr.hw_size = 6;
    packet->arp_hdr.proto_size = 4;
    packet->arp_hdr.opcode = htons(1);
    memcpy(packet->arp_hdr.sender_mac, mac, 6);
    assert(inet_pton(AF_INET, "192.0.2.2", &packet->arp_hdr.sender_ip) == 1);
    assert(inet_pton(AF_INET, "192.0.2.1", &packet->arp_hdr.target_ip) == 1);
}

ssize_t __wrap_recvfrom(int fd, void *buffer, size_t length, int flags,
                       struct sockaddr *addr, socklen_t *addrlen)
{
    t_arp_packet packet;
    struct sockaddr_ll source;
    assert(fd == channels[0] && flags == 0);
    assert(addr && addrlen && *addrlen == sizeof(source));
    received++;
    assert(received <= 2);
    if (scenario("sigint"))
    {
        char byte;
        fprintf(stderr, "TEST: blocked receive\n");
        return read(channels[0], &byte, 1);
    }
    if (scenario("receive_failure")) { errno = EIO; return -1; }
    if (received == 1 && scenario("receive_eintr")) { errno = EINTR; return -1; }
    if (received == 1 && scenario("receive_timeout")) { errno = EAGAIN; return -1; }
    if (scenario("sigint_before_receive"))
    {
        assert(raise(SIGINT) == 0);
        errno = EAGAIN;
        return -1;
    }
    make_request(&packet);
    memset(&source, 0, sizeof(source));
    source.sll_family = AF_PACKET;
    source.sll_protocol = htons(ETH_P_ARP);
    source.sll_hatype = ARPHRD_ETHER;
    source.sll_halen = ETH_ALEN;
    source.sll_pkttype = PACKET_BROADCAST;
    source.sll_ifindex = scenario("first_interface") ? 7 : 11;
    if (received > 1 && (scenario("runtime_down") || scenario("runtime_removed")
        || scenario("runtime_index_failure")))
        source.sll_ifindex = 7;
    *addrlen = sizeof(source);
    if (received == 1)
    {
        if (scenario("short_address")) *addrlen = 4;
        if (scenario("empty_address")) *addrlen = 0;
        if (scenario("minimal_address"))
            *addrlen = offsetof(struct sockaddr_ll, sll_addr) + ETH_ALEN;
        if (scenario("unknown_index")) source.sll_ifindex = 99;
        if (scenario("zero_index")) source.sll_ifindex = 0;
        if (scenario("negative_index")) source.sll_ifindex = -1;
        if (scenario("wrong_family")) source.sll_family = AF_INET;
        if (scenario("wrong_hardware")) source.sll_hatype = ARPHRD_NONE;
        if (scenario("wrong_length")) source.sll_halen = 8;
        if (scenario("wrong_protocol")) source.sll_protocol = htons(ETH_P_IP);
        if (scenario("outgoing")) source.sll_pkttype = PACKET_OUTGOING;
        if (scenario("looped_packet")) source.sll_pkttype = PACKET_LOOPBACK;
        if (scenario("internal_packet")) source.sll_pkttype = PACKET_USER;
        if (scenario("host_packet")) source.sll_pkttype = PACKET_HOST;
        if (scenario("multicast_packet")) source.sll_pkttype = PACKET_MULTICAST;
        if (scenario("unicast_request")) packet.eth_hdr.dest_mac[0] = 2;
        if (scenario("multicast_request")) packet.eth_hdr.dest_mac[0] = 1;
        if (scenario("arp_hardware")) packet.arp_hdr.hw_type = htons(ARPHRD_NONE);
        if (scenario("arp_protocol")) packet.arp_hdr.proto_type = htons(ETH_P_IPV6);
        if (scenario("arp_hardware_length")) packet.arp_hdr.hw_size = 8;
        if (scenario("arp_protocol_length")) packet.arp_hdr.proto_size = 16;
        if (scenario("arp_reply")) packet.arp_hdr.opcode = htons(ARPOP_REPLY);
        if (scenario("ethernet_protocol")) packet.eth_hdr.eth_type = htons(ETH_P_IP);
        if (scenario("ethernet_sender")) packet.eth_hdr.src_mac[5] = 99;
        if (scenario("wrong_sender_ip")) packet.arp_hdr.sender_ip = 0;
        if (scenario("wrong_requested_ip")) packet.arp_hdr.target_ip = 0;
        if (scenario("unrelated_request")) packet.arp_hdr.sender_mac[5] = 99;
    }
    memcpy(addr, &source, *addrlen);
    assert(length >= sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    return received == 1 && scenario("short_packet") ? 10 : sizeof(packet);
}

ssize_t __wrap_sendto(int fd, const void *buffer, size_t length, int flags,
                     const struct sockaddr *addr, socklen_t addrlen)
{
    const t_arp_packet *packet = buffer;
    const struct sockaddr_ll *link = (const struct sockaddr_ll *)addr;
    const unsigned char source_mac[6] = {2, 0, 0, 0, 0, 1};
    const unsigned char target_mac[6] = {2, 0, 0, 0, 0, 2};
    uint32_t source_ip, target_ip;
    int expected_index = 11;
    if (scenario("first_interface") || scenario("runtime_down")
        || scenario("runtime_removed") || scenario("runtime_index_failure"))
        expected_index = 7;
    assert(fd == channels[0] && flags == 0 && length == 42);
    assert(addrlen == sizeof(*link) && link->sll_family == AF_PACKET);
    assert(link->sll_ifindex == expected_index);
    assert(link->sll_halen == 6 && link->sll_protocol == htons(ETH_P_ARP));
    assert(memcmp(link->sll_addr, target_mac, 6) == 0);
    assert(memcmp(packet->eth_hdr.dest_mac, target_mac, 6) == 0);
    assert(memcmp(packet->eth_hdr.src_mac, source_mac, 6) == 0);
    assert(memcmp(packet->arp_hdr.sender_mac, source_mac, 6) == 0);
    assert(memcmp(packet->arp_hdr.target_mac, target_mac, 6) == 0);
    assert(inet_pton(AF_INET, "192.0.2.1", &source_ip) == 1);
    assert(inet_pton(AF_INET, "192.0.2.2", &target_ip) == 1);
    assert(packet->arp_hdr.sender_ip == source_ip);
    assert(packet->arp_hdr.target_ip == target_ip);
    assert(ntohs(packet->eth_hdr.eth_type) == ETH_P_ARP);
    assert(ntohs(packet->arp_hdr.hw_type) == ARPHRD_ETHER);
    assert(ntohs(packet->arp_hdr.proto_type) == ETH_P_IP);
    assert(packet->arp_hdr.hw_size == 6 && packet->arp_hdr.proto_size == 4);
    assert(ntohs(packet->arp_hdr.opcode) == 2);
    sent++;
    assert(sent == 1 && !outstanding);
    if (scenario("send_failure")) { errno = EIO; return -1; }
    if (scenario("short_send")) { errno = 0; return length - 1; }
    return length;
}

int __real_close(int);
int __wrap_close(int fd)
{
    assert(!outstanding);
    if (fd == channels[0])
    {
        fprintf(stderr, "TEST: closed sends=%d receives=%d\n", sent, received);
        __real_close(channels[1]);
    }
    return __real_close(fd);
}
"""


def compile_program(root, stage, mocked):
    environment = dict(os.environ, CCACHE_DISABLE="1")
    for name in ("MAKEFLAGS", "MFLAGS"):
        environment.pop(name, None)
    subprocess.run(["make", "-C", str(root / "libft")], env=environment,
                   check=True, stdout=subprocess.DEVNULL)
    command = [
        "gcc", "-Wall", "-Wextra", "-Werror",
        f"-I{root / 'includes'}", f"-I{root / 'libft'}",
    ]
    command += [str(root / "src" / f"{name}.c")
                for name in ("main", "arp", "net", "interface", "mac_addr",
                             "reply", "setup")]
    if mocked:
        wrapper = stage / "wrappers.c"
        wrapper.write_text(WRAPPERS)
        command += [str(wrapper)]
        command += [
            f"-Wl,--wrap={name}" for name in
            ("socket", "getifaddrs", "freeifaddrs", "if_nametoindex",
             "bind", "setsockopt", "recvfrom", "sendto", "close", "sigaction")
        ]
    binary = stage / ("mocked" if mocked else "ft_malcolm")
    command += [str(root / "libft/libft.a"), "-o", str(binary)]
    subprocess.run(command, check=True, env=environment)
    return binary


def arguments(binary):
    return [str(binary), "192.0.2.1", "02:00:00:00:00:01",
            "192.0.2.2", "02:00:00:00:00:02"]


def mocked_checks(binary, valgrind):
    rejected = [
        "up_only", "running_only", "down", "loopback", "no_address", "no_name",
        "ipv4_address", "non_ethernet", "non_ethernet_length", "index_failure",
        "no_interface", "enumeration_failure",
    ]
    retry = [
        "short_address", "empty_address", "unknown_index", "zero_index",
        "negative_index", "wrong_family", "wrong_hardware", "wrong_length",
        "wrong_protocol", "outgoing", "looped_packet", "internal_packet",
        "unrelated_request", "short_packet", "receive_eintr", "runtime_down",
        "runtime_removed", "runtime_index_failure",
        "host_packet", "multicast_packet", "unicast_request", "multicast_request",
        "arp_hardware", "arp_protocol", "arp_hardware_length",
        "arp_protocol_length", "arp_reply", "ethernet_protocol", "ethernet_sender",
        "wrong_sender_ip", "wrong_requested_ip", "receive_timeout",
    ]
    failures = ["socket_failure", "signal_failure", "receive_failure",
                "runtime_enumeration_failure", "send_failure", "short_send",
                "timeout_setup_failure"]
    scenarios = ["second_interface", "first_interface", "mixed_candidates",
                 "minimal_address", "sigint_before_receive", "sigint_after_match"] + rejected + retry + failures
    prefix = []
    if valgrind:
        prefix = ["valgrind", "--quiet", "--leak-check=full",
                  "--errors-for-leak-kinds=all", "--error-exitcode=99"]
    for scenario in scenarios:
        result = subprocess.run(
            prefix + arguments(binary), capture_output=True, text=True,
            env=dict(os.environ, INTERFACE_SCENARIO=scenario), timeout=10,
        )
        expected_status = int(scenario in rejected + failures)
        assert result.returncode == expected_status, (scenario, result.stderr)
        if scenario != "socket_failure":
            sends = int(scenario not in rejected
                        + ["signal_failure", "receive_failure",
                           "runtime_enumeration_failure", "timeout_setup_failure",
                           "sigint_before_receive", "sigint_after_match"])
            assert f"TEST: closed sends={sends}" in result.stderr, (
                scenario, result.stderr)
        if scenario in retry:
            assert "receives=2" in result.stderr, (scenario, result.stderr)
        if scenario == "short_send":
            assert "incomplete ARP reply (41/42 bytes)" in result.stderr
            assert "Success" not in result.stderr
    process = subprocess.Popen(
        prefix + arguments(binary), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        text=True, env=dict(os.environ, INTERFACE_SCENARIO="sigint"),
    )
    try:
        with selectors.DefaultSelector() as selector:
            selector.register(process.stderr, selectors.EVENT_READ)
            assert selector.select(10), "mock did not enter receive"
        assert process.stderr.readline().strip() == "TEST: blocked receive"
        # The notification precedes read(). Wait for the actual blocking syscall
        # so SIGINT cannot arrive between that notification and entering read().
        deadline = time.monotonic() + 5
        while "pipe_read" not in Path(f"/proc/{process.pid}/wchan").read_text():
            assert time.monotonic() < deadline, "mock did not block in read"
            time.sleep(0.005)
        process.send_signal(signal.SIGINT)
        _, stderr = process.communicate(timeout=10)
        assert process.returncode == 0 and "closed sends=0" in stderr, stderr
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
    print(f"PASS: {len(scenarios) + 1} mocked interface/runtime scenarios")


def request_frame():
    mac = bytes.fromhex("020000000002")
    return (b"\xff" * 6 + mac + struct.pack("!H", 0x0806)
            + struct.pack("!HHBBH", 1, 0x0800, 6, 4, 1)
            + mac + socket.inet_aton("192.0.2.2") + b"\0" * 6
            + socket.inet_aton("192.0.2.1"))


def reply_frame():
    source = bytes.fromhex("020000000001")
    target = bytes.fromhex("020000000002")
    return (target + source + struct.pack("!H", 0x0806)
            + struct.pack("!HHBBH", 1, 0x0800, 6, 4, 2)
            + source + socket.inet_aton("192.0.2.1")
            + target + socket.inet_aton("192.0.2.2"))


def invalid_request_frames():
    valid = request_frame()

    def change(offset, data):
        return valid[:offset] + data + valid[offset + len(data):]

    return {
        "unicast": change(0, b"\x02"),
        "multicast": change(0, b"\x01"),
        "hardware type": change(14, b"\x00\x00"),
        "protocol type": change(16, b"\x86\xdd"),
        "hardware length": change(18, b"\x05"),
        "protocol length": change(19, b"\x10"),
        "reply opcode": change(20, b"\x00\x02"),
        "Ethernet source": change(11, b"\x03"),
        "ARP source": change(27, b"\x03"),
        "sender IP": change(28, socket.inet_aton("192.0.2.3")),
        "requested IP": change(38, socket.inet_aton("192.0.2.3")),
        "Ethernet protocol": change(12, b"\x08\x00"),
        "short frame": valid[:20],
    }


def kernel_peer():
    """A target with its own network namespace and independent neighbor table."""
    peer = None

    def ip(*args):
        return subprocess.check_output(["ip", *args], text=True)

    def report(value):
        print(json.dumps(value), flush=True)

    report({"status": "ready"})
    try:
        for line in sys.stdin:
            command = json.loads(line)["command"]
            if command == "quit":
                return
            if command == "configure":
                ip("link", "set", "right1", "address", "02:00:00:00:00:02")
                ip("link", "set", "right1", "up")
                ip("address", "add", "192.0.2.2/24", "dev", "right1")
                peer = socket.socket(socket.AF_PACKET, socket.SOCK_RAW,
                                     socket.htons(0x0806))
                peer.bind(("right1", 0))
                report({"status": "configured"})
            elif command == "invalid":
                assert peer is not None
                for name, frame in invalid_request_frames().items():
                    peer.send(frame)
                    deadline = time.monotonic() + 0.04
                    while time.monotonic() < deadline:
                        peer.settimeout(max(0.001, deadline - time.monotonic()))
                        try:
                            packet = peer.recv(2048)
                        except socket.timeout:
                            break
                        assert packet[:42] != reply_frame(), name
                report({"ignored": len(invalid_request_frames())})
            elif command == "probe":
                assert peer is not None
                peer.settimeout(3)
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as trigger:
                    trigger.bind(("192.0.2.2", 0))
                    trigger.sendto(b"probe", ("192.0.2.1", 9))
                while True:
                    packet, address = peer.recvfrom(2048)
                    if packet[:42] == reply_frame():
                        assert address[0] == "right1"
                        assert address[2] != socket.PACKET_OUTGOING
                        break
                deadline = time.monotonic() + 0.1
                while time.monotonic() < deadline:
                    peer.settimeout(max(0.001, deadline - time.monotonic()))
                    try:
                        packet = peer.recv(2048)
                    except socket.timeout:
                        break
                    assert packet[:42] != reply_frame(), "duplicate reply"
                deadline = time.monotonic() + 1
                while True:
                    neighbor = ip("neigh", "show", "192.0.2.1", "dev", "right1")
                    if "02:00:00:00:00:01" in neighbor:
                        break
                    assert time.monotonic() < deadline, neighbor
                    time.sleep(0.01)
                report({"reply": "valid", "neighbor": neighbor.strip()})
            else:
                raise ValueError(command)
    finally:
        if peer is not None:
            peer.close()


def start_real_program(binary):
    master, slave = pty.openpty()
    process = subprocess.Popen(arguments(binary), stdout=slave,
                               stderr=subprocess.PIPE)
    os.close(slave)
    output = b""
    deadline = time.monotonic() + 3
    with selectors.DefaultSelector() as selector:
        selector.register(master, selectors.EVENT_READ)
        while b"Waiting for ARP request" not in output:
            assert time.monotonic() < deadline, output
            if selector.select(0.1):
                output += os.read(master, 4096)
    return process, master, output


def terminal_output(master):
    output = b""
    while True:
        try:
            data = os.read(master, 4096)
            if not data:
                return output
            output += data
        except OSError as error:
            if error.errno == errno.EIO:
                return output
            raise


def kernel_checks(binary):
    """Two independent network stacks, contained inside a fresh namespace."""
    def ip(*args):
        return subprocess.check_output(["ip", *args], text=True)

    assert len(socket.if_nameindex()) == 1, "requires a fresh network namespace"
    for first, second in (("wrong0", "wrong1"), ("right0", "right1")):
        ip("link", "add", first, "type", "veth", "peer", "name", second)
        ip("link", "set", first, "up")
        if second == "wrong1":
            ip("link", "set", second, "up")
    peer = subprocess.Popen(
        ["unshare", "--net", "python3", str(Path(__file__).resolve()),
         "--kernel-peer"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True,
    )

    def response():
        with selectors.DefaultSelector() as selector:
            selector.register(peer.stdout, selectors.EVENT_READ)
            assert selector.select(5), "target namespace timed out"
        line = peer.stdout.readline()
        assert line, peer.stderr.read()
        return json.loads(line)

    def command(name):
        peer.stdin.write(json.dumps({"command": name}) + "\n")
        peer.stdin.flush()
        return response()

    try:
        assert response()["status"] == "ready"
        ip("link", "set", "right1", "netns", str(peer.pid))
        assert command("configure")["status"] == "configured"
        process, master, output = start_real_program(binary)
        try:
            ignored = command("invalid")["ignored"]
            assert process.poll() is None, "a malformed request triggered a reply"
            result = command("probe")
            _, stderr = process.communicate(timeout=3)
            assert process.returncode == 0, stderr
            output += terminal_output(master)
            assert b"Interface selected: right0" in output, output
            assert b"Interface selected: wrong" not in output, output
            assert output.count(b"ARP reply sent.") == 1, output
            assert result["reply"] == "valid"
            assert "02:00:00:00:00:01" in result["neighbor"]
            print(f"PASS: {ignored} invalid/unicast frames ignored; one valid "
                  "reply on the receiving adapter; independent target ARP cache updated")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            os.close(master)
        process, master, output = start_real_program(binary)
        try:
            # Wait through a receive timeout: an idle network must keep waiting.
            time.sleep(1.1)
            assert process.poll() is None
            process.send_signal(signal.SIGINT)
            _, stderr = process.communicate(timeout=3)
            output += terminal_output(master)
            assert process.returncode == 0, stderr
            assert b"ARP reply sent." not in output
            print("PASS: receive timeout keeps waiting; SIGINT exits without sending")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            os.close(master)
    finally:
        if peer.poll() is None:
            peer.stdin.write(json.dumps({"command": "quit"}) + "\n")
            peer.stdin.flush()
            try:
                peer.wait(timeout=3)
            except subprocess.TimeoutExpired:
                peer.kill()
                peer.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--valgrind", action="store_true")
    parser.add_argument("--kernel", action="store_true")
    parser.add_argument("--kernel-peer", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--kernel-inside", type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.kernel_peer:
        kernel_peer()
        return
    if args.kernel_inside:
        kernel_checks(args.kernel_inside)
        return
    root = Path(__file__).resolve().parent.parent
    with tempfile.TemporaryDirectory(prefix="ft-malcolm-interfaces-") as directory:
        stage = Path(directory)
        if args.kernel:
            binary = compile_program(root, stage, mocked=False)
            subprocess.run([
                "unshare", "--user", "--map-root-user", "--net",
                "python3", str(Path(__file__).resolve()),
                "--kernel-inside", str(binary),
            ], check=True, timeout=20)
        else:
            mocked_checks(compile_program(root, stage, mocked=True),
                          args.valgrind)


if __name__ == "__main__":
    main()

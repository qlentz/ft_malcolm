#!/usr/bin/env python3
"""Verify the running Compose demo, captured ARP replies, and target cache."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import selectors
import socket
import struct
import subprocess
import tempfile
import time


ATTACKER = "arp_attacker"
VICTIM = "arp_victim"
SOURCE_IP = "192.168.0.42"
SOURCE_MAC = "02:42:aa:aa:00:42"
TARGET_IP = "192.168.0.3"
TARGET_MAC = "02:42:aa:aa:00:03"
PROGRAM = ["/opt/ft_malcolm/ft_malcolm", SOURCE_IP, SOURCE_MAC,
           TARGET_IP, TARGET_MAC]


class Demo:
    def __init__(self, sudo):
        self.docker = ["sudo", "-n", "docker"] if sudo else ["docker"]

    def run(self, *args, check=True):
        result = subprocess.run(self.docker + list(args), capture_output=True,
                                text=True, timeout=20)
        assert not check or result.returncode == 0, result.stdout + result.stderr
        return result

    def start(self, container, command, ready, tty=False):
        args = self.docker + ["exec"] + (["-t"] if tty else [])
        args += [container, "sh", "-c",
                 'printf "CHECK_PID=%s\\n" "$$" >&2; exec "$@"',
                 "demo-check", *command]
        process = subprocess.Popen(args, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT if tty else subprocess.PIPE,
                                   bufsize=0)
        stream = process.stdout if tty else process.stderr
        output = b""
        deadline = time.monotonic() + 5
        with selectors.DefaultSelector() as selector:
            selector.register(stream, selectors.EVENT_READ)
            while ready not in output:
                assert time.monotonic() < deadline, output
                if selector.select(0.1):
                    chunk = os.read(stream.fileno(), 4096)
                    assert chunk, output
                    output += chunk
        pid = re.search(rb"CHECK_PID=(\d+)", output)
        assert pid, output
        return process, pid[1].decode(), output

    def stop(self, process, container, pid):
        if process.poll() is None:
            self.run("exec", container, "kill", "-INT", pid, check=False)
        try:
            return process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            self.run("exec", container, "kill", "-KILL", pid, check=False)
            process.kill()
            process.communicate()
            raise

    def capture(self):
        return self.start(VICTIM, ["tcpdump", "-U", "-n", "-i", "eth0",
                                  "-w", "-", "arp and host " + SOURCE_IP],
                          b"listening on eth0")

    def flush(self):
        self.run("exec", VICTIM, "ip", "neigh", "flush", "to", SOURCE_IP,
                 "dev", "eth0")


def packets(data):
    assert len(data) >= 24, "missing pcap header"
    endian = {b"\xd4\xc3\xb2\xa1": "<", b"\xa1\xb2\xc3\xd4": ">"}[data[:4]]
    assert struct.unpack_from(endian + "I", data, 20)[0] == 1, "not Ethernet"
    result = []
    offset = 24
    while offset < len(data):
        assert len(data) - offset >= 16, "truncated pcap record"
        size = struct.unpack_from(endian + "I", data, offset + 8)[0]
        offset += 16
        assert len(data) - offset >= size, "truncated frame"
        result.append(data[offset:offset + size])
        offset += size
    return result


def expected_reply():
    source = bytes.fromhex(SOURCE_MAC.replace(":", ""))
    target = bytes.fromhex(TARGET_MAC.replace(":", ""))
    return (target + source + struct.pack("!HHHBBH", 0x0806, 1, 0x0800, 6, 4, 2)
            + source + socket.inet_aton(SOURCE_IP)
            + target + socket.inet_aton(TARGET_IP))


def fixture_checks(demo, root):
    for name, address, mac in ((ATTACKER, "192.168.0.2", "02:42:aa:aa:00:02"),
                               (VICTIM, TARGET_IP, TARGET_MAC)):
        info = json.loads(demo.run("inspect", name).stdout)[0]
        assert info["State"]["Running"], name
        networks = info["NetworkSettings"]["Networks"].values()
        assert any(net["IPAddress"] == address and net["MacAddress"] == mac
                   for net in networks), name
        demo.run("exec", name, "sh", "-c",
                 "command -v ip && command -v ping && command -v arping && command -v tcpdump")
    assert not demo.run("exec", ATTACKER, "pidof", "ft_malcolm", check=False).stdout, \
        "stop the existing demo program before running this check"
    paths = [Path("Makefile"), Path("libft/Makefile")]
    for directory in ("src", "includes", "libft"):
        paths += sorted(path.relative_to(root) for path in (root / directory).iterdir()
                        if path.suffix in (".c", ".h"))
    hashes = demo.run("exec", ATTACKER, "sha256sum", *map(str, paths)).stdout
    for line in hashes.splitlines():
        digest, name = line.split(maxsplit=1)
        assert hashlib.sha256((root / name).read_bytes()).hexdigest() == digest, name
    print("PASS: Compose addresses/tools and every built C/header source match this checkout", flush=True)


def error_checks(demo):
    for args in ([], ["bad-ip", *PROGRAM[2:]],
                 [SOURCE_IP, "100000002:00:00:00:00:01", TARGET_IP, TARGET_MAC]):
        result = demo.run("exec", ATTACKER, PROGRAM[0], *args, check=False)
        assert result.returncode == 1 and "Error:" in result.stderr, result
    image = demo.run("inspect", "--format", "{{.Image}}", ATTACKER).stdout.strip()
    result = demo.run("run", "--rm", "--network", "none", "--cap-drop", "ALL",
                      image, *PROGRAM, check=False)
    assert result.returncode == 1 and "socket:" in result.stderr, result
    with tempfile.TemporaryDirectory(prefix="ft-malcolm-docker-") as directory:
        makefile = Path(directory) / "Makefile"
        makefile.write_text("all:\n\t@exit 7\n")
        result = demo.run("run", "--rm", "--network", "none", "--mount",
                          f"type=bind,src={makefile},dst=/opt/ft_malcolm/Makefile,readonly",
                          image, "sh", "-c", "echo BAD_STARTED", check=False)
        assert result.returncode != 0
        assert "BAD_STARTED" not in result.stdout + result.stderr, result
    print("PASS: real-container CLI rejection, raw-socket permission failure, and fail-fast startup", flush=True)


def exchange(demo, interrupt):
    capture, capture_pid, _ = demo.capture()
    process = None
    try:
        demo.flush()
        process, pid, output = demo.start(ATTACKER, PROGRAM,
                                         b"Waiting for ARP request", tty=True)
        if interrupt:
            time.sleep(1.1)
            assert process.poll() is None, "idle timeout terminated the program"
            rest, _ = demo.stop(process, ATTACKER, pid)
            output += rest
        demo.run("exec", VICTIM, "ping", "-c", "1", "-W", "1", SOURCE_IP,
                 check=False)
        if not interrupt:
            rest, _ = process.communicate(timeout=5)
            output += rest
        assert process.returncode == 0, output
        assert output.count(b"ARP reply sent.") == int(not interrupt), output
        neighbors = json.loads(demo.run("exec", VICTIM, "ip", "-j", "neigh",
                                        "show", "to", SOURCE_IP, "dev", "eth0").stdout)
        learned = any(item.get("lladdr") == SOURCE_MAC for item in neighbors)
        assert learned == (not interrupt), neighbors
        data, _ = demo.stop(capture, VICTIM, capture_pid)
        assert capture.returncode == 0
        frames = packets(data)
        requests = [frame for frame in frames if len(frame) >= 42
                    and frame[12:14] == b"\x08\x06" and frame[20:22] == b"\x00\x01"
                    and frame[38:42] == socket.inet_aton(SOURCE_IP)]
        replies = [frame for frame in frames if len(frame) >= 42
                   and frame[12:14] == b"\x08\x06" and frame[20:22] == b"\x00\x02"]
        assert requests and all(frame[:6] == b"\xff" * 6 for frame in requests)
        assert len(replies) == int(not interrupt), replies
        if replies:
            assert replies[0][:42] == expected_reply(), replies[0].hex()
        if interrupt:
            print("PASS: container idle wait and SIGINT exit 0; capture contains no reply", flush=True)
        else:
            print(f"PASS: one exact 42-byte reply; victim learns {SOURCE_IP} -> {SOURCE_MAC}", flush=True)
    finally:
        if process is not None:
            demo.stop(process, ATTACKER, pid)
        demo.stop(capture, VICTIM, capture_pid)
        demo.flush()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sudo", action="store_true", help="run Docker using sudo -n")
    args = parser.parse_args()
    demo = Demo(args.sudo)
    fixture_checks(demo, Path(__file__).resolve().parent.parent)
    error_checks(demo)
    exchange(demo, interrupt=True)
    exchange(demo, interrupt=False)


if __name__ == "__main__":
    main()

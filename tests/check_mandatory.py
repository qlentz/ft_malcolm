#!/usr/bin/env python3
"""Check mandatory packet parsing, MAC input, CLI errors, and container startup."""

from contextlib import contextmanager
import ctypes
import os
from pathlib import Path
import re
import socket
import subprocess
import sys
import tempfile

from check_interfaces import (
    arguments, compile_program, invalid_request_frames, reply_frame, request_frame,
)


class Targets(ctypes.Structure):
    _fields_ = [
        ("target_ip", ctypes.c_uint32), ("source_ip", ctypes.c_uint32),
        ("target_mac", ctypes.c_ubyte * 6), ("source_mac", ctypes.c_ubyte * 6),
    ]


@contextmanager
def quiet_c_stdout():
    libc = ctypes.CDLL(None)
    sys.stdout.flush()
    libc.fflush(None)
    saved = os.dup(1)
    try:
        with open(os.devnull, "wb") as sink:
            os.dup2(sink.fileno(), 1)
            yield
            libc.fflush(None)
    finally:
        os.dup2(saved, 1)
        os.close(saved)


def load_library(root, stage):
    sources = ["arp", "reply", "net", "interface", "mac_addr"]
    helpers = ["ft_memcpy", "ft_memset", "ft_memcmp", "ft_strlen",
               "ft_isdigit", "ft_tolower"]
    command = [
        "gcc", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC",
        f"-I{root / 'includes'}", f"-I{root / 'libft'}",
    ]
    command += [str(root / "src" / f"{name}.c") for name in sources]
    command += [str(root / "libft" / f"{name}.c") for name in helpers]
    command += ["-o", str(stage / "mandatory.so")]
    subprocess.run(command, check=True, env=dict(os.environ, CCACHE_DISABLE="1"))
    library = ctypes.CDLL(str(stage / "mandatory.so"))
    library.process_incoming_packet.argtypes = [
        ctypes.c_void_p, ctypes.c_ssize_t, ctypes.POINTER(Targets),
    ]
    library.mac_string_to_bytes.argtypes = [ctypes.c_char_p, ctypes.c_void_p]
    library.is_valid_mac_address.argtypes = [ctypes.c_char_p]
    library.create_arp_reply.argtypes = [ctypes.c_void_p, ctypes.POINTER(Targets)]
    library.create_arp_reply.restype = None
    return library


def packet_checks(library):
    targets = Targets()
    targets.source_ip = int.from_bytes(socket.inet_aton("192.0.2.1"), sys.byteorder)
    targets.target_ip = int.from_bytes(socket.inet_aton("192.0.2.2"), sys.byteorder)
    targets.source_mac[:] = bytes.fromhex("020000000001")
    targets.target_mac[:] = bytes.fromhex("020000000002")
    valid = request_frame()
    assert len(valid) == len(reply_frame()) == 42
    count = 0

    def check(frame, expected, length=None):
        nonlocal count
        data = ctypes.create_string_buffer(frame) if frame is not None else None
        actual = library.process_incoming_packet(
            data, len(frame) if length is None else length, ctypes.byref(targets),
        )
        assert actual == expected, (frame, length, actual, expected)
        count += 1

    with quiet_c_stdout():
        check(valid, 1)
        for length in range(42):
            check(valid[:length], 0)
        for length in (-1, -1024, -(2**31)):
            check(b"", 0, length)
        check(None, 0, 42)
        for frame in invalid_request_frames().values():
            check(frame, 0)
        for offset in range(42):
            for bit in range(8):
                frame = bytearray(valid)
                frame[offset] ^= 1 << bit
                # The request's target hardware address is not a trigger field.
                check(bytes(frame), int(32 <= offset < 38))
        check(valid + bytes(range(64)), 1)
        check(valid.replace(bytes.fromhex("020000000002"),
                            bytes.fromhex("020000000003")), 0)
    output = (ctypes.c_ubyte * 44)(*([0xa5] * 44))
    library.create_arp_reply(ctypes.byref(output, 1), ctypes.byref(targets))
    assert output[0] == output[43] == 0xa5
    assert bytes(output[1:43]) == reply_frame()
    print(f"PASS: {count} packet cases and every constructed reply byte")


def mac_checks(library):
    pattern = re.compile(rb"[0-9a-fA-F]{2}(?::[0-9a-fA-F]{2}){5}")
    count = 0

    def check(value):
        nonlocal count
        expected = value is not None and pattern.fullmatch(value) is not None
        output = (ctypes.c_ubyte * 8)(*([0xa5] * 8))
        result = library.mac_string_to_bytes(value, ctypes.byref(output, 1))
        assert result == int(expected), value
        assert library.is_valid_mac_address(value) == int(expected), value
        assert output[0] == output[7] == 0xa5
        if expected:
            assert bytes(output[1:7]) == bytes.fromhex(value.decode().replace(":", ""))
        count += 1

    for offset in range(6):
        for value in range(256):
            octets = [2, 1, 0xab, 0x70, 0xfe, 0xff]
            octets[offset] = value
            address = ":".join(f"{octet:02x}" for octet in octets).encode()
            check(address)
            check(address.upper())
    valid = b"02:aB:cd:EF:10:ff"
    for offset in range(len(valid)):
        for value in range(1, 256):
            check(valid[:offset] + bytes([value]) + valid[offset + 1:])
        check(valid[:offset])
    for value in [
        None, b"", b"100000002:00:00:00:00:01", b"0x02:00:00:00:00:01",
        b"+02:00:00:00:00:01", b"02: 00:00:00:00:01", b"2:0:0:0:0:1",
        b"02:00:00:00:00:01junk", b"aaa:bb:cc:dd:ee:ff",
        b"02-00-00-00-00-01", b"02:00:00:00:00:01\n",
        b"\xff2:00:00:00:00:01", b"0" * 4096,
    ]:
        check(value)
    assert library.mac_string_to_bytes(valid, None) == 0
    print(f"PASS: {count} MAC parser cases")


def cli_checks(binary):
    valid = arguments(binary)
    cases = [valid[:length] for length in range(1, 5)] + [valid + ["extra"]]
    bad_ips = [
        "", "999.0.0.1", "192.0.2", "-1.2.3.4", "192.0.2.1x",
        "host.example", "2130706433", "1.2.3.4.5", " 1.2.3.4",
    ]
    bad_macs = [
        "", "100000002:00:00:00:00:01", "0x02:00:00:00:00:01",
        "+02:00:00:00:00:01", "02: 00:00:00:00:01",
        "2:0:0:0:0:1", "02:00:00:00:00:01junk", "aaa:bb:cc:dd:ee:ff",
    ]
    for offset, values in ((1, bad_ips), (3, bad_ips), (2, bad_macs), (4, bad_macs)):
        for value in values:
            case = valid.copy()
            case[offset] = value
            cases.append(case)
    for case in cases:
        result = subprocess.run(case, capture_output=True, text=True, timeout=3)
        assert result.returncode == 1 and "Error:" in result.stderr, case
        assert "TEST: closed" not in result.stderr, "invalid input opened a socket"
    print(f"PASS: {len(cases)} CLI error cases")


def entrypoint_checks(root, stage):
    entrypoint = root / "test_env/attacker_entrypoint.sh"
    subprocess.run(["sh", "-n", str(entrypoint)], check=True)
    commands = stage / "commands"
    commands.mkdir()
    make = commands / "make"
    marker = stage / "started"
    environment = dict(os.environ, PATH=str(commands) + ":" + os.environ["PATH"])
    for status in (0, 7):
        marker.unlink(missing_ok=True)
        make.write_text(f"#!/bin/sh\nexit {status}\n")
        make.chmod(0o755)
        result = subprocess.run(
            ["sh", str(entrypoint), "sh", "-c", 'printf started > "$1"',
             "startup-check", str(marker)],
            cwd=stage, env=environment, capture_output=True, text=True, timeout=3,
        )
        assert result.returncode == status
        assert marker.exists() == (status == 0)
    print("PASS: container entrypoint starts only after a successful build")


def contract_checks(root, stage):
    binary = compile_program(root, stage, mocked=False)
    allowed = set((
        "sendto recvfrom socket setsockopt inet_pton inet_ntop if_nametoindex "
        "sleep getuid close sigaction signal inet_addr gethostbyname getaddrinfo "
        "freeaddrinfo getifaddrs freeifaddrs htons ntohs strerror gai_strerror "
        "printf dprintf sprintf snprintf fprintf vprintf vdprintf vsprintf "
        "vsnprintf vfprintf __errno_location __libc_start_main __cxa_finalize "
        "__stack_chk_fail puts"
    ).split())
    imports = set()
    for line in subprocess.check_output(["nm", "-u", str(binary)], text=True).splitlines():
        parts = line.split()
        if parts[-2].lower() != "w":
            imports.add(parts[-1].split("@")[0])
    assert not imports - allowed, imports - allowed
    objects = []
    for source in sorted((root / "src").glob("*.c")):
        target = stage / (source.stem + ".o")
        subprocess.run(
            ["gcc", "-Wall", "-Wextra", "-Werror", f"-I{root / 'includes'}",
             f"-I{root / 'libft'}", "-c", str(source), "-o", str(target)],
            env=dict(os.environ, CCACHE_DISABLE="1"), check=True,
        )
        objects.append(str(target))
    variables = []
    for line in subprocess.check_output(
        ["nm", "--defined-only", *objects], text=True,
    ).splitlines():
        parts = line.split()
        if (len(parts) == 3 and parts[1] in "bBdDrRsSgG"
                and not parts[2].startswith(".")):
            variables.append(parts[2])
    assert variables == ["g_stop"], variables
    print("PASS: function whitelist and one project global variable")


def main():
    root = Path(__file__).resolve().parent.parent
    with tempfile.TemporaryDirectory(prefix="ft-malcolm-mandatory-") as directory:
        stage = Path(directory)
        library = load_library(root, stage)
        packet_checks(library)
        mac_checks(library)
        cli_checks(compile_program(root, stage, mocked=True))
        entrypoint_checks(root, stage)
        contract_checks(root, stage)


if __name__ == "__main__":
    main()

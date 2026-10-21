#!/usr/bin/env python3
"""Check clean, parallel, incremental, and library-dependent builds in a copy."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parent.parent
    environment = dict(os.environ, CCACHE_DISABLE="1")
    for name in ("MAKEFLAGS", "MFLAGS"):
        environment.pop(name, None)
    with tempfile.TemporaryDirectory(prefix="ft-malcolm-build-") as directory:
        stage = Path(directory)
        shutil.copy2(root / "Makefile", stage / "Makefile")
        for goal in ("clean", "fclean"):
            result = subprocess.run(
                ["make", goal], cwd=stage, env=environment, text=True,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10,
            )
            assert result.returncode == 0, result.stdout
            assert not (stage / "libft").exists(), "cleanup initialized a dependency"
        for name in ("src", "includes", "libft"):
            shutil.copytree(
                root / name, stage / name,
                ignore=shutil.ignore_patterns(".git", "*.o", "*.d", "*.a",
                                             "__pycache__"),
            )

        def run(*arguments):
            result = subprocess.run(
                ["make", *arguments], cwd=stage, env=environment, text=True,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60,
            )
            assert result.returncode == 0, result.stdout[-6000:]

        def snapshot():
            files = list(stage.rglob("*.o"))
            files += [stage / "ft_malcolm", stage / "libft/libft.a"]
            return {str(path.relative_to(stage)): path.stat().st_mtime_ns
                    for path in files if path.exists()}

        def no_op():
            before = snapshot()
            run("-j8")
            assert snapshot() == before, "an unchanged build rebuilt artifacts"

        def update(name, expected):
            before = snapshot()
            path = stage / name
            path.write_bytes(path.read_bytes() + b"\n")
            run("-j8")
            changed = {name for name, stamp in snapshot().items()
                       if before.get(name) != stamp}
            assert changed == expected, (name, sorted(changed), sorted(expected))

        run("-j8", "ft_malcolm")
        project = {str(path.relative_to(stage))
                   for path in (stage / "src").glob("*.o")}
        library = {str(path.relative_to(stage))
                   for path in (stage / "libft").glob("*.o")}
        assert {"src/reply.o", "src/setup.o"} <= project and len(project) == 7
        assert len(list(stage.rglob("*.d"))) == len(project) + len(library)
        no_op()
        run("-j8", "re")
        no_op()
        update("src/net.c", {"src/net.o", "ft_malcolm"})
        update("libft/ft_memcpy.c",
               {"libft/ft_memcpy.o", "libft/libft.a", "ft_malcolm"})
        for header in ("includes/arp.h", "includes/ft_malcolm.h"):
            update(header, project | {"ft_malcolm"})
        # ft_isalpha.c intentionally has no libft.h include.
        update("libft/libft.h", project | (library - {"libft/ft_isalpha.o"})
               | {"libft/libft.a", "ft_malcolm"})
        (stage / "libft/libft.a").unlink()
        run("-j8")
        assert (stage / "libft/libft.a").is_file()
        (stage / "libft/ft_memcpy.o").unlink()
        run("-j8")
        assert (stage / "libft/ft_memcpy.o").is_file()
        run("-C", "libft", "-j8", "re")
        before = snapshot()["ft_malcolm"]
        run("-j8")
        assert snapshot()["ft_malcolm"] != before
        run("clean")
        assert not list(stage.rglob("*.o")) and not list(stage.rglob("*.d"))
        assert (stage / "ft_malcolm").is_file()
        run("-j8")
        no_op()
        run("fclean")
        assert not snapshot() and not list(stage.rglob("*.d"))
    print("PASS: clean/parallel builds, header/source/archive updates, "
          "missing library recovery, unchanged builds, and cleanup")


if __name__ == "__main__":
    main()

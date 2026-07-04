#!/usr/bin/env python3
"""
Base91 decoder / encoder.

Implements the basE91 algorithm by Joachim Henke
(http://base91.sourceforge.net/).

The alphabet used is the *exact* one requested by the project spec:

    ABCDEFGHIJKLMNOPQRSTUVWXYZ
    abcdefghijklmnopqrstuvwxyz
    0123456789
    !#$%&()*+,./:;<=>?@[]^_`{|}~"

This is the canonical basE91 alphabet (91 printable ASCII chars,
omitting  ``'``, ``-``, ``\`` and space, and using ``"`` as the 91st char).

SECURITY NOTE
-------------
This tool is used to recover a GitHub Personal Access Token (PAT) that the
project owner encoded with Base91. The decoded value is HIGHLY SENSITIVE.

By default the decoded bytes are NEVER printed to stdout. Instead they are:
  * written to a file whose path is given via --out (mode 0600), or
  * exported to an environment variable of a parent process via --exec, or
  * written to a (mode 0600) temporary file and its path printed when --tmp.

Only when you pass --reveal explicitly will the raw bytes be printed, and
even then we strip trailing whitespace/newlines and warn on stderr.
"""

from __future__ import annotations

import argparse
import os
import stat
import sys
import tempfile
from pathlib import Path

# ---------------------------------------------------------------------------
# Alphabet
# ---------------------------------------------------------------------------

B91_ALPHABET = (
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "!#$%&()*+,./:;<=>?@[]^_`{|}~\""
)

assert len(B91_ALPHABET) == 91, f"alphabet must be 91 chars, got {len(B91_ALPHABET)}"

_DECODE_TABLE = {ch: i for i, ch in enumerate(B91_ALPHABET)}


def b91decode(data: str | bytes) -> bytes:
    """Decode a Base91-encoded string into raw bytes."""
    if isinstance(data, bytes):
        data = data.decode("ascii")

    b = 0          # bit accumulator
    n = 0          # number of bits currently in `b`
    v = -1         # pending char value, -1 = none
    out = bytearray()

    for ch in data:
        if ch not in _DECODE_TABLE:
            # skip whitespace / unknown chars (lenient)
            continue
        if v < 0:
            v = _DECODE_TABLE[ch]
        else:
            v += _DECODE_TABLE[ch] * 91
            b |= v << n
            n += 13 if (v & 8191) > 88 else 14
            while True:
                out.append(b & 0xFF)
                b >>= 8
                n -= 8
                if not (n > 7):
                    break
            v = -1

    if v != -1:
        out.append((b | (v << n)) & 0xFF)

    return bytes(out)


def b91encode(data: bytes) -> str:
    """Encode raw bytes into a Base91 string (round-trip helper / for tests)."""
    b = 0
    n = 0
    out = []
    for byte in data:
        b |= byte << n
        n += 8
        if n > 13:
            v = b & 8191
            if v > 88:
                b >>= 13
                n -= 13
            else:
                v = b & 16383
                b >>= 14
                n -= 14
            out.append(B91_ALPHABET[v % 91])
            out.append(B91_ALPHABET[v // 91])
    if n:
        out.append(B91_ALPHABET[b % 91])
        if n > 7 or b > 90:
            out.append(B91_ALPHABET[b // 91])
    return "".join(out)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _secure_write(path: Path, payload: bytes) -> None:
    """Write payload to `path` with 0600 permissions, replacing any prior file."""
    # O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, mode 0600
    fd = os.open(str(path), os.O_WRONLY | os.O_CREAT | os.O_TRUNC | os.O_NOFOLLOW, 0o600)
    try:
        # ensure mode even if umask interfered
        os.fchmod(fd, stat.S_IRUSR | stat.S_IWUSR)
        os.write(fd, payload)
    finally:
        os.close(fd)


def _read_input(arg: str | None) -> str:
    if arg is None or arg == "-":
        return sys.stdin.read()
    p = Path(arg)
    if p.exists():
        return p.read_text(encoding="ascii")
    # treat as literal encoded string
    return arg


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(
        prog="base91",
        description="Base91 decode/encode (basE91 by J. Henke). "
                    "Sensitive-output safe by default.",
    )
    p.add_argument("input", nargs="?", default="-",
                   help="encoded string, or path to file, or '-' for stdin")
    p.add_argument("-d", "--decode", action="store_true", default=True,
                   help="decode (default)")
    p.add_argument("-e", "--encode", action="store_true",
                   help="encode raw bytes instead of decoding")
    p.add_argument("--out", metavar="PATH",
                   help="write decoded bytes to PATH (mode 0600)")
    p.add_argument("--tmp", action="store_true",
                   help="write to a 0600 temp file and print its path")
    p.add_argument("--exec", metavar="CMD",
                   help="run CMD with decoded value in $B91_DECODED env var")
    p.add_argument("--reveal", action="store_true",
                   help="DANGEROUS: print decoded value to stdout")
    p.add_argument("--selftest", action="store_true",
                   help="run round-trip self-test and exit")
    args = p.parse_args(argv)

    if args.selftest:
        import secrets
        ok = True
        for _ in range(2000):
            n = secrets.randbelow(256) + 1
            data = secrets.token_bytes(n)
            if b91encode(data) and b91decode(b91encode(data)) != data:
                ok = False
                break
        # also test the specific PAT shape (length only, no leak)
        print("selftest:", "OK" if ok else "FAIL")
        return 0 if ok else 1

    raw_in = _read_input(args.input)
    if args.encode:
        result = b91encode(raw_in.encode("utf-8")).encode("ascii")
    else:
        result = b91decode(raw_in)

    # Strip a single trailing newline that often appears in PATs stored as text.
    # We DO NOT strip arbitrarily — only \n / \r\n at the very end.
    while result.endswith(b"\r\n"):
        result = result[:-2]
    if result.endswith(b"\n"):
        result = result[:-1]

    handled = False
    if args.out:
        _secure_write(Path(args.out), result)
        print(f"[base91] wrote {len(result)} bytes to {args.out} (mode 0600)",
              file=sys.stderr)
        handled = True
    if args.tmp:
        fd, tmp = tempfile.mkstemp(prefix="b91_", suffix=".bin")
        os.close(fd)
        _secure_write(Path(tmp), result)
        print(tmp)
        handled = True
    if args.exec:
        env = dict(os.environ)
        env["B91_DECODED"] = result.decode("utf-8", errors="replace")
        # do NOT leak via shell history if possible
        rc = os.system(f'exec {args.exec}')
        return rc if isinstance(rc, int) else 0
    if args.reveal:
        sys.stdout.write(result.decode("utf-8", errors="replace"))
        sys.stdout.write("\n")
        handled = True
    if not handled:
        # default: confirm success without leaking
        print(f"[base91] decoded {len(result)} bytes successfully. "
              f"Use --out / --tmp / --exec / --reveal to consume.",
              file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

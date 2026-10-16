#!/usr/bin/env python3
"""Compares the helpers' biquads with the TI PurePath Console reference (ppc_reference.cfg, 96 kHz).
usage: compare.py <cfgtest binary>   (the binary takes: filter type, frequency, gain, Q)
Band Pass and Notch are defined in PPC by a bandwidth (its QVal is ignored); the helpers take Q, so they are
given Q = frequency / bandwidth."""
import re
import subprocess
import sys
from pathlib import Path

# FilterType values in tas58xx_helpers.h
TYPES = {"Equalizer (Q Factor)": 1, "Peaking EQ": 2, "Low Shelf": 3, "High Shelf": 4,
         "Low Pass-ButterWorth 2": 5, "High Pass-ButterWorth 2": 6, "Notch": 7, "Band Pass": 8}
BANDWIDTH_FILTERS = ("Notch", "Band Pass")
COEFFICIENTS = ("b0", "b1", "b2", "a1", "a2")


def signed(value: int) -> int:
    return value - 2**32 if value >= 2**31 else value


def main(binary: str) -> None:
    lines = (Path(__file__).parent / "ppc_reference.cfg").read_text().split("\n")
    compared = exact = 0
    worst = 0
    differences = []
    skipped = set()
    for i, line in enumerate(lines):
        m = re.match(r"# Filter: (.+?)\s+Frequency: (\d+) Hz(?:\s+Gain: (-?\d+)dB)?\s+QVal: ([\d.]+)"
                     r"(?:\s+Bandwidth: ([\d.]+) Hz)?", line)
        if not m:
            continue
        name, frequency, gain, q = m.group(1), m.group(2), m.group(3) or "0", m.group(4)
        if name not in TYPES:
            skipped.add(name)
            continue
        if name in BANDWIDTH_FILTERS:
            q = repr(float(frequency) / float(m.group(5)))  # PPC's QVal is not used for these filters
        reference = [int(lines[i + 1 + k].strip(), 16) for k in range(5)]
        ours = [int(v, 16) for v in subprocess.run([binary, str(TYPES[name]), frequency, gain, q],
                                                    capture_output=True, text=True, check=True).stdout.split()]
        for k in range(5):
            compared += 1
            delta = signed(reference[k]) - signed(ours[k])
            if delta == 0:
                exact += 1
            else:
                worst = max(worst, abs(delta))
                differences.append(f"{frequency:>5} Hz {gain:>3} dB {name:<24} {COEFFICIENTS[k]}: "
                                   f"helpers 0x{ours[k]:08X} reference 0x{reference[k]:08X} ({delta:+d} LSB)")
    print(f"compared {compared}, exact {exact}, different {compared - exact}, largest difference {worst} LSB "
          f"(1 LSB = 7.45e-9); not helpers: {', '.join(sorted(skipped)) or 'none'}")
    print("\n".join(differences))


main(sys.argv[1])

#!/usr/bin/env python3
"""Build a traceable candidate-parameter inventory from repository TXT notes.

This is an indexer, not a physics parser. It preserves the original line and
source so every candidate can be reviewed before entering a simulation case.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


NUMBER_RE = re.compile(r"(?<![A-Za-z])[-+]?\d+(?:[.,]\d+)?(?:[eE][-+]?\d+)?")

CATEGORIES = {
    "thermodynamics": ("pressure", "presion", "temperatura", "temperature", "ental", "cp", "cv", "gamma", "mach", "combust", "lox", "ch4", "methane", "metano"),
    "geometry": ("chamber", "camara", "throat", "garganta", "nozzle", "tobera", "diameter", "diametro", "radius", "radio", "length", "longitud", "angle", "angulo", "volume", "volumen"),
    "turbomachinery": ("pump", "bomba", "turbine", "turbina", "rpm", "npsh", "inducer", "bearing", "rodamiento", "shaft", "eje"),
    "flow": ("flow", "flujo", "caudal", "mass", "velocity", "velocidad", "viscos", "density", "densidad", "cavitation", "cavitacion", "injector", "inyector"),
    "materials": ("material", "inconel", "uhtc", "ceramic", "ceram", "steel", "acero", "thermal", "termic", "oxid", "stress", "tension"),
    "simulation": ("amrex", "nanovdb", "openvdb", "cuda", "voxel", "grid", "malla", "kernel", "boundary", "frontera"),
}


def classify(line: str) -> list[str]:
    normalized = line.casefold()
    matches = []
    for category, keywords in CATEGORIES.items():
        if any(keyword in normalized for keyword in keywords):
            matches.append(category)
    return matches or ["unclassified"]


def extract(root: Path) -> list[dict[str, object]]:
    records = []
    for path in sorted(root.rglob("*.txt")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for line_number, raw_line in enumerate(text.splitlines(), start=1):
            line = " ".join(raw_line.split())
            numbers = NUMBER_RE.findall(line)
            if not numbers or len(line) < 3:
                continue
            records.append(
                {
                    "source": path.as_posix(),
                    "line": line_number,
                    "categories": classify(line),
                    "numbers": numbers,
                    "text": line,
                }
            )
    return records


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=Path("data/parameters"))
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/parameters/normalized/txt_parameter_inventory.json"),
    )
    args = parser.parse_args()
    records = extract(args.input)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(
            {
                "schema": "candidate_parameter_inventory.v1",
                "status": "requires_review",
                "source_count": len({record["source"] for record in records}),
                "candidate_count": len(records),
                "records": records,
            },
            indent=2,
            ensure_ascii=True,
        )
        + "\n",
        encoding="utf-8",
    )
    print(f"indexed {len(records)} candidates from {args.input}")


if __name__ == "__main__":
    main()
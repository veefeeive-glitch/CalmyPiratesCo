"""Compare two named Blender inspection sweeps while tolerating render noise."""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

from PIL import Image, ImageChops, ImageFilter, ImageStat


def rms(diff: Image.Image) -> float:
    values = ImageStat.Stat(diff).rms
    return math.sqrt(sum(value * value for value in values) / len(values))


def compare(old_path: Path, new_path: Path) -> dict[str, object]:
    with Image.open(old_path) as old_source, Image.open(new_path) as new_source:
        old = old_source.convert("RGB")
        new = new_source.convert("RGB")
        if old.size != new.size:
            return {"name": new_path.name, "status": "size_changed", "old_size": old.size, "new_size": new.size}

        raw_difference = ImageChops.difference(old, new)
        old_blurred = old.filter(ImageFilter.GaussianBlur(radius=2.0))
        new_blurred = new.filter(ImageFilter.GaussianBlur(radius=2.0))
        blurred_difference = ImageChops.difference(old_blurred, new_blurred)
        return {
            "name": new_path.name,
            "status": "compared",
            "raw_rms_255": round(rms(raw_difference), 4),
            "blurred_rms_255": round(rms(blurred_difference), 4),
        }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("old", type=Path)
    parser.add_argument("new", type=Path)
    parser.add_argument("--threshold", type=float, default=1.0)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    old_files = {path.name: path for path in args.old.glob("*.png")}
    new_files = {path.name: path for path in args.new.glob("*.png")}
    names = sorted(set(old_files) | set(new_files))
    results: list[dict[str, object]] = []
    for name in names:
        if name not in old_files:
            results.append({"name": name, "status": "added"})
        elif name not in new_files:
            results.append({"name": name, "status": "removed"})
        else:
            results.append(compare(old_files[name], new_files[name]))

    material = [
        row for row in results
        if row["status"] != "compared" or float(row.get("blurred_rms_255", 999.0)) > args.threshold
    ]
    summary = {
        "old": str(args.old.resolve()),
        "new": str(args.new.resolve()),
        "threshold_blurred_rms_255": args.threshold,
        "images": len(results),
        "material_differences": len(material),
        "max_blurred_rms_255": max((float(row.get("blurred_rms_255", 0.0)) for row in results), default=0.0),
        "results": results,
    }
    output = json.dumps(summary, indent=2)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(output, encoding="utf-8")
    print(output)


if __name__ == "__main__":
    main()

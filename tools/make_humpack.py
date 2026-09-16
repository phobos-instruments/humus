#!/usr/bin/env python3
"""Bundle an organism pack into a .humpack (a zip).

    make_humpack.py <packSrcDir> <dynBinDir> <out.humpack>

Stages the pack's ASSETS only (pack.json, per-organism organism.json +
editor layout JSONs, help/) plus the compiled binaries from <dynBinDir>
(bin/<platform>/pack.so) - never the C++ sources. The result is what the
Organism Manager's "Install pack..." consumes.

A bundle.json at the root records the pack ABI it was built against
(HUM_PACK_ABI from sdk/include/hum/PackEntry.h) and the one platform tag under
bin/, so tools/pack_versions.py can list the bundle without a build tree.
"""
import json
import os
import re
import sys
import zipfile


def pack_abi():
    header = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sdk", "include", "hum", "PackEntry.h")
    with open(header, encoding="utf-8") as f:
        m = re.search(r"^#define HUM_PACK_ABI (\d+)", f.read(), re.M)
    if not m:
        raise SystemExit(f"error: no HUM_PACK_ABI in {header}")
    return int(m.group(1))


def main() -> int:
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    src, dyn, out = sys.argv[1:4]

    entries: list[tuple[str, str]] = []  # (absolute path, archive name)

    pack_json = os.path.join(src, "pack.json")
    if not os.path.isfile(pack_json):
        print(f"error: {pack_json} not found")
        return 1
    entries.append((pack_json, "pack.json"))

    cdir = os.path.join(src, "organisms")
    for folder in sorted(os.listdir(cdir)) if os.path.isdir(cdir) else []:
        fdir = os.path.join(cdir, folder)
        if not os.path.isdir(fdir):
            continue
        for name in sorted(os.listdir(fdir)):
            if name.endswith(".json"):  # organism.json + editor layout(s)
                entries.append((os.path.join(fdir, name), f"organisms/{folder}/{name}"))
        # Data an organism ships beside itself (banks, the preset tree); sources are stripped, content is not.
        for sub in ("banks", "presets"):
            sdir = os.path.join(fdir, sub)
            if not os.path.isdir(sdir):
                continue
            for root, _, files in os.walk(sdir):
                for name in sorted(files):
                    full = os.path.join(root, name)
                    entries.append((full, f"organisms/{folder}/{os.path.relpath(full, fdir)}"))

    hdir = os.path.join(src, "help")
    if os.path.isdir(hdir):
        for name in sorted(os.listdir(hdir)):
            entries.append((os.path.join(hdir, name), f"help/{name}"))

    bins = 0
    bdir = os.path.join(dyn, "bin")
    for root, _dirs, files in os.walk(bdir):
        for name in files:
            path = os.path.join(root, name)
            entries.append((path, os.path.relpath(path, dyn)))
            bins += 1
    if bins == 0:
        print(f"error: no binaries under {bdir} (build the *_dyn target first)")
        return 1
    platforms = sorted(d for d in os.listdir(bdir) if os.path.isdir(os.path.join(bdir, d)))
    if len(platforms) != 1:
        print(f"error: expected one platform under {bdir}, found {platforms}")
        return 1
    bundle = json.dumps({"abi": pack_abi(), "platform": platforms[0]}, indent=2) + "\n"

    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for path, arc in entries:
            z.write(path, arc)
        z.writestr("bundle.json", bundle)
    print(f"wrote {out} ({len(entries)} files, {os.path.getsize(out) // 1024} KiB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

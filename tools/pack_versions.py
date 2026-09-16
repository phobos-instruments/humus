#!/usr/bin/env python3
"""Write the packs manifest the Organism Manager reads, from the bundles in a folder.

    pack_versions.py <packs dir> <base url>    write <packs dir>/versions.json
    pack_versions.py --check                    self-test on temporary bundles

Every <id>-<version>-<platform>.humpack in the folder is opened for its
pack.json (id, version, notes) and its bundle.json (abi, platform - written by
tools/make_humpack.py), and digested. Per pack id the newest version wins and
its entry lists one URL and one sha256 per platform that has a bundle of that
version; a platform still on the previous version is simply not offered until
its machine ships. Runs on the server over ssh (tools/ship_release.sh --packs),
so it needs nothing but Python.

The shape is what engine/src/core/packs/PackUpdates.cpp parses:
  {"packs": [{"id", "version", "abi", "notes",
              "platforms": {tag: url}, "sha256": {tag: hex}}]}
"""
import hashlib
import json
import pathlib
import sys
import tempfile
import zipfile


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def version_key(v):
    return [int(p) if p.isdigit() else 0 for p in v.split(".")]


def describe(path):
    with zipfile.ZipFile(path) as z:
        pack = json.loads(z.read("pack.json"))
        bundle = json.loads(z.read("bundle.json"))
    return {
        "id": pack["id"],
        "version": pack["version"],
        "notes": pack.get("notes", ""),
        "abi": int(bundle["abi"]),
        "platform": bundle["platform"],
        "file": path.name,
        "sha256": digest(path),
    }


def build(folder, base_url):
    bundles = []
    for f in sorted(folder.glob("*.humpack")):
        try:
            bundles.append(describe(f))
        except (KeyError, ValueError, zipfile.BadZipFile) as e:
            print(f"  skipped {f.name}: {e}")
    packs = []
    for pid in sorted({b["id"] for b in bundles}):
        mine = [b for b in bundles if b["id"] == pid]
        newest = max(mine, key=lambda b: version_key(b["version"]))
        current = [b for b in mine if b["version"] == newest["version"]]
        abis = {b["abi"] for b in current}
        if len(abis) != 1:
            print(f"  {pid} {newest['version']}: bundles disagree on the ABI {sorted(abis)}; not listed")
            continue
        packs.append({
            "id": pid,
            "version": newest["version"],
            "abi": newest["abi"],
            "notes": newest["notes"],
            "platforms": {b["platform"]: f"{base_url.rstrip('/')}/{b['file']}" for b in current},
            "sha256": {b["platform"]: b["sha256"] for b in current},
        })
    return {"packs": packs}


def write(folder, base_url):
    manifest = build(folder, base_url)
    out = folder / "versions.json"
    out.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"pack-versions: {len(manifest['packs'])} pack(s) -> {out}")
    for p in manifest["packs"]:
        print(f"  {p['id']} {p['version']} abi {p['abi']}: {', '.join(sorted(p['platforms']))}")
    return 0


def fake_bundle(folder, pid, version, platform, abi):
    path = folder / f"{pid}-{version}-{platform}.humpack"
    with zipfile.ZipFile(path, "w") as z:
        z.writestr("pack.json", json.dumps({"id": pid, "version": version, "notes": f"{pid} {version}"}))
        z.writestr("bundle.json", json.dumps({"abi": abi, "platform": platform}))
        z.writestr(f"bin/{platform}/pack.so", platform + version)
    return path


def check():
    with tempfile.TemporaryDirectory() as tmp:
        folder = pathlib.Path(tmp)
        fake_bundle(folder, "mulch", "1.1.0", "linux-x86_64", 1)
        fake_bundle(folder, "mulch", "1.2.0", "linux-x86_64", 1)
        newer_mac = fake_bundle(folder, "mulch", "1.2.0", "darwin-arm64", 1)
        fake_bundle(folder, "mulch", "1.1.0", "windows-x86_64", 1)
        fake_bundle(folder, "other", "0.1.0", "linux-x86_64", 2)
        (folder / "stray.humpack").write_bytes(b"not a zip")
        m = build(folder, "https://example.invalid/packs/")
        by_id = {p["id"]: p for p in m["packs"]}
        ok = True

        def expect(cond, what):
            nonlocal ok
            if not cond:
                ok = False
                print(f"pack-versions: FAIL {what}")

        expect(set(by_id) == {"mulch", "other"}, "both ids listed, the stray zip skipped")
        mulch = by_id.get("mulch", {})
        expect(mulch.get("version") == "1.2.0", "the newest version wins")
        expect(set(mulch.get("platforms", {})) == {"linux-x86_64", "darwin-arm64"},
               "only platforms holding the newest version are offered")
        expect(mulch.get("platforms", {}).get("darwin-arm64")
               == "https://example.invalid/packs/mulch-1.2.0-darwin-arm64.humpack", "url joins base and file")
        expect(mulch.get("sha256", {}).get("darwin-arm64") == digest(newer_mac), "sha256 is the file's")
        expect(mulch.get("abi") == 1 and by_id.get("other", {}).get("abi") == 2, "abi comes from bundle.json")
        expect(mulch.get("notes") == "mulch 1.2.0", "notes come from the newest pack.json")
        print("pack-versions: self-test " + ("ok" if ok else "FAILED"))
        return 0 if ok else 1


def main(argv):
    if "--check" in argv:
        return check()
    if len(argv) != 3:
        print(__doc__)
        return 2
    folder = pathlib.Path(argv[1])
    if not folder.is_dir():
        print(f"pack-versions: no {folder}/")
        return 1
    return write(folder, argv[2])


if __name__ == "__main__":
    sys.exit(main(sys.argv))

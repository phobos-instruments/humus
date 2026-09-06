#!/usr/bin/env python3
"""Convert MediaPipe's pose .tflite models into the compact form Skeleton reads.

    python3 tools/pose_model_convert.py <pose_detector.tflite> \\
        <pose_landmarks_detector.tflite> <out.humnet>

The two models come from Google's pose_landmarker_lite.task bundle (Apache-2.0,
see the model card); this repository does not redistribute them. Run once,
commit the result, or ship the .humnet as an asset - see
packs/av/assets/Models/body/README.md.

This rides on tools/hand_model_convert.py (same container, same interpreter)
and adds the three things the pose graphs need on top of the hand ones:

  * The detector stores some 1x1-conv weights sparse (CSR over the input-
    channel axis, behind DENSIFY nodes). The interpreter should not carry a
    sparse decoder for a handful of small tensors, so they are densified here,
    at conversion time, and the DENSIFY nodes folded away.
  * The detector upsamples with DEPTH_TO_SPACE, which the interpreter learned
    as kind 13; models using it are emitted as HNET version 2 so an
    interpreter that predates the op refuses the file instead of mis-running
    it. The hands file stays version 1, byte for byte.
  * The landmark model also decodes a segmentation mask, heatmaps and world
    coordinates that Skeleton never reads. Everything not needed for the first two
    outputs (screen landmarks and presence) is pruned, ops and weights both -
    about a third of the graph.
"""
import collections
import gzip
import io
import pathlib
import struct
import sys

import hand_model_convert as hc

POSE_VERSION = 2


def read_sparsity(path):
    """Map tensor index -> (segments, indices) for every sparse tensor.

    Only the shape the pose detector actually uses is accepted: traversal
    order 0,1,2,3, no block sparsity, all dimensions dense except the last,
    which is CSR. Anything else fails loudly.
    """
    raw = open(path, "rb").read()
    buf = hc.Buf(raw)
    model = hc.Table(buf, buf.u32(0))
    sg = model.tvec(2)[0]

    def union_vec(owner, type_field, value_field):
        kind = owner.num(type_field, "u8", 0)
        table = owner.table(value_field)
        if table is None:
            return None
        start, count = table.vec(0)
        if kind == 1:
            return [buf.i32(start + i * 4) for i in range(count)]
        if kind == 2:
            return [buf.u16(start + i * 2) for i in range(count)]
        if kind == 3:
            return [buf.u8(start + i) for i in range(count)]
        raise SystemExit("sparse index vector type %d is not handled" % kind)

    out = {}
    for ti, t in enumerate(sg.tvec(0)):
        sp = t.table(6)
        if sp is None:
            continue
        if sp.ivec(0) != [0, 1, 2, 3] or sp.ivec(1):
            raise SystemExit("tensor %d: sparsity layout is not the one this "
                             "converter was written for" % ti)
        dims = sp.tvec(2)
        for d in dims[:-1]:
            if d.num(0, "i8", 0) != 0:
                raise SystemExit("tensor %d: only last-axis CSR is handled" % ti)
        last = dims[-1]
        if last.num(0, "i8", 0) != 1:
            raise SystemExit("tensor %d: last axis is not CSR" % ti)
        segments = union_vec(last, 2, 3)
        indices = union_vec(last, 4, 5)
        out[ti] = (segments, indices)
    return out


def densify(g, sparsity):
    """Materialise sparse f16 weights and fold the DENSIFY nodes away."""
    alias, kept = {}, []
    for op in g.ops:
        if op["op"] != "DENSIFY":
            kept.append(op)
            continue
        ti = op["in"][0]
        if ti not in sparsity:
            raise SystemExit("DENSIFY over tensor %d, which carries no "
                             "sparsity table" % ti)
        t = g.tensors[ti]
        if t["type"] != hc.F16:
            raise SystemExit("sparse tensor %d is not f16" % ti)
        segments, indices = sparsity[ti]
        shape = t["shape"]
        last = shape[-1]
        rows = 1
        for d in shape[:-1]:
            rows *= d
        if len(segments) != rows + 1:
            raise SystemExit("tensor %d: %d segment rows for %d dense rows"
                             % (ti, len(segments) - 1, rows))
        packed = t["data"]
        if len(packed) != 2 * len(indices):
            raise SystemExit("tensor %d: %d packed bytes for %d indices"
                             % (ti, len(packed), len(indices)))
        dense = bytearray(2 * rows * last)
        for r in range(rows):
            for k in range(segments[r], segments[r + 1]):
                c = indices[k]
                dense[(r * last + c) * 2:(r * last + c) * 2 + 2] = \
                    packed[k * 2:k * 2 + 2]
        t["data"] = bytes(dense)
        alias[op["out"][0]] = ti
    for op in kept:
        op["in"] = [alias.get(i, i) for i in op["in"]]
        op["out"] = [alias.get(i, i) for i in op["out"]]
    g.ops = kept
    g.outputs = [alias.get(i, i) for i in g.outputs]
    return len(alias)


def prune(g, keep_outputs):
    """Drop every op and weight not feeding the kept outputs."""
    g.outputs = [g.outputs[i] for i in keep_outputs]
    needed = set(g.outputs)
    kept = []
    for op in reversed(g.ops):
        if any(o in needed for o in op["out"]):
            kept.append(op)
            needed.update(op["in"])
    dropped_ops = len(g.ops) - len(kept)
    g.ops = list(reversed(kept))
    stripped = 0
    for i, t in enumerate(g.tensors):
        if t["data"] and i not in needed and i not in g.inputs:
            stripped += len(t["data"])
            t["data"] = b""
    return dropped_ops, stripped


def main():
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    det, lm, out = sys.argv[1], sys.argv[2], sys.argv[3]

    parts = []
    for path, label, want_in, want_out, keep in (
            (det, "detector", [1, 224, 224, 3],
             [[1, 2254, 12], [1, 2254, 1]], [0, 1]),
            (lm, "landmarks", [1, 256, 256, 3],
             [[1, 195], [1, 1], [1, 256, 256, 1], [1, 64, 64, 39], [1, 117]],
             [0, 1])):
        g = hc.load(path)
        densified = densify(g, read_sparsity(path))
        folded = hc.fold_dequantize(g)
        got_in = g.tensors[g.inputs[0]]["shape"]
        got_out = [g.tensors[i]["shape"] for i in g.outputs]
        if got_in != want_in or got_out != want_out:
            raise SystemExit("%s: shapes are not the ones this was written for\n"
                             "  input  %s (want %s)\n  outputs %s (want %s)"
                             % (label, got_in, want_in, got_out, want_out))
        dropped, stripped = prune(g, keep)
        hist = collections.Counter(o["op"] for o in g.ops)
        print("%-10s %3d ops after folding %d dequantize, %d densify; "
              "pruned %d ops, %.2f MB of weights: %s"
              % (label, len(g.ops), folded, densified, dropped,
                 stripped / 1e6,
                 ", ".join("%s x%d" % (k, v) for k, v in hist.most_common())))
        parts.append(hc.emit(g, label, POSE_VERSION))

    body = struct.pack("<4sII", b"HAND", POSE_VERSION, len(parts))
    body += b"".join(struct.pack("<I", len(p)) for p in parts)
    body += b"".join(parts)
    buf = io.BytesIO()
    with gzip.GzipFile(fileobj=buf, mode="wb", compresslevel=9, mtime=0) as gz:
        gz.write(body)
    packed = buf.getvalue()
    pathlib.Path(out).write_bytes(packed)
    print("wrote %s  %.2f MB (from %.2f MB)"
          % (out, len(packed) / 1e6, len(body) / 1e6))


if __name__ == "__main__":
    main()

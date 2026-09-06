#!/usr/bin/env python3
"""Convert a .humnet model (tools/pose_model_convert.py output) into two Core ML
packages, one per part (detector, landmarks), so Skeleton can run on the Apple
Neural Engine while HumNet stays the portable fallback.

    python3 tools/humnet_to_coreml.py packs/av/assets/Models/body/body.humnet \
        <outdir> [refdir]

Writes <outdir>/det.mlpackage and <outdir>/lm.mlpackage. With a refdir from
`hum_snapshot --humnet-dump` it also predicts on the dumped inputs and prints
the max abs divergence against the C++ interpreter - the parity gate: refuse
to ship a conversion whose outputs drift.

Both nets ship at fp16, the Neural Engine's precision; --fp32 forces full
precision (GPU/CPU) for comparison runs. The parity gate always runs at
fp32 because it proves the graph, not the precision.

Layout strategy: HumNet is NHWC end to end; MIL convolutions want NCHW. The
graph runs in two zones. The conv zone keeps vars in NCHW (input transposed
once on entry). The first Reshape of a var transposes it back to NHWC and
from there the head zone (reshape/concat over SSD rows) works on logical
shapes with no further transposes. Outputs are emitted in NHWC/logical order
so the C++ side reads the same flat float layout it reads from HumNet.
"""
import gzip
import json
import struct
import sys

import numpy as np

KIND = {1: "conv", 2: "depthwise", 3: "add", 4: "prelu", 5: "maxpool",
        6: "dense", 7: "logistic", 8: "mean", 9: "pad", 10: "concat",
        11: "reshape", 12: "resize", 13: "depth_to_space"}


class Tensor:
    def __init__(self, rank, shape, data):
        self.rank = rank
        self.shape = shape          # always padded to 4, NHWC-style
        self.data = data            # np.float32 array or None


class Net:
    def __init__(self):
        self.tensors = []
        self.ops = []
        self.inputs = []
        self.outputs = []


def parse_part(buf):
    if buf[:4] != b"HNET":
        raise ValueError("not a HNET part")
    ver, n_t, n_o, n_in, n_out = struct.unpack_from("<5I", buf, 4)
    if ver not in (1, 2):
        raise ValueError(f"HNET version {ver}")
    at = 24
    net = Net()
    net.inputs = list(struct.unpack_from(f"<{n_in}i", buf, at)); at += 4 * n_in
    net.outputs = list(struct.unpack_from(f"<{n_out}i", buf, at)); at += 4 * n_out
    t_bytes, o_bytes, _b_bytes = struct.unpack_from("<3I", buf, at); at += 12
    t_at, o_at = at, at + t_bytes
    blob_at = o_at + o_bytes
    for i in range(n_t):
        rec = t_at + i * 26
        typ, rank = struct.unpack_from("<2B", buf, rec)
        shape = list(struct.unpack_from("<4i", buf, rec + 2))
        off, nbytes = struct.unpack_from("<2I", buf, rec + 18)
        if 0 < rank < 4:
            padded = [1, 1, 1, 1]
            padded[4 - rank:] = shape[:rank]
            shape = padded
        count = int(np.prod(shape))
        data = None
        if nbytes:
            raw = buf[blob_at + off:blob_at + off + nbytes]
            if typ == 1:
                data = np.frombuffer(raw, np.float16, count).astype(np.float32)
            elif typ == 2:
                data = np.frombuffer(raw, np.int32, count).astype(np.float32)
            else:
                data = np.frombuffer(raw, np.float32, count).copy()
        net.tensors.append(Tensor(rank, shape, data))
    at = o_at
    for _ in range(n_o):
        kind = buf[at]; at += 1
        p = list(struct.unpack_from("<5i", buf, at)); at += 20
        ni, no = buf[at], buf[at + 1]; at += 2
        ins = list(struct.unpack_from(f"<{ni}i", buf, at)); at += 4 * ni
        outs = list(struct.unpack_from(f"<{no}i", buf, at)); at += 4 * no
        net.ops.append((kind, p, ins, outs))
    return net


def load(path):
    raw = open(path, "rb").read()
    if raw[:2] == b"\x1f\x8b":
        raw = gzip.decompress(raw)
    if raw[:4] != b"HAND":
        raise ValueError("not a humnet file")
    parts = struct.unpack_from("<I", raw, 8)[0]
    if parts != 2:
        raise ValueError("expected 2 parts")
    la, lb = struct.unpack_from("<2I", raw, 12)
    return parse_part(raw[20:20 + la]), parse_part(raw[20 + la:20 + la + lb])


def pad_for(in_size, out_size, k, stride):
    need = (out_size - 1) * stride + k - in_size
    if need <= 0:
        return 0, 0
    return need // 2, need - need // 2


def act_of(v, fused):
    from coremltools.converters.mil import Builder as mb
    if fused == 1:
        return mb.relu(x=v)
    if fused == 3:
        return mb.clip(x=v, alpha=0.0, beta=6.0)
    return v


def build_mil(net, name):
    import coremltools as ct
    from coremltools.converters.mil import Builder as mb

    in_t = net.tensors[net.inputs[0]]
    in_shape = tuple(in_t.shape)

    @mb.program(input_specs=[mb.TensorSpec(shape=in_shape)],
                opset_version=ct.target.iOS16)
    def prog(in0):
        # var per tensor id + whether it currently sits in NCHW
        var = {net.inputs[0]: mb.transpose(x=in0, perm=[0, 3, 1, 2])}
        nchw = {net.inputs[0]: True}

        def logical(tid):
            if nchw.get(tid):
                var[tid] = mb.transpose(x=var[tid], perm=[0, 2, 3, 1])
                nchw[tid] = False
            return var[tid]

        def const_of(tid):
            t = net.tensors[tid]
            return t.data.reshape([d for d in t.shape])

        for kind, p, ins, outs in net.ops:
            k = KIND[kind]
            o = outs[0]
            ot = net.tensors[o]
            if k in ("conv", "depthwise"):
                x = var[ins[0]]
                w = net.tensors[ins[1]]
                b = net.tensors[ins[2]]
                it = net.tensors[ins[0]]
                kh, kw = w.shape[1], w.shape[2]
                sh, sw = p[2], p[1]
                py = pad_for(it.shape[1], ot.shape[1], kh, sh)
                px = pad_for(it.shape[2], ot.shape[2], kw, sw)
                if k == "conv":
                    wd = w.data.reshape(w.shape).transpose(0, 3, 1, 2)
                    groups = 1
                else:
                    wd = w.data.reshape(w.shape).transpose(3, 0, 1, 2)
                    groups = ot.shape[3]
                v = mb.conv(x=x, weight=wd, bias=b.data.reshape(-1),
                            strides=[sh, sw], pad_type="custom",
                            pad=[py[0], py[1], px[0], px[1]], groups=groups)
                var[o] = act_of(v, p[3])
                nchw[o] = True
            elif k == "maxpool":
                x = var[ins[0]]
                it = net.tensors[ins[0]]
                fw, fh = (p[4] >> 8) & 0xFF, p[4] & 0xFF
                sh, sw = p[2], p[1]
                py = pad_for(it.shape[1], ot.shape[1], fh, sh)
                px = pad_for(it.shape[2], ot.shape[2], fw, sw)
                var[o] = mb.max_pool(x=x, kernel_sizes=[fh, fw], strides=[sh, sw],
                                     pad_type="custom",
                                     pad=[py[0], py[1], px[0], px[1]])
                nchw[o] = True
            elif k == "add":
                a_id, b_id = ins[0], ins[1]
                small = None
                if net.tensors[b_id].data is not None and net.tensors[b_id].rank == 1:
                    small = const_of(b_id).reshape(1, -1, 1, 1)
                    v = mb.add(x=var[a_id], y=small)
                    nchw[o] = nchw.get(a_id, False)
                else:
                    same = nchw.get(a_id, False) and nchw.get(b_id, False)
                    if same:
                        v = mb.add(x=var[a_id], y=var[b_id])
                        nchw[o] = True
                    else:
                        v = mb.add(x=logical(a_id), y=logical(b_id))
                        nchw[o] = False
                var[o] = act_of(v, p[3])
            elif k == "prelu":
                x = var[ins[0]]
                alpha = const_of(ins[1]).reshape(-1)
                if not nchw.get(ins[0], False):
                    raise ValueError("prelu outside the conv zone")
                var[o] = mb.prelu(x=x, alpha=alpha)
                nchw[o] = True
            elif k == "logistic":
                var[o] = mb.sigmoid(x=var[ins[0]])
                nchw[o] = nchw.get(ins[0], False)
            elif k == "pad":
                pd = const_of(ins[1]).reshape(-1)
                front = int(pd[6]) if pd.size >= 8 else 0
                back = int(pd[7]) if pd.size >= 8 else 0
                var[o] = mb.pad(x=var[ins[0]],
                                pad=[front, back, int(pd[2]), int(pd[3]),
                                     int(pd[4]), int(pd[5])],
                                mode="constant", constant_val=0.0)
                nchw[o] = True
            elif k == "concat":
                axis = p[0] + (4 - max(net.tensors[o].rank, 1)
                               if 0 < net.tensors[o].rank < 4 else 0)
                if all(nchw.get(t, False) for t in ins):
                    axis = {0: 0, 1: 2, 2: 3, 3: 1}[axis]
                    var[o] = mb.concat(values=[var[t] for t in ins], axis=axis)
                    nchw[o] = True
                else:
                    var[o] = mb.concat(values=[logical(t) for t in ins], axis=axis)
                    nchw[o] = False
            elif k == "reshape":
                var[o] = mb.reshape(x=logical(ins[0]), shape=list(ot.shape))
                nchw[o] = False
            elif k == "depth_to_space":
                var[o] = mb.depth_to_space(x=var[ins[0]], block_size=max(1, p[0]))
                nchw[o] = True
            elif k == "resize":
                align, half = p[0] != 0, p[4] != 0
                mode = ("STRICT_ALIGN_CORNERS" if align
                        else "UNALIGN_CORNERS" if half else "DEFAULT")
                var[o] = mb.resize_bilinear(x=var[ins[0]],
                                            target_size_height=ot.shape[1],
                                            target_size_width=ot.shape[2],
                                            sampling_mode=mode)
                nchw[o] = True
            elif k == "mean":
                v = mb.reduce_mean(x=var[ins[0]], axes=[2, 3], keep_dims=False)
                var[o] = mb.reshape(x=v, shape=[1, 1, 1, net.tensors[o].shape[3]])
                nchw[o] = False
            elif k == "dense":
                w = net.tensors[ins[1]]
                b = net.tensors[ins[2]]
                units, depth = w.shape[2], w.shape[3]
                flat = mb.reshape(x=logical(ins[0]), shape=[1, depth])
                v = mb.linear(x=flat, weight=w.data.reshape(units, depth),
                              bias=b.data.reshape(-1))
                var[o] = act_of(mb.reshape(x=v, shape=[1, 1, 1, units]), p[3])
                nchw[o] = False
            else:
                raise ValueError(f"unhandled op kind {kind}")

        return tuple(mb.identity(x=logical(t), name=f"out{i}")
                     for i, t in enumerate(net.outputs))

    return prog


def convert(net, name, fp32=False):
    import coremltools as ct
    prog = build_mil(net, name)
    return ct.convert(prog, convert_to="mlprogram",
                      compute_units=ct.ComputeUnit.ALL,
                      compute_precision=ct.precision.FLOAT32 if fp32
                                        else ct.precision.FLOAT16,
                      minimum_deployment_target=ct.target.iOS16)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    fp32 = "--fp32" in sys.argv
    args = [a for a in sys.argv[1:] if a != "--fp32"]
    src, outdir = args[0], args[1]
    refdir = args[2] if len(args) > 2 else None
    det, lm = load(src)
    import os
    os.makedirs(outdir, exist_ok=True)
    worst = 0.0
    for net, tag in ((det, "det"), (lm, "lm")):
        if refdir:
            # the gate runs at fp32: it proves the GRAPH, not the precision
            gate = convert(net, tag, fp32=True)
            in_t = net.tensors[net.inputs[0]]
            ref_in = np.fromfile(os.path.join(refdir, tag + "_in.f32"),
                                 np.float32).reshape(in_t.shape)
            got = gate.predict({"in0": ref_in})
            for i, t in enumerate(net.outputs):
                ref = np.fromfile(os.path.join(refdir, f"{tag}_out{i}.f32"),
                                  np.float32)
                out = np.asarray(got[f"out{i}"]).reshape(-1)
                d = float(np.abs(out - ref).max())
                worst = max(worst, d)
                print(f"   {tag} out{i}: n={ref.size}  fp32 max|diff|={d:.6f}")
        wide = fp32
        model = convert(net, tag, wide)
        path = os.path.join(outdir, tag + ".mlpackage")
        model.save(path)
        print(f"{tag}: saved {path} ({'fp32' if wide else 'fp16 for the ANE'})")
    if refdir:
        print(f"graph parity worst {worst:.6f} => "
              f"{'OK' if worst < 1e-2 else 'DRIFT'}")
        return 0 if worst < 1e-2 else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

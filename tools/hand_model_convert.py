#!/usr/bin/env python3
"""Convert MediaPipe's hand .tflite models into the compact form Hands reads.

    python3 tools/hand_model_convert.py <detector.tflite> <landmarks.tflite> <out.humnet>

The two models come from Google's hand_landmarker.task bundle (Apache-2.0, see
the model card); this repository does not redistribute them. Run once, commit
the result, or ship the .humnet as an asset - see docs/dev/hands.md.

Why a converter at all: the .tflite container is FlatBuffers plus a full op
registry, and every weight arrives through a DEQUANTIZE node. Folding those
away leaves twelve operators and a flat float16 blob, which a few hundred lines
of C++ can run with no third-party runtime. The graphs use no dilation, no
depth multiplier, and only RELU6 as a fused activation, so the interpreter
never grows past what is checked in here.
"""
import collections
import gzip
import io
import pathlib
import struct
import sys

# ---- FlatBuffers, only what the TFLite schema needs -------------------------

class Buf:
    def __init__(self, b): self.b = b
    def u8(self, p):  return self.b[p]
    def i8(self, p):  return struct.unpack_from("<b", self.b, p)[0]
    def i32(self, p): return struct.unpack_from("<i", self.b, p)[0]
    def u32(self, p): return struct.unpack_from("<I", self.b, p)[0]
    def u16(self, p): return struct.unpack_from("<H", self.b, p)[0]


class Table:
    def __init__(self, buf, pos):
        self.buf, self.pos = buf, pos
        self.vt = pos - buf.i32(pos)
        self.vtsize = buf.u16(self.vt)

    def off(self, field):
        vo = 4 + 2 * field
        return 0 if vo >= self.vtsize else self.buf.u16(self.vt + vo)

    def num(self, field, kind, default=0):
        o = self.off(field)
        if o == 0: return default
        return getattr(self.buf, kind)(self.pos + o)

    def vec(self, field):
        o = self.off(field)
        if o == 0: return (0, 0)
        p = self.pos + o
        v = p + self.buf.u32(p)
        return (v + 4, self.buf.u32(v))

    def ivec(self, field):
        s, n = self.vec(field)
        return [self.buf.i32(s + i * 4) for i in range(n)]

    def table(self, field):
        o = self.off(field)
        if o == 0: return None
        p = self.pos + o
        return Table(self.buf, p + self.buf.u32(p))

    def tvec(self, field):
        s, n = self.vec(field)
        return [Table(self.buf, s + i * 4 + self.buf.u32(s + i * 4)) for i in range(n)]

    def string(self, field):
        o = self.off(field)
        if o == 0: return ""
        p = self.pos + o
        v = p + self.buf.u32(p)
        return self.buf.b[v + 4:v + 4 + self.buf.u32(v)].decode("utf-8", "replace")

    def blob(self, field):
        s, n = self.vec(field)
        return self.buf.b[s:s + n]


OPS = """ADD AVERAGE_POOL_2D CONCATENATION CONV_2D DEPTHWISE_CONV_2D DEPTH_TO_SPACE
DEQUANTIZE EMBEDDING_LOOKUP FLOOR FULLY_CONNECTED HASHTABLE_LOOKUP L2_NORMALIZATION
L2_POOL_2D LOCAL_RESPONSE_NORMALIZATION LOGISTIC LSH_PROJECTION LSTM MAX_POOL_2D MUL
RELU RELU_N1_TO_1 RELU6 RESHAPE RESIZE_BILINEAR RNN SOFTMAX SPACE_TO_DEPTH SVDF TANH
CONCAT_EMBEDDINGS SKIP_GRAM CALL CUSTOM EMBEDDING_LOOKUP_SPARSE PAD
UNIDIRECTIONAL_SEQUENCE_RNN GATHER BATCH_TO_SPACE_ND SPACE_TO_BATCH_ND TRANSPOSE MEAN
SUB DIV SQUEEZE UNIDIRECTIONAL_SEQUENCE_LSTM STRIDED_SLICE BIDIRECTIONAL_SEQUENCE_RNN
EXP TOPK_V2 SPLIT LOG_SOFTMAX DELEGATE BIDIRECTIONAL_SEQUENCE_LSTM CAST PRELU MAXIMUM
ARG_MAX MINIMUM LESS NEG PADV2 GREATER GREATER_EQUAL LESS_EQUAL SELECT SLICE SIN
TRANSPOSE_CONV SPARSE_TO_DENSE TILE EXPAND_DIMS EQUAL NOT_EQUAL LOG SUM SQRT RSQRT
SHAPE POW ARG_MIN FAKE_QUANT REDUCE_PROD REDUCE_MAX PACK LOGICAL_OR ONE_HOT LOGICAL_AND
LOGICAL_NOT UNPACK REDUCE_MIN FLOOR_DIV REDUCE_ANY SQUARE ZEROS_LIKE FILL FLOOR_MOD
RANGE RESIZE_NEAREST_NEIGHBOR LEAKY_RELU SQUARED_DIFFERENCE MIRROR_PAD ABS SPLIT_V
UNIQUE CEIL REVERSE_V2 ADD_N GATHER_ND COS WHERE RANK ELU REVERSE_SEQUENCE MATRIX_DIAG
QUANTIZE MATRIX_SET_DIAG ROUND HARD_SWISH IF WHILE NON_MAX_SUPPRESSION_V4
NON_MAX_SUPPRESSION_V5 SCATTER_ND SELECT_V2 DENSIFY SEGMENT_SUM BATCH_MATMUL""".split()

F32, F16, I32 = 0, 1, 2
TYPE_MAP = {0: F32, 1: F16, 2: I32}

# Kind codes the C++ interpreter switches on. Order is the file format; append
# only, and emit with a bumped version when a model actually uses the new kind -
# the hands file stays version 1 so its pinned digest holds.
KIND = {"CONV_2D": 1, "DEPTHWISE_CONV_2D": 2, "ADD": 3, "PRELU": 4, "MAX_POOL_2D": 5,
        "FULLY_CONNECTED": 6, "LOGISTIC": 7, "MEAN": 8, "PAD": 9, "CONCATENATION": 10,
        "RESHAPE": 11, "RESIZE_BILINEAR": 12, "DEPTH_TO_SPACE": 13}
VERSION = 1


class Graph:
    pass


def load(path):
    raw = open(path, "rb").read()
    buf = Buf(raw)
    model = Table(buf, buf.u32(0))

    names = []
    for c in model.tvec(1):
        b = c.num(3, "i32", -1)
        if b < 0: b = c.num(0, "i8", 0)
        names.append(OPS[b] if 0 <= b < len(OPS) else "OP_%d" % b)

    buffers = [t.blob(0) for t in model.tvec(4)]
    sg = model.tvec(2)[0]

    g = Graph()
    g.tensors = []
    for t in sg.tvec(0):
        g.tensors.append({
            "shape": t.ivec(0),
            "type": TYPE_MAP.get(t.num(1, "u8", 0), F32),
            "data": buffers[t.num(2, "u32", 0)],
            "name": t.string(3),
        })
    g.inputs, g.outputs = sg.ivec(1), sg.ivec(2)
    g.ops = []
    for o in sg.tvec(3):
        g.ops.append({"op": names[o.num(0, "u32", 0)], "in": o.ivec(1),
                      "out": o.ivec(2), "opt": o.table(4)})
    return g


def fold_dequantize(g):
    """Every weight arrives as f16 through a DEQUANTIZE. Point consumers at the
    f16 tensor and drop the node; the interpreter widens once, at load."""
    alias, kept = {}, []
    for op in g.ops:
        if op["op"] == "DEQUANTIZE" and g.tensors[op["in"][0]]["data"]:
            alias[op["out"][0]] = op["in"][0]
        else:
            kept.append(op)
    for op in kept:
        op["in"] = [alias.get(i, i) for i in op["in"]]
        op["out"] = [alias.get(i, i) for i in op["out"]]
    g.ops = kept
    g.outputs = [alias.get(i, i) for i in g.outputs]
    return len(alias)


def op_params(op):
    """Fixed-width parameter block, meaning per kind. Asserts the shapes this
    converter was written against, so an upstream model change fails loudly
    rather than producing a graph the interpreter quietly mis-runs."""
    o, name = op["opt"], op["op"]
    if name == "CONV_2D":
        assert o.num(4, "i32", 1) == 1 and o.num(5, "i32", 1) == 1, "dilated conv"
        return [o.num(0, "u8", 0), o.num(1, "i32", 1), o.num(2, "i32", 1),
                o.num(3, "u8", 0), 0]
    if name == "DEPTHWISE_CONV_2D":
        assert o.num(3, "i32", 1) == 1, "depth_multiplier != 1"
        assert o.num(5, "i32", 1) == 1 and o.num(6, "i32", 1) == 1, "dilated conv"
        return [o.num(0, "u8", 0), o.num(1, "i32", 1), o.num(2, "i32", 1),
                o.num(4, "u8", 0), 0]
    if name == "MAX_POOL_2D":
        return [o.num(0, "u8", 0), o.num(1, "i32", 1), o.num(2, "i32", 1),
                o.num(5, "u8", 0), (o.num(3, "i32", 1) << 8) | o.num(4, "i32", 1)]
    if name == "FULLY_CONNECTED":
        return [0, 0, 0, o.num(0, "u8", 0), 0]
    if name == "ADD":
        return [0, 0, 0, o.num(0, "u8", 0), 0]
    if name == "CONCATENATION":
        return [o.num(0, "i32", 0), 0, 0, o.num(1, "u8", 0), 0]
    if name == "RESIZE_BILINEAR":
        return [o.num(2, "u8", 0), 0, 0, 0, o.num(3, "u8", 0)]
    if name == "DEPTH_TO_SPACE":
        return [o.num(0, "i32", 1), 0, 0, 0, 0]
    return [0, 0, 0, 0, 0]


def emit(g, name, version=VERSION):
    """One model: a tensor table, an op table, and the weight blob."""
    blob = bytearray()
    tensors = bytearray()
    for t in g.tensors:
        off, nbytes = 0, 0
        if t["data"]:
            off, nbytes = len(blob), len(t["data"])
            blob += t["data"]
            blob += b"\0" * (-len(blob) % 4)
        shape = t["shape"][:4]
        tensors += struct.pack("<BB4iII", t["type"], len(shape),
                               *(list(shape) + [0] * (4 - len(shape))), off, nbytes)

    ops = bytearray()
    for op in g.ops:
        if op["op"] not in KIND:
            raise SystemExit("unsupported op in %s: %s" % (name, op["op"]))
        p = op_params(op)
        ops += struct.pack("<B5i", KIND[op["op"]], *p)
        ops += struct.pack("<BB", len(op["in"]), len(op["out"]))
        for i in op["in"] + op["out"]:
            ops += struct.pack("<i", i)

    head = struct.pack("<4sIIIII", b"HNET", version, len(g.tensors), len(g.ops),
                       len(g.inputs), len(g.outputs))
    head += b"".join(struct.pack("<i", i) for i in g.inputs + g.outputs)
    return head + struct.pack("<III", len(tensors), len(ops), len(blob)) \
                + bytes(tensors) + bytes(ops) + bytes(blob)


def main():
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    det, lm, out = sys.argv[1], sys.argv[2], sys.argv[3]

    parts = []
    for path, label, want_in, want_out in (
            (det, "detector", [1, 192, 192, 3], [[1, 2016, 18], [1, 2016, 1]]),
            (lm, "landmarks", [1, 224, 224, 3], [[1, 63], [1, 1], [1, 1], [1, 63]])):
        g = load(path)
        folded = fold_dequantize(g)
        got_in = g.tensors[g.inputs[0]]["shape"]
        got_out = [g.tensors[i]["shape"] for i in g.outputs]
        if got_in != want_in or got_out != want_out:
            raise SystemExit("%s: shapes are not the ones this was written for\n"
                             "  input  %s (want %s)\n  outputs %s (want %s)"
                             % (label, got_in, want_in, got_out, want_out))
        hist = collections.Counter(o["op"] for o in g.ops)
        print("%-10s %2d ops after folding %d dequantize nodes: %s"
              % (label, len(g.ops), folded,
                 ", ".join("%s x%d" % (k, v) for k, v in hist.most_common())))
        parts.append(emit(g, label))

    body = struct.pack("<4sII", b"HAND", VERSION, len(parts))
    body += b"".join(struct.pack("<I", len(p)) for p in parts)
    body += b"".join(parts)
    # gzip, which the loader detects by magic: float16 weights give back about a
    # quarter, and the cost is one inflate at startup.
    #
    # mtime=0, because gzip stamps the current time into its header and that one
    # field was the only thing standing between this script and a reproducible
    # output: two runs on the same .tflite differed in byte 5 and nowhere else.
    # A build that verifies the model by digest needs the conversion to be a
    # function of its input. gzip.compress grew an mtime argument in 3.8;
    # GzipFile has had one since forever, and this file targets neither.
    buf = io.BytesIO()
    with gzip.GzipFile(fileobj=buf, mode="wb", compresslevel=9, mtime=0) as gz:
        gz.write(body)
    packed = buf.getvalue()
    pathlib.Path(out).write_bytes(packed)
    print("wrote %s  %.2f MB (from %.2f MB)"
          % (out, len(packed) / 1e6, len(body) / 1e6))


if __name__ == "__main__":
    main()

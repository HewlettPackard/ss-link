# SPDX-License-Identifier: GPL-2.0
# Copyright 2026 Hewlett Packard Enterprise Development LP

# Converts the cable DB text table (TSV) to a firmware binary.
#
# Dependencies:
#   Python 3 standard library only
#
# Usage:
#   python3 <script> [infile] [outfile]
#   Defaults:
#     infile  = sl_cable_db.tsv
#     outfile = sl_cable_db.bin

import os
import re
import sys
import struct
from datetime import date

DEFAULT_INFILE  = "sl_cable_db.tsv"
DEFAULT_OUTFILE = "sl_cable_db.bin"

_REPO_ROOT   = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_FIRMWARE_H  = os.path.join(_REPO_ROOT,
    "drivers", "net", "ethernet", "hpe", "sl", "media", "data",
    "sl_media_cable_db_firmware.h")
_SL_MEDIA_H  = os.path.join(_REPO_ROOT,
    "include", "linux", "hpe", "sl", "sl_media.h")


def _load_c_consts(filepath):
    """Parse integer #define and enum constants from a C header file."""
    with open(filepath, encoding='utf-8') as f:
        text = f.read()
    text = re.sub(r'\\\s*\n\s*', ' ', text)   # join line continuations

    def _eval(expr, ns):
        s = re.sub(r'/\*.*?\*/', '', expr).strip()
        s = re.sub(r'\b(0[xX][0-9a-fA-F]+|[0-9]+)[UuLl]+\b', r'\1', s)
        s = re.sub(r'\bBIT\((\w+)\)', r'(1 << \1)', s)
        for n, v in sorted(ns.items(), key=lambda x: -len(x[0])):
            s = re.sub(r'\b' + re.escape(n) + r'\b', str(v), s)
        try:
            return int(eval(s))  # noqa: S307
        except Exception:
            return None

    ns = {}
    for m in re.finditer(r'^\s*#define\s+(\w+)(?!\()(\s+)([^\n]+)', text, re.MULTILINE):
        val = _eval(m.group(3), ns)
        if val is not None:
            ns[m.group(1)] = val
    for em in re.finditer(r'enum\s+\w+\s*\{([^}]+)\}', text, re.DOTALL):
        counter = 0
        for part in re.split(r',', em.group(1)):
            part = re.sub(r'/\*.*?\*/', '', part).strip()
            if not part:
                continue
            if '=' in part:
                name, val_str = part.split('=', 1)
                val = _eval(val_str, ns)
                if val is not None:
                    counter = val
                name = name.strip()
            else:
                name = part.strip()
            if re.match(r'^\w+$', name):
                ns[name] = counter
                counter += 1
    return ns


_fw = _load_c_consts(_FIRMWARE_H)
_sl = _load_c_consts(_SL_MEDIA_H)

CABLE_DB_MAGIC        = _fw["SL_MEDIA_CABLE_DB_MAGIC"]
CABLE_DB_HDR_VERSION  = _fw["SL_MEDIA_CABLE_DB_HDR_VERSION"]
CABLE_DB_DATA_VERSION = _fw["SL_MEDIA_CABLE_DB_DATA_VERSION"]
VENDOR_PN_SIZE        = _sl["SL_MEDIA_VENDOR_PN_SIZE"]

HDR_FMT  = "<IIIII"
HDR_SIZE = struct.calcsize(HDR_FMT)

_fixed_fmt  = "<IIIIIIhhhhhh4bB"
_fixed_size = struct.calcsize(_fixed_fmt)
_rec_size   = _fw["SL_MEDIA_CABLE_DB_RECORD_V1_SIZE"]
_pad        = _rec_size - _fixed_size - VENDOR_PN_SIZE
assert _pad >= 0, "SL_MEDIA_CABLE_DB_RECORD_V1_SIZE too small for fields"
RECORD_FMT  = f"{_fixed_fmt}{VENDOR_PN_SIZE}s{_pad}x"
RECORD_SIZE = struct.calcsize(RECORD_FMT)
assert RECORD_SIZE == _rec_size, f"record size mismatch ({RECORD_SIZE} != {_rec_size})"

VENDOR_MAP = {
    "TE":         _sl["SL_MEDIA_VENDOR_TE"],
    "Hisense":    _sl["SL_MEDIA_VENDOR_HISENSE"],
    "Finisar":    _sl["SL_MEDIA_VENDOR_FINISAR"],
    "Molex":      _sl["SL_MEDIA_VENDOR_MOLEX"],
    "Bizlink":    _sl["SL_MEDIA_VENDOR_BIZLINK"],
    "CloudLight": _sl["SL_MEDIA_VENDOR_CLOUD_LIGHT"],
}

TYPE_MAP = {
    "PEC": _sl["SL_MEDIA_TYPE_PEC"],
    "AOC": _sl["SL_MEDIA_TYPE_AOC"],
    "POC": _sl["SL_MEDIA_TYPE_POC"],
    "AEC": _sl["SL_MEDIA_TYPE_AEC"],
}

SHAPE_MAP = {
    "Straight":   _sl["SL_MEDIA_SHAPE_STRAIGHT"],
    "Splitter":   _sl["SL_MEDIA_SHAPE_SPLITTER"],
    "Bifurcated": _sl["SL_MEDIA_SHAPE_BIFURCATED"],
}

SPEED_MAP = {
    "200G": _sl["SL_MEDIA_SPEEDS_SUPPORT_CK_200G"],
    "400G": _sl["SL_MEDIA_SPEEDS_SUPPORT_CK_400G"],
    "800G": _sl["SL_MEDIA_SPEEDS_SUPPORT_CK_800G"],
}


def parse_record(fields, lineno):
    if len(fields) != 18:
        print(f"ERROR: line {lineno}: expected 18 fields, got {len(fields)}",
              file=sys.stderr)
        sys.exit(1)

    (hpe_pn_s, vendor_s, vendor_pn_s, type_s, shape_s, length_cm_s,
     is_ss200_s, max_speed_s,
     pre1_s, pre2_s, pre3_s, cursor_s, post1_s, post2_s,
     fw_major_s, fw_minor_s, fw_split_major_s, fw_split_minor_s) = fields

    if vendor_s not in VENDOR_MAP:
        print(f"ERROR: line {lineno}: unknown vendor '{vendor_s}'", file=sys.stderr)
        sys.exit(1)
    if type_s not in TYPE_MAP:
        print(f"ERROR: line {lineno}: unknown type '{type_s}'", file=sys.stderr)
        sys.exit(1)
    if shape_s not in SHAPE_MAP:
        print(f"ERROR: line {lineno}: unknown shape '{shape_s}'", file=sys.stderr)
        sys.exit(1)
    if max_speed_s not in SPEED_MAP:
        print(f"ERROR: line {lineno}: unknown speed '{max_speed_s}'", file=sys.stderr)
        sys.exit(1)

    vendor_pn_bytes = vendor_pn_s.encode("ascii", errors="replace")[:VENDOR_PN_SIZE - 1]
    vendor_pn_bytes = vendor_pn_bytes.ljust(VENDOR_PN_SIZE, b'\x00')

    record = struct.pack(
        RECORD_FMT,
        int(hpe_pn_s),
        VENDOR_MAP[vendor_s],
        TYPE_MAP[type_s],
        SHAPE_MAP[shape_s],
        int(length_cm_s),
        SPEED_MAP[max_speed_s],
        int(pre1_s),
        int(pre2_s),
        int(pre3_s),
        int(cursor_s),
        int(post1_s),
        int(post2_s),
        int(fw_major_s),
        int(fw_minor_s),
        int(fw_split_major_s),
        int(fw_split_minor_s),
        int(is_ss200_s),
        vendor_pn_bytes,
    )
    assert len(record) == RECORD_SIZE
    return record


def main():
    infile  = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_INFILE
    outfile = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_OUTFILE

    print(f"Input  : {infile}")
    print(f"Output : {outfile}")

    records = []
    with open(infile, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            fields = line.split("\t")
            records.append(parse_record(fields, lineno))

    total  = len(records)
    header = struct.pack(HDR_FMT, CABLE_DB_MAGIC, CABLE_DB_HDR_VERSION,
                         CABLE_DB_DATA_VERSION, total, date.today().toordinal())

    with open(outfile, "wb") as f:
        f.write(header)
        for rec in records:
            f.write(rec)

    file_size = HDR_SIZE + total * RECORD_SIZE
    print(f"Written {file_size} bytes to {outfile}")
    print(f"  hdr_version={CABLE_DB_HDR_VERSION} data_version={CABLE_DB_DATA_VERSION} records={total} record_size={RECORD_SIZE}")


if __name__ == "__main__":
    main()

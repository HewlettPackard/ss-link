# SPDX-License-Identifier: GPL-2.0
# Copyright 2023-2026 Hewlett Packard Enterprise Development LP

# Reads the cable compatibility spreadsheet and writes a tab-separated text
# table (TSV) that is checked into the repository as the cable DB source.
#
# Dependencies:
#   sudo apt install python3-pip
#   sudo pip install openpyxl
#
# Usage:
#   python3 <script> [infile] [outfile]
#   Defaults:
#     infile  = cable compatibility spreadsheet (.xlsm)
#     outfile = <repo>/drivers/net/ethernet/hpe/sl/media/data/sl_cable_db.tsv

import re
import sys
import os
from openpyxl import load_workbook

DEFAULT_INFILE  = "SlingshotCableCompatibilityMatrix.xlsm"
DEFAULT_OUTFILE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               "../drivers/net/ethernet/hpe/sl/media/data/sl_cable_db.tsv")

_FIRMWARE_H = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "../drivers/net/ethernet/hpe/sl/media/data/sl_media_cable_db_firmware.h")

def _parse_define_int(filepath, name):
    with open(filepath, encoding='utf-8') as f:
        text = f.read()
    m = re.search(r'#define\s+' + re.escape(name) + r'\s+(\d+)', text)
    return int(m.group(1)) if m else None

CABLE_DB_DATA_VERSION = _parse_define_int(_FIRMWARE_H, "SL_MEDIA_CABLE_DB_DATA_VERSION")

SOURCE1 = "S1 S2 QSFP Cable List"
SOURCE2 = "S2 OSFP Cable List"

CABLE_KIT_DESCRIPTIONS = {
    "HPE Slingshot L1 1x16 Sw Cbl Kit Cray EX",
    "HPE Slingshot L1 2x16 Sw Cbl Kit Cray EX",
    "HPE Slingshot L1 1x32 Sw Cbl Kit Cray EX",
}

VENDOR_MAP = {
    "TE":                       "TE",
    "Hisense":                  "Hisense",
    "Bizlink":                  "Bizlink",
    "BizLink":                  "Bizlink",
    "Coherent (Finisar II-VI)": "Finisar",
    "Cloud Light":              "CloudLight",
    "Molex":                    "Molex",
}

TYPE_MAP = {
    "DAC":   "PEC",
    "PEC":   "PEC",
    "AOC":   "AOC",
    "AOC-A": "AOC",
    "AOC-D": "AOC",
    "POF":   "POC",
    "AEC":   "AEC",
    "XCVR":  "POC",
}

SHAPE_MAP = {
    "Straight":       "Straight",
    "Splitter (Y)":   "Splitter",
    "Bifurcated (H)": "Bifurcated",
}

SPEED_MAP = {
    "200G E": "200G",
    "200Gb":  "200G",
    "400G E": "400G",
    "400Gb":  "400G",
    "800Gb":  "800G",
}

SERDES_BY_TYPE_STR = {
    "DAC":  (0,   0, 0, 100,  0, 0),
    "PEC":  (0,   0, 0, 100,  0, 0),
    "AEC":  (-4,  0, 0,  98,  0, 0),
    "XCVR": (-4,  0, 0,  98,  0, 0),
    "AOC":  (-12, 0, 0,  98, -4, 0),
}
SERDES_DEFAULT = (-20, 0, 0, 116, 0, 0)


def rows_count(sheet):
    curr_row = 1
    counter  = 1
    while True:
        if str(sheet.cell(row=curr_row + 1, column=1).value) == "None":
            break
        counter  += 1
        curr_row += 1
    return counter


def part_nums_get(sheet, count, part_nums):
    for row in range(1, count):
        if str(sheet.cell(row=row + 1, column=10).value) in CABLE_KIT_DESCRIPTIONS:
            continue
        vendor = str(sheet.cell(row=row + 1, column=7).value).strip()
        if vendor == "?":
            continue
        alpha_pn = str(sheet.cell(row=row + 1, column=1).value).strip()
        if alpha_pn == "?":
            continue
        pn_str = re.sub(r'[^0-9]', '', alpha_pn)
        part_nums.append(int(pn_str))


def _parse_fw_ver(cell_value):
    """Return a signed int8 value for a firmware version cell (-1 if N/A)."""
    v = str(cell_value).strip()
    if v in ("None", "N/A", ""):
        return -1
    return int(v, 16)


def cable_rows_collect(sheet, part_nums, rows):
    """
    For each PN in part_nums (sorted), locate its row in the sheet, extract
    all fields, and append a tuple to rows.  Deletes matched rows to prevent
    duplicate matches.
    """
    for pn in part_nums:
        curr_row = 1
        while True:
            length_cell = str(sheet.cell(row=curr_row + 1, column=10).value)
            if length_cell in CABLE_KIT_DESCRIPTIONS:
                curr_row += 1
                continue

            vendor_str = str(sheet.cell(row=curr_row + 1, column=7).value).strip()
            if vendor_str == "?":
                curr_row += 1
                continue

            alpha_pn = str(sheet.cell(row=curr_row + 1, column=1).value).strip()
            if alpha_pn == "?":
                curr_row += 1
                continue

            row_pn = int(re.sub(r'[^0-9]', '',
                                str(sheet.cell(row=curr_row + 1, column=1).value)))
            if row_pn != pn:
                curr_row += 1
                continue

            # --- vendor ---
            if vendor_str not in VENDOR_MAP:
                print(f"ERROR: unknown vendor '{vendor_str}' at row {curr_row + 1}",
                      file=sys.stderr)
                sys.exit(1)
            vendor = VENDOR_MAP[vendor_str]

            # --- vendor PN string (col 8) ---
            vendor_pn = str(sheet.cell(row=curr_row + 1, column=8).value).replace(" ", "")

            # --- type (col 6) ---
            type_str = str(sheet.cell(row=curr_row + 1, column=6).value).strip()
            if type_str not in TYPE_MAP:
                print(f"ERROR: unknown type '{type_str}' at row {curr_row + 1}",
                      file=sys.stderr)
                sys.exit(1)
            media_type = TYPE_MAP[type_str]

            # --- shape (col 5) ---
            shape_str = str(sheet.cell(row=curr_row + 1, column=5).value).strip()
            if shape_str not in SHAPE_MAP:
                print(f"ERROR: unknown shape '{shape_str}' at row {curr_row + 1}",
                      file=sys.stderr)
                sys.exit(1)
            shape = SHAPE_MAP[shape_str]

            # --- length (col 10) ---
            if type_str == "XCVR":
                length_cm = 0
            else:
                whitelist = set('0123456789.')
                num_str   = ''.join(filter(whitelist.__contains__, length_cell))
                length_cm = int(float(num_str) * 100)

            # --- SS200 support (col 4) ---
            ss200_str = str(sheet.cell(row=curr_row + 1, column=4).value).strip()
            is_ss200  = 1 if ss200_str == "SS200" else 0

            # --- max speed (col 11) ---
            speed_str = str(sheet.cell(row=curr_row + 1, column=11).value).strip()
            if speed_str not in SPEED_MAP:
                print(f"ERROR: unknown speed '{speed_str}' at row {curr_row + 1}",
                      file=sys.stderr)
                sys.exit(1)
            max_speed = SPEED_MAP[speed_str]

            # --- SerDes settings ---
            (pre1, pre2, pre3, cursor, post1, post2) = SERDES_BY_TYPE_STR.get(
                type_str, SERDES_DEFAULT)

            # --- firmware versions (cols 17-20) ---
            fw_major       = _parse_fw_ver(sheet.cell(row=curr_row + 1, column=17).value)
            fw_minor       = _parse_fw_ver(sheet.cell(row=curr_row + 1, column=18).value)
            fw_split_major = _parse_fw_ver(sheet.cell(row=curr_row + 1, column=19).value)
            fw_split_minor = _parse_fw_ver(sheet.cell(row=curr_row + 1, column=20).value)

            rows.append((pn, vendor, vendor_pn, media_type, shape, length_cm, is_ss200,
                         max_speed, pre1, pre2, pre3, cursor, post1, post2,
                         fw_major, fw_minor, fw_split_major, fw_split_minor))

            sheet.delete_rows(curr_row + 1, 1)
            break


def write_tsv(outfile, rows):
    with open(outfile, "w", encoding="utf-8") as f:
        f.write("# SPDX-License-Identifier: GPL-2.0\n")
        f.write("# Copyright 2026 Hewlett Packard Enterprise Development LP\n")
        f.write("#\n")
        f.write("# Generated Slingshot cable compatibility database\n")
        f.write("# Do not edit manually!\n")
        f.write("#\n")
        for row in rows:
            f.write("\t".join(str(v) for v in row) + "\n")


def main():
    infile  = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_INFILE
    outfile = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_OUTFILE

    print(f"Input  : {infile}")
    print(f"Output : {outfile}")

    wb = load_workbook(infile)

    print(f"Convert: {SOURCE1}")
    sheet1 = wb[SOURCE1]
    pns1   = []
    part_nums_get(sheet1, rows_count(sheet1), pns1)
    pns1.sort()
    print(f"  Valid QSFP cables: {len(pns1)}")

    print(f"Convert: {SOURCE2}")
    sheet2 = wb[SOURCE2]
    pns2   = []
    part_nums_get(sheet2, rows_count(sheet2), pns2)
    pns2.sort()
    print(f"  Valid S2 OSFP cables: {len(pns2)}")

    # Reload for clean record collection (delete_rows mutates the worksheet)
    wb2     = load_workbook(infile)
    sheet1r = wb2[SOURCE1]
    sheet2r = wb2[SOURCE2]

    rows = []
    cable_rows_collect(sheet1r, pns1, rows)
    cable_rows_collect(sheet2r, pns2, rows)

    write_tsv(outfile, rows)

    print(f"Written {len(rows)} entries to {outfile}")


if __name__ == "__main__":
    main()

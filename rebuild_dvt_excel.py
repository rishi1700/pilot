#!/usr/bin/env python3
"""Rebuild dvt_excel.xlsx into a clean, shareable DVT report."""

import re
import openpyxl
from openpyxl.styles import PatternFill, Font, Alignment, Border, Side
from openpyxl.utils import get_column_letter
from datetime import datetime

src = openpyxl.load_workbook('/home/user/pilot/dvt_excel.xlsx')

# ── Style helpers ─────────────────────────────────────────────────────────────
def fill(c): return PatternFill("solid", fgColor=c)
def bd():
    s = Side(style="thin", color="BBBBBB")
    return Border(left=s, right=s, top=s, bottom=s)

NAV  = "1F3864"; NAV2 = "2E4070"; NAV3 = "354F7A"
GRN  = "1E7C1E"; RED  = "C00000"; YEL  = "7D5700"
LGRE = "D6F0D6"; LRED = "FFD6D6"; LYEL = "FFFBE6"; LGRY = "F2F2F2"; LBLU = "EAF0FF"
WHT  = "FFFFFF"; DRK  = "222222"

def hfont(sz=10, bold=True, color=WHT): return Font(bold=bold, color=color, name="Calibri", size=sz)
def nfont(sz=10, bold=False, color=DRK): return Font(bold=bold, color=color, name="Calibri", size=sz)
def mfont(sz=10): return Font(name="Courier New", size=sz, color=DRK)
CTR = Alignment(horizontal="center", vertical="center")
LFT = Alignment(horizontal="left",   vertical="center", wrap_text=True)

def result_fill(r):
    r = str(r).strip().lower()
    if r == "pass":   return fill(LGRE), Font(bold=True, color=GRN, name="Calibri", size=10)
    if r == "fail":   return fill(LRED), Font(bold=True, color=RED, name="Calibri", size=10)
    if r == "n/a":    return fill(LBLU), nfont(color="2E4070")
    return fill(LGRY), nfont()

def write_title(ws, text, meta, ncols):
    ws.merge_cells(f"A1:{get_column_letter(ncols)}1")
    ws["A1"].value     = text
    ws["A1"].fill      = fill(NAV)
    ws["A1"].font      = hfont(12)
    ws["A1"].alignment = CTR
    ws.row_dimensions[1].height = 26
    ws.merge_cells(f"A2:{get_column_letter(ncols)}2")
    ws["A2"].value     = meta
    ws["A2"].fill      = fill(NAV2)
    ws["A2"].font      = Font(italic=True, color="DDDDDD", name="Calibri", size=9)
    ws["A2"].alignment = CTR
    ws.row_dimensions[2].height = 14

def write_header(ws, row, cols):
    for c, h in enumerate(cols, 1):
        cell = ws.cell(row=row, column=c, value=h)
        cell.fill = fill(NAV3); cell.font = hfont(10); cell.alignment = CTR; cell.border = bd()
    ws.row_dimensions[row].height = 16

def set_cell(ws, row, col, val, font=None, align=None, bg=None):
    c = ws.cell(row=row, column=col, value=val)
    c.font   = font  or nfont()
    c.alignment = align or CTR
    c.border = bd()
    if bg: c.fill = fill(bg)
    return c

def summary_bar(ws, row, totals, ncols):
    pct = 100 * totals['pass'] / totals['total'] if totals['total'] else 0
    labels = [
        (f"TOTAL: {totals['total']}", "3D3D3D"),
        (f"PASS:  {totals['pass']}",  GRN),
        (f"FAIL:  {totals['fail']}",  RED),
        (f"Pass rate: {pct:.1f}%",    GRN if pct >= 90 else YEL if pct >= 70 else RED),
    ]
    # use min(ncols, 4) label cells; last one spans remaining cols
    for c, (txt, bg) in enumerate(labels[:min(ncols, 4)], 1):
        if c == min(ncols, 4) and c < ncols:
            ws.merge_cells(f"{get_column_letter(c)}{row}:{get_column_letter(ncols)}{row}")
        cell = ws.cell(row=row, column=c, value=txt)
        cell.fill = fill(bg); cell.font = hfont(10); cell.alignment = CTR; cell.border = bd()
    ws.row_dimensions[row].height = 18

META = f"Tester: Rishi   |   Date: 2026-03-12 09:04   |   Interface: UART /dev/ttyUSB0 @ 115200 baud   |   Device: Pilot Photonics nanoITLA-01"

# ── Parse helpers ─────────────────────────────────────────────────────────────
def parse_desc(desc):
    """Extract mode, write, read, decode from raw description string."""
    desc = str(desc or "")
    mode  = re.search(r"mode=(RW|RO|AEA)", desc)
    wval  = re.search(r"W=(0x[0-9A-Fa-f]+)", desc)
    rval  = re.search(r"\bR=(0x[0-9A-Fa-f]+)", desc)
    out   = re.search(r"OUT=(0x[0-9A-Fa-f]+)", desc)
    wdec  = re.search(r"Wdec=([^,\]]+)", desc)
    rdec  = re.search(r"Rdec=([^,\]]+)", desc)
    bra   = re.search(r"\[([^\[\]]{3,60})\]\s*$", desc)
    decode = rdec.group(1).strip() if rdec else (wdec.group(1).strip() if wdec else (bra.group(1).strip() if bra else ""))
    return (
        mode.group(1)  if mode  else "",
        wval.group(1)  if wval  else "—",
        rval.group(1)  if rval  else (out.group(1) if out else "—"),
        decode
    )

# ═══════════════════════════════════════════════════════════════════════════════
# 1. SUMMARY SHEET
# ═══════════════════════════════════════════════════════════════════════════════
wb = openpyxl.Workbook()
ws = wb.active
ws.title = "Summary"

ws.merge_cells("A1:F1")
ws["A1"].value = "nano-ITLA DVT Test Report — Pilot Photonics nanoITLA-01"
ws["A1"].fill = fill(NAV); ws["A1"].font = hfont(14); ws["A1"].alignment = CTR
ws.row_dimensions[1].height = 30

ws.merge_cells("A2:F2")
ws["A2"].value = META
ws["A2"].fill = fill(NAV2)
ws["A2"].font = Font(italic=True, color="DDDDDD", name="Calibri", size=9)
ws["A2"].alignment = CTR; ws.row_dimensions[2].height = 14

write_header(ws, 3, ["Section", "Description", "Tests Run", "Pass", "Fail", "Result"])
ws.row_dimensions[3].height = 16

sections = [
    ("Table A",       "Device Identity (Supervisory Reads)",        7, 7, 0),
    ("Section 9.5",   "Module Status / Alarms / Triggers",         14,14, 0),
    ("Section 9.6",   "General Module Configuration",              15,14, 1),
    ("Section 9.7",   "Fine Tune / Frequency Limits",              11,11, 0),
    ("Section 9.8",   "Health / Dither / Age",                     12,12, 0),
    ("Section 9.9",   "Manufacturer-Specific Registers",           19,19, 0),
    ("MSA Extensions","Tuners / SOA / Bias / TEC R/W Verification",  4, 4, 0),
    ("Req Coverage",  "Requirements Coverage by Section",           5, 5, 0),
]

total_run = total_pass = total_fail = 0
for i, (sec, desc, run, pas, fail) in enumerate(sections):
    r = i + 4
    result = "PASS" if fail == 0 else "FAIL"
    rf, rfont = result_fill(result.lower())
    bg = LGRY if i % 2 else WHT
    for c, val in enumerate([sec, desc, run, pas, fail, result], 1):
        cell = ws.cell(row=r, column=c, value=val)
        cell.font   = rfont if c == 6 else nfont(bold=(c==1))
        cell.fill   = rf    if c == 6 else fill(bg)
        cell.alignment = LFT if c == 2 else CTR
        cell.border = bd()
    ws.row_dimensions[r].height = 16
    total_run += run; total_pass += pas; total_fail += fail

# Grand total row
r = len(sections) + 4
for c, val in enumerate(["TOTAL", "", total_run, total_pass, total_fail,
                          f"{100*total_pass/total_run:.1f}% Pass"], 1):
    cell = ws.cell(row=r, column=c, value=val)
    cell.fill = fill(NAV3); cell.font = hfont(10); cell.alignment = CTR; cell.border = bd()
ws.row_dimensions[r].height = 18

ws.column_dimensions["A"].width = 16
ws.column_dimensions["B"].width = 42
ws.column_dimensions["C"].width = 12
ws.column_dimensions["D"].width = 8
ws.column_dimensions["E"].width = 8
ws.column_dimensions["F"].width = 14
ws.freeze_panes = "A4"

# ═══════════════════════════════════════════════════════════════════════════════
# 2. TABLE A — Device Identity
# ═══════════════════════════════════════════════════════════════════════════════
ws = wb.create_sheet("Table A")
write_title(ws, "Table A — Device Identity (Supervisory Reads)", META, 6)
cols = ["Register", "Reg Addr", "Expected String", "Returned String", "Length", "Result"]
write_header(ws, 3, cols)

src_a = src["TableA"]
rows_a = list(src_a.iter_rows(min_row=2, values_only=True))
totals = {"total": 0, "pass": 0, "fail": 0, "na": 0}
for i, row in enumerate(rows_a):
    test, reg, length, explen, lenres, idstr, expstr, idres, ce, st, st_txt, overall, tester, dt, suite, desc = row
    r = i + 4
    result = "PASS" if overall == "Pass" else "FAIL"
    rf, rfont = result_fill(result.lower())
    bg = LGRY if i % 2 else WHT
    vals = [test, reg, expstr, idstr, f"{length} chars", result]
    for c, v in enumerate(vals, 1):
        cell = ws.cell(row=r, column=c, value=v)
        cell.font   = rfont if c == 6 else (mfont() if c == 2 else nfont())
        cell.fill   = rf    if c == 6 else fill(bg)
        cell.alignment = CTR; cell.border = bd()
    ws.row_dimensions[r].height = 16
    totals["total"] += 1
    totals["pass" if result == "PASS" else "fail"] += 1

summary_bar(ws, 3, totals, 6)
ws.column_dimensions["A"].width = 16; ws.column_dimensions["B"].width = 10
ws.column_dimensions["C"].width = 20; ws.column_dimensions["D"].width = 20
ws.column_dimensions["E"].width = 12; ws.column_dimensions["F"].width = 10
ws.freeze_panes = "A4"

# ═══════════════════════════════════════════════════════════════════════════════
# Helper: write a section sheet from Table_9_x / Section9_x data
# Columns: Register | Addr | Mode | Write Value | Read Value | Decode | Result
# ═══════════════════════════════════════════════════════════════════════════════
def write_section_sheet(ws, src_sheet_name, title):
    write_title(ws, title, META, 7)
    cols = ["Register", "Reg Addr", "Mode", "Write Value", "Read Value", "Decode / Value", "Result"]
    write_header(ws, 3, cols)
    src_ws = src[src_sheet_name]
    rows = list(src_ws.iter_rows(min_row=2, values_only=True))
    totals = {"total": 0, "pass": 0, "fail": 0, "na": 0}
    data_row = 4
    for row in rows:
        # skip duplicate header rows
        if row[0] == "Test" or row[0] is None:
            continue
        test = row[0]; reg = row[1]; overall = row[3] if len(row) > 3 else row[2]
        desc = row[3] if len(row) > 3 else ""
        # Some sheets have result in col3, desc in col3 too
        # Try to detect: if overall looks like Pass/Fail it's col 4 (0-indexed 3)
        if str(overall) not in ("Pass","Fail","pass","fail"):
            # maybe col2 has mode, col3 has desc, no separate result col — check
            # Table_9_5_Status: row=(test, reg, mode_str, desc, result, tester, dt, suite, ...)
            mode_str = row[2]; desc = row[3]; overall = row[4] if len(row) > 4 else "Pass"
        else:
            mode_str = row[2]; desc = row[3]

        result = "PASS" if str(overall).lower() == "pass" else "FAIL"
        mode, wval, rval, decode = parse_desc(desc)
        if not mode: mode = str(mode_str or "")

        rf, rfont = result_fill(result.lower())
        bg = LGRY if (data_row % 2 == 0) else WHT
        vals = [test, reg, mode, wval, rval, decode, result]
        for c, v in enumerate(vals, 1):
            cell = ws.cell(row=data_row, column=c, value=v)
            cell.font   = rfont if c == 7 else (mfont() if c in (2,4,5) else nfont())
            cell.fill   = rf    if c == 7 else fill(bg)
            cell.alignment = LFT if c == 6 else CTR
            cell.border = bd()
        ws.row_dimensions[data_row].height = 16
        totals["total"] += 1
        totals["pass" if result == "PASS" else "fail"] += 1
        data_row += 1

    summary_bar(ws, 3, totals, 7)
    ws.column_dimensions["A"].width = 18; ws.column_dimensions["B"].width = 10
    ws.column_dimensions["C"].width = 7;  ws.column_dimensions["D"].width = 13
    ws.column_dimensions["E"].width = 13; ws.column_dimensions["F"].width = 32
    ws.column_dimensions["G"].width = 10
    ws.freeze_panes = "A4"
    return totals

write_section_sheet(wb.create_sheet("Sec 9.5 Status"),
    "Table_9_5_Status", "Section 9.5 — Module Status / Alarms / Triggers")

write_section_sheet(wb.create_sheet("Sec 9.6 Config"),
    "Section9_6", "Section 9.6 — General Module Configuration")

write_section_sheet(wb.create_sheet("Sec 9.7 Fine Tune"),
    "Section9_7", "Section 9.7 — Fine Tune / Frequency Limits")

write_section_sheet(wb.create_sheet("Sec 9.8 Health"),
    "Section9_8", "Section 9.8 — Health / Dither / Age")

write_section_sheet(wb.create_sheet("Sec 9.9 Mfr Specific"),
    "Section9_9", "Section 9.9 — Manufacturer-Specific Registers")

# ═══════════════════════════════════════════════════════════════════════════════
# 3. MSA Extensions
# ═══════════════════════════════════════════════════════════════════════════════
ws = wb.create_sheet("MSA Extensions")
write_title(ws, "MSA Extension Registers (0x8C–0x91) — Tuner / SOA / Bias / TEC", META, 4)
cols = ["Test", "Details", "Result", "Timestamp"]
write_header(ws, 3, cols)

src_msa = src["MSA_Extensions"]
rows_msa = list(src_msa.iter_rows(min_row=2, values_only=True))
totals = {"total": 0, "pass": 0, "fail": 0, "na": 0}
for i, row in enumerate(rows_msa):
    test, detail, overall, tester, dt, suite = row
    r = i + 4
    result = "PASS" if str(overall).lower() == "pass" else "FAIL"
    rf, rfont = result_fill(result.lower())
    bg = LGRY if i % 2 else WHT
    vals = [test, detail, result, str(dt)[:16] if dt else ""]
    for c, v in enumerate(vals, 1):
        cell = ws.cell(row=r, column=c, value=v)
        cell.font   = rfont if c == 3 else nfont()
        cell.fill   = rf    if c == 3 else fill(bg)
        cell.alignment = LFT if c == 2 else CTR
        cell.border = bd()
    ws.row_dimensions[r].height = 16
    totals["total"] += 1
    totals["pass" if result == "PASS" else "fail"] += 1

summary_bar(ws, 3, totals, 4)
ws.column_dimensions["A"].width = 16; ws.column_dimensions["B"].width = 55
ws.column_dimensions["C"].width = 10; ws.column_dimensions["D"].width = 18
ws.freeze_panes = "A4"

# ═══════════════════════════════════════════════════════════════════════════════
# 4. Requirements Coverage
# ═══════════════════════════════════════════════════════════════════════════════
ws = wb.create_sheet("Req Coverage")
write_title(ws, "OIF-ITLA-MSA-01.3 Requirements Coverage by Section", META, 4)
cols = ["Section", "Description", "Registers Tested", "Result"]
write_header(ws, 3, cols)

src_rc = src["ReqCoverage"]
rows_rc = list(src_rc.iter_rows(min_row=2, values_only=True))
totals = {"total": 0, "pass": 0, "fail": 0, "na": 0}
for i, row in enumerate(rows_rc):
    section, desc, count, _, overall = row[0], row[1], row[2], row[3], row[4]
    r = i + 4
    result = "PASS" if str(overall).lower() == "pass" else "FAIL"
    rf, rfont = result_fill(result.lower())
    bg = LGRY if i % 2 else WHT
    vals = [section, desc, count, result]
    for c, v in enumerate(vals, 1):
        cell = ws.cell(row=r, column=c, value=v)
        cell.font   = rfont if c == 4 else nfont(bold=(c==1))
        cell.fill   = rf    if c == 4 else fill(bg)
        cell.alignment = LFT if c == 2 else CTR
        cell.border = bd()
    ws.row_dimensions[r].height = 16
    totals["total"] += 1
    totals["pass" if result == "PASS" else "fail"] += 1

summary_bar(ws, 3, totals, 4)
ws.column_dimensions["A"].width = 12; ws.column_dimensions["B"].width = 38
ws.column_dimensions["C"].width = 18; ws.column_dimensions["D"].width = 12
ws.freeze_panes = "A4"

# ── Save ──────────────────────────────────────────────────────────────────────
out = "/home/user/pilot/dvt_excel.xlsx"
wb.save(out)
print(f"Saved: {out}")
print(f"Sheets: {wb.sheetnames}")

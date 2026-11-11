#!/usr/bin/env python3
"""
Generate <config_dir>/io_var_id.h from config JSON files.

Usage: python gen_io_var_id.py [config_dir]     默认 config
"""

import json
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
CONFIG_DIR = (Path(sys.argv[1]).resolve() if len(sys.argv) > 1
              else SCRIPT_DIR / "config")
OUTPUT_FILE = CONFIG_DIR / "io_var_id.h"

ACS_CONFIG = CONFIG_DIR / "acs" / "io_config.json"
PLC_CONFIG = CONFIG_DIR / "plc" / "io_config.json"

SEG_COMMENTS = {
    0:  "PLC AI",
    1:  "ACS AI",
    2:  "PLC DI",
    3:  "ACS DI",
    4:  "PLC AO",
    5:  "ACS AO",
    6:  "PLC DO",
    7:  "ACS DO",
    8:  "SettingDouble / SettingBool",
    9:  "Buffer",
    10: "Cmd",
}


def hw_label(seg):
    return "PLC" if seg % 2 == 0 else "ACS"


def sanitize(name):
    s = name.strip()
    s = re.sub(r"[^a-zA-Z0-9_\.]", "_", s)
    s = s.replace(".", "_")
    s = re.sub(r"_+", "_", s)
    s = s.strip("_")
    return s


def collect():
    """Return list of (seg, enum_name, id, comment)。comment 来自
    io_config.json 各条目的 "comment" 字段（点位表"说明"列原文），
    只用于生成枚举行尾注释，不参与协议。"""
    result = []

    def cmt(e):
        return " ".join(str(e.get("comment") or "").split())

    acs = json.loads(ACS_CONFIG.read_text(encoding="utf-8"))
    for cat in ["AI", "DI", "AO", "DO"]:
        for e in acs.get(cat, []):
            vid = e["id"]
            seg = vid // 1000
            hw = hw_label(seg)
            name = sanitize(e["name"])
            result.append((seg, f"{cat}_{hw}_{name}", vid, cmt(e)))

    for e in acs.get("Buffer", []):
        vid = e["id"]
        result.append((vid // 1000, f"BUF_{e['bufferId']}", vid, cmt(e)))

    plc = json.loads(PLC_CONFIG.read_text(encoding="utf-8"))
    for mp in plc.get("machineParts", []):
        for cat, prefix in [("AI", "AI"), ("DI", "DI"), ("AO", "AO"),
                             ("DO", "DO"), ("Cmd", "CMD")]:
            for e in mp.get(cat, []):
                vid = e["id"]
                seg = vid // 1000
                hw = hw_label(seg)
                display_name = e.get("label") or e["name"]
                name = sanitize(display_name)
                result.append((seg, f"{prefix}_{hw}_{name}", vid, cmt(e)))

        for e in mp.get("SettingDouble", []):
            vid = e["id"]
            display_name = e.get("label") or e["name"]
            name = sanitize(display_name)
            result.append((vid // 1000, f"SETDBL_PLC_{name}", vid, cmt(e)))

        for e in mp.get("SettingBool", []):
            vid = e["id"]
            display_name = e.get("label") or e["name"]
            name = sanitize(display_name)
            result.append((vid // 1000, f"SETBOOL_PLC_{name}", vid, cmt(e)))

    return result


def generate():
    entries = collect()

    # 枚举成员名重复 = C++ 重定义(编译期 C2371)。在这里先报出来并列出冲突 id,
    # 免得只看到一句难懂的编译器错误。修法:给其中一条加同类内唯一的 label。
    seen = {}
    dups = []
    for _seg, name, vid, _cmt in entries:
        if name in seen:
            dups.append((name, seen[name], vid))
        else:
            seen[name] = vid
    if dups:
        for name, first_id, dup_id in dups:
            print("[ERROR] duplicate enum member %r (id %d 与 id %d 撞名) "
                  "- 给其中一条补唯一的 label" % (name, first_id, dup_id),
                  file=sys.stderr)
        raise SystemExit(1)

    lines = []
    lines.append("#ifndef __IO_VAR_ID_H__")
    lines.append("#define __IO_VAR_ID_H__")
    lines.append("")
    lines.append("// ===================================================================")
    lines.append("// IO 变量 ID 枚举 — 由 gen_io_var_id.py 自动生成，请勿手动修改")
    lines.append("//")
    lines.append("// 号段划分: id / 1000 确定类型")
    for seg, desc in sorted(SEG_COMMENTS.items()):
        lines.append(f"//   {seg}: {desc}")
    lines.append("//")
    lines.append("// 此文件随协议一同提交给上层，上层可自行改名或 include。")
    lines.append("// ===================================================================")
    lines.append("")

    def emit(seg, prefix=None):
        items = [e for e in entries if e[0] == seg]
        if prefix:
            items = [e for e in items if e[1].startswith(prefix)]
        desc = SEG_COMMENTS.get(seg, str(seg))
        rng = f"{seg*1000}..{seg*1000+999}"
        lines.append(f"    // {desc}  ({rng})")
        rows = sorted(items, key=lambda x: x[2])
        # 注释列对齐：先算 "<枚举名> = <id>," 的最宽长度
        width = max([len(f"{name} = {vid},") for _, name, vid, _ in rows] or [0])
        for _, name, vid, comment in rows:
            line = f"    {name} = {vid},"
            if comment:
                line = line.ljust(4 + width + 1) + "// " + comment
            lines.append(line.rstrip())

    lines.append("enum AiVarId {")
    emit(0)
    emit(1)
    lines.append("};")
    lines.append("")

    lines.append("enum DiVarId {")
    emit(2)
    emit(3)
    lines.append("};")
    lines.append("")

    lines.append("enum AoVarId {")
    emit(4)
    emit(5)
    lines.append("};")
    lines.append("")

    lines.append("enum DoVarId {")
    emit(6)
    emit(7)
    lines.append("};")
    lines.append("")

    lines.append("enum SettingDoubleVarId {")
    emit(8, "SETDBL_")
    lines.append("};")
    lines.append("")

    lines.append("enum SettingBoolVarId {")
    emit(8, "SETBOOL_")
    lines.append("};")
    lines.append("")

    lines.append("enum BufferVarId {")
    emit(9)
    lines.append("};")
    lines.append("")

    lines.append("enum CmdVarId {")
    emit(10)
    lines.append("};")
    lines.append("")

    lines.append("#endif // __IO_VAR_ID_H__")
    lines.append("")

    content = "\n".join(lines) + "\n"
    OUTPUT_FILE.write_text(content, encoding="utf-8")
    print(f"Generated: {OUTPUT_FILE}")
    print(f"  {len(entries)} entries")


if __name__ == "__main__":
    generate()

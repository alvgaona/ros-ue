# Rewrites ROS 2 message IDL for idlc under the names ROS 2 uses on the wire (std_msgs::msg::dds_::String_).
# Usage: idl.py <share dir> <out dir> <package>...
import json
import re
import sys
from pathlib import Path

share, out, *packages = sys.argv[1:]
for package in packages:
    sources = list(Path(share, package, "msg").glob("*.idl"))
    if not sources:
        sys.exit(f"idl.py: no {package}/msg/*.idl in {share}, add the package to pixi.toml")
    for src in sources:
        idl = src.read_text()
        # @verbatim, @default and @unit carry nothing on the wire, and idlc warns on each
        idl = re.sub(r'@\w+\s*\((?:[^()"]|"(?:\\.|[^"\\])*")*\)', "", idl)
        idl = re.sub(r"(\w+)::msg::(\w+)", r"\1::msg::dds_::\2_", idl)
        idl = re.sub(r"struct (\w+) \{", r"struct \1_ {", idl)
        idl = re.sub(r'#include "(\w+)/msg/(\w+)\.idl"', r'#include "\1_\2.idl"', idl)
        # Inline array typedefs, which collide once two included files declare the same one
        for kind, name, size in re.findall(r"typedef (\S+) (\w+)(\[\d+\]);", idl):
            idl = idl.replace(f"typedef {kind} {name}{size};", "")
            idl = re.sub(rf"\b{name} (\w+);", rf"{kind} \1{size};", idl)
        # ROS 2 nodes warn about endpoints whose USER_DATA lacks this hash
        hashes = json.loads(src.with_suffix(".json").read_text())["type_hashes"]
        type_hash = next(h["hash_string"] for h in hashes if h["type_name"] == f"{package}/msg/{src.stem}")
        dds = f'module msg {{ module dds_ {{ const string {src.stem}__typehash = "{type_hash}";'
        # The file ends in closing braces, so one more closes dds_. ROS's IDL has no include guards.
        guard = f"{package}_{src.stem}_idl"
        idl = f"#ifndef {guard}\n#define {guard}\n" + idl.replace("module msg {", dds, 1) + "};\n#endif\n"
        Path(out, f"{package}_{src.name}").write_text(idl)

#!/usr/bin/python3

# By kentonm

import io
import os
import shutil
import argparse
from pathlib import Path
from subprocess import Popen, PIPE


def main() -> None:
    args = _parse_args()
    compile(args.cc, args.input, args.output)


def compile(executable: str, input: io.TextIOWrapper, output: str) -> None:
    compiler = Popen([executable, "-c", "-o", output, "-x", "c", "-"], stdin=PIPE, stdout=PIPE)

    with compiler.stdin as stdin:
        buffer = []
        def writeline(line: str, indent=0) -> None:
            indent *= "\t"
            buffer.append(f"{indent}{line}\n".encode("utf-8"))

        writeline("struct {")
        writeline("unsigned int size;", indent=1)
        writeline("unsigned char data[];", indent=1)
        writeline("}")
        writeline(f"g{Path(input.name).stem} = {{")
        writeline(f".size = {os.stat(input.name).st_size},", indent=1)
        writeline(f".data = {{", indent=1)

        while it := input.read(1):
            writeline(f"0x{ord(it):02X},", indent=2)

        writeline("},", indent=1)
        writeline("};")
        stdin.writelines(buffer)
    compiler.wait()


def _parse_args() -> argparse.Namespace:
    def exec(arg: str) -> str:
        if shutil.which(arg) is None:
            raise argparse.ArgumentTypeError(f"Could not locate executable '{arg}'")
        return arg
    
    parser = argparse.ArgumentParser(description="Data File Compiler")
    parser.add_argument("--cc", type=exec, dest="cc", help="<compiler>", required=True)
    parser.add_argument("--i", type=argparse.FileType("rb"), dest="input", help="<source_file>", required=True)
    parser.add_argument("--o", type=str, dest="output", help="<output_object>", required=True)

    return parser.parse_args()


if __name__ == "__main__":
    main()
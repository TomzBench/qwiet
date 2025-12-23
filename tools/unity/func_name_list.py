#!/usr/bin/env python3
"""Extract function names from C header files for --wrap linker trick."""

import argparse
import re


def func_names_from_header(in_file: str, out_file: str) -> None:
    with open(in_file) as f_in:
        content = f_in.read()

    with open(out_file, "w") as f_out:
        # Regex match all function names in the header file
        matches = re.findall(
            r"^\s*(?:\w+[*\s]+)+(\w+?)\s*\([\w\s,*\.\[\]]*?\)\s*;",
            content,
            re.M | re.S,
        )
        for item in matches:
            f_out.write(item + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Extract function names from header for linker wrapping"
    )
    parser.add_argument("-i", "--input", type=str, help="input header file", required=True)
    parser.add_argument("-o", "--output", type=str, help="output function list file", required=True)
    args = parser.parse_args()

    func_names_from_header(args.input, args.output)

#!/usr/bin/env python3
"""Prepare C headers for CMock mock generation."""

import argparse
import re


def header_prepare(in_file: str, out_file: str, out_wrap_file: str) -> None:
    with open(in_file) as f_in:
        content = f_in.read()

    # Remove C-style comments
    c_comments_pattern = re.compile(
        r"/\*([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+/", re.DOTALL | re.MULTILINE
    )
    content = c_comments_pattern.sub(r"", content)

    # Remove C++-style comments
    cpp_comments_pattern = re.compile(r"(?!.*\".*)\/\/.*$", re.MULTILINE)
    content = cpp_comments_pattern.sub(r"", content)

    # Remove inline syscalls (z_impl_ functions)
    static_inline_pattern = re.compile(
        r"(?:__deprecated\s+)?(?:static\s+inline\s+|inline\s+static\s+|"
        r"static\s+ALWAYS_INLINE\s+|__STATIC_INLINE\s+)"
        r"((?:\w+[*\s]+)+z_impl_\w+?\(.*?\))\n\{.+?\n\}",
        re.M | re.S,
    )
    content = static_inline_pattern.sub(r"", content)

    # Change static inline functions to normal function declarations
    static_inline_pattern = re.compile(
        r"(?:__deprecated\s+)?(?:static\s+inline\s+|inline\s+static\s+|"
        r"static\s+ALWAYS_INLINE\s+|__STATIC_INLINE\s+)"
        r"((?:\w+[*\s]+)+\w+?\(.*?\))\n\{.+?\n\}",
        re.M | re.S,
    )
    content = static_inline_pattern.sub(r"\1;", content)

    # Remove syscall includes
    syscall_pattern = re.compile(r"#include <syscalls/\w+?.h>", re.M | re.S)
    content = syscall_pattern.sub(r"", content)

    # Remove __syscall declarations
    syscall_decl_pattern = re.compile(r"__syscall\s+", re.M | re.S)
    content = syscall_decl_pattern.sub("", content)

    # Remove extern prefix from function declarations
    prefixed_func_decl_pattern = re.compile(r"extern\s+((?:\w+[*\s]+)+\w+?\(.*?\);)", re.M | re.S)
    content = prefixed_func_decl_pattern.sub(r"\1", content)

    with open(out_file, "w") as f_out:
        f_out.write(content)

    # Prepare file with functions prefixed with __wrap_ for mock generation
    func_pattern = re.compile(r"^\s*((?:\w+[*\s]+)+)(\w+?\s*\([\w\s,*\.\[\]]*?\)\s*;)", re.M)
    content2 = func_pattern.sub(r"\n\1__wrap_\2", content)

    with open(out_wrap_file, "w") as f_wrap:
        f_wrap.write(content2)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Prepare headers for CMock mock generation")
    parser.add_argument("-i", "--input", type=str, help="input header file", required=True)
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        help="stripped header file for test inclusion",
        required=True,
    )
    parser.add_argument(
        "-w",
        "--wrap",
        type=str,
        help="header with __wrap_-prefixed functions for mock generation",
        required=True,
    )
    args = parser.parse_args()

    header_prepare(args.input, args.output, args.wrap)

#!/usr/bin/env python3
# SPDX-License-Identifier: Marvell-MIT
# Copyright (c) 2024 Marvell.
#
"""
Script to automatically generate boilerplate for using DPDK cmdline library.
"""

import argparse
import shlex
import sys

PARSE_FN_PARAMS = "void *parsed_result, struct cmdline *cl, void *data"
PARSE_FN_BODY = """
    /* TODO: command action */
    RTE_SET_USED(parsed_result);
    RTE_SET_USED(cl);
    RTE_SET_USED(data);
"""
NUMERIC_TYPES = [
    "UINT8",
    "UINT16",
    "UINT32",
    "UINT64",
    "INT8",
    "INT16",
    "INT32",
    "INT64",
]


def process_command(lineno, tokens, comment):
    """Generate the structures and definitions for a single command."""
    out = []
    cfile_out = []

    if tokens[0].startswith("<"):
        raise ValueError(f"Error line {lineno + 1}: command must start with a literal string")

    name_tokens = []
    for t in tokens:
        if t.startswith("<"):
            # stop processing the name building at a variable token,
            # UNLESS the token name starts with "__"
            t_type, t_name = t[1:].split(">")
            if not t_name.startswith("__"):
                break
            t = t_name[2:]   # strip off the leading '__'
        name_tokens.append(t)
    name = "_".join(name_tokens)

    result_struct = []
    initializers = []
    token_list = []
    for t in tokens:
        if t.startswith("<"):
            t_type, t_name = t[1:].split(">")
            t_val = "NULL"
            if t_name.startswith("__"):
                t_name = t_name[2:]
        else:
            t_type = "STRING"
            t_name = t
            t_val = f'"{t}"'

        if t_type == "STRING":
            result_struct.append(f"\tcmdline_fixed_string_t {t_name};")
            initializers.append(
                f"static cmdline_parse_token_string_t cmd_{name}_{t_name}_tok =\n"
                f"\tTOKEN_STRING_INITIALIZER(struct cmd_{name}_result, {t_name}, {t_val});"
            )
        elif t_type in NUMERIC_TYPES:
            result_struct.append(f"\t{t_type.lower()}_t {t_name};")
            initializers.append(
                f"static cmdline_parse_token_num_t cmd_{name}_{t_name}_tok =\n"
                f"\tTOKEN_NUM_INITIALIZER(struct cmd_{name}_result, {t_name}, RTE_{t_type});"
            )
        elif t_type in ["IP", "IP_ADDR", "IPADDR"]:
            result_struct.append(f"\tcmdline_ipaddr_t {t_name};")
            initializers.append(
                f"static cmdline_parse_token_ipaddr_t cmd_{name}_{t_name}_tok =\n"
                f"\tTOKEN_IPADDR_INITIALIZER(struct cmd_{name}_result, {t_name});"
            )
        elif t_type in ["IPV4", "IPv4", "IPV4_ADDR"]:
            result_struct.append(f"\tcmdline_ipaddr_t {t_name};")
            initializers.append(
                f"static cmdline_parse_token_ipaddr_t cmd_{name}_{t_name}_tok =\n"
                f"\tTOKEN_IPV4_INITIALIZER(struct cmd_{name}_result, {t_name});"
            )
        elif t_type in ["IPV6", "IPv6", "IPV6_ADDR"]:
            result_struct.append(f"\tcmdline_ipaddr_t {t_name};")
            initializers.append(
                f"static cmdline_parse_token_ipaddr_t cmd_{name}_{t_name}_tok =\n"
                f"\tTOKEN_IPV6_INITIALIZER(struct cmd_{name}_result, {t_name});"
            )
        elif t_type.startswith("(") and t_type.endswith(")"):
            result_struct.append(f"\tcmdline_fixed_string_t {t_name};")
            t_val = f'"{t_type[1:-1].replace(",","#")}"'
            initializers.append(
                f"static cmdline_parse_token_string_t cmd_{name}_{t_name}_tok =\n"
                f"\tTOKEN_STRING_INITIALIZER(struct cmd_{name}_result, {t_name}, {t_val});"
            )
        else:
            raise TypeError(f"Error line {lineno + 1}: unknown token type '{t_type}'")
        token_list.append(f"cmd_{name}_{t_name}_tok")

    out.append(f'/* Auto-generated handling for command "{" ".join(tokens)}" */')
    # output function prototype
    func_sig = f"void\ncmd_{name}_parsed({PARSE_FN_PARAMS})"
    out.append(f"extern {func_sig};\n")
    # output result data structure
    out.append(f"struct cmd_{name}_result {{\n" + "\n".join(result_struct) + "\n};\n")
    # output the initializer tokens
    out.append("\n".join(initializers) + "\n")
    # output the instance structure
    inst_elems = "\n".join([f"\t\t(void *)&{t}," for t in token_list])
    out.append(
        f"""\
static cmdline_parse_inst_t cmd_{name} = {{
\t.f = cmd_{name}_parsed,
\t.data = NULL,
\t.help_str = "{comment}",
\t.tokens = {{
{inst_elems}
\t\tNULL,
\t}}
}};
"""
    )
    # output function template if C file being written
    cfile_out.append(f"{func_sig}\n{{{PARSE_FN_BODY}}}\n")

    # return the instance structure name
    return (f"cmd_{name}", out, cfile_out)


def process_commands(infile, hfile, cfile, ctxname):
    """Generate boilerplate output for a list of commands from infile."""
    instances = []

    hfile.write(
        f"""\
/* File autogenerated by {sys.argv[0]} */
#ifndef GENERATED_COMMANDS_H
#define GENERATED_COMMANDS_H
#include <rte_common.h>
#include <cmdline.h>
#include <cmdline_parse_string.h>
#include <cmdline_parse_num.h>
#include <cmdline_parse_ipaddr.h>

"""
    )

    for lineno, line in enumerate(infile.readlines()):
        tokens = shlex.split(line, comments=True)
        if not tokens:
            continue
        if "#" in line:
            comment = line.split("#", 1)[-1].strip()
        else:
            comment = ""
        cmd_inst, h_out, c_out = process_command(lineno, tokens, comment)
        hfile.write("\n".join(h_out))
        if cfile:
            cfile.write("\n".join(c_out))
        instances.append(cmd_inst)

    inst_join_str = ",\n\t&"
    hfile.write(
        f"""
static __rte_used cmdline_parse_ctx_t {ctxname}[] = {{
\t&{inst_join_str.join(instances)},
\tNULL
}};

#endif /* GENERATED_COMMANDS_H */
"""
    )


def main():
    """Application main entry point."""
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--stubs",
        action="store_true",
        help="Produce C file with empty function stubs for each command",
    )
    ap.add_argument(
        "--output-file",
        "-o",
        default="-",
        help="Output header filename [default to stdout]",
    )
    ap.add_argument(
        "--context-name",
        default="ctx",
        help="Name given to the cmdline context variable in the output header [default=ctx]",
    )
    ap.add_argument("infile", type=argparse.FileType("r"), help="File with list of commands")
    args = ap.parse_args()

    if not args.stubs:
        if args.output_file == "-":
            process_commands(args.infile, sys.stdout, None, args.context_name)
        else:
            with open(args.output_file, "w") as hfile:
                process_commands(args.infile, hfile, None, args.context_name)
    else:
        if not args.output_file.endswith(".h"):
            ap.error(
                "-o/--output-file: specify an output filename ending with .h when creating stubs"
            )

        cfilename = args.output_file[:-2] + ".c"
        with open(args.output_file, "w") as hfile:
            with open(cfilename, "w") as cfile:
                cfile.write(f'#include "{args.output_file}"\n\n')
                process_commands(args.infile, hfile, cfile, args.context_name)


if __name__ == "__main__":
    main()

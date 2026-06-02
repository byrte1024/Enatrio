#!/usr/bin/env python3
"""
Enatrio macro linter -- enforces engine-specific rules on .h and .c files.

Uses tree-sitter for AST-based analysis and text scanning for pattern rules.
Run via: uv run --with tree-sitter --with tree-sitter-c python3 tools/lint.py src/

Suppress regions with:
    #define LINTNORE
    // ... code the linter should ignore ...
    #undef LINTNORE
"""

import sys
import os
import re

import tree_sitter_c as tsc
from tree_sitter import Language, Parser

C_LANG = Language(tsc.language())
PARSER = Parser(C_LANG)


class LintError:
    def __init__(self, file, line, rule, msg):
        self.file = file
        self.line = line
        self.rule = rule
        self.msg = msg

    def __str__(self):
        # gcc-compatible format so VS Code problem matchers pick it up
        return f"{self.file}:{self.line}:1: error: [{self.rule}] {self.msg}"


def find_source_files(root):
    files = []
    for dirpath, _, filenames in os.walk(root):
        for f in filenames:
            if f.endswith(".h") or f.endswith(".c"):
                files.append(os.path.join(dirpath, f))
    return sorted(files)


def build_ignore_ranges(source):
    """Parse #define LINTNORE / #undef LINTNORE pairs into line ranges."""
    ranges = []
    start = None
    for i, line in enumerate(source.split("\n"), 1):
        stripped = line.strip()
        if stripped == "#define LINTNORE":
            start = i
        elif stripped == "#undef LINTNORE" and start is not None:
            ranges.append((start, i))
            start = None
    if start is not None:
        ranges.append((start, 999999))
    return ranges


def is_ignored(line, ignore_ranges):
    return any(s <= line <= e for s, e in ignore_ranges)


def get_line(source_bytes, byte_offset):
    return source_bytes[:byte_offset].count(ord("\n")) + 1


def walk_tree(tree):
    stack = [tree.root_node]
    while stack:
        node = stack.pop()
        yield node
        for child in node.children:
            stack.append(child)


def find_macro_calls(tree, source_bytes, name):
    results = []
    for node in walk_tree(tree):
        if node.type == "call_expression":
            fn = node.child_by_field_name("function")
            if fn and fn.text.decode() == name:
                results.append(node)
    return results


def find_text_occurrences(source, pattern):
    """Find line numbers where a regex pattern matches."""
    regex = re.compile(pattern)
    results = []
    for i, line in enumerate(source.split("\n"), 1):
        if regex.search(line):
            results.append(i)
    return results


def extract_macro_arg(node, index=0):
    """Extract the text of argument at position `index` from a call_expression."""
    args = node.child_by_field_name("arguments")
    if args and args.named_child_count > index:
        return args.named_children[index].text.decode()
    return None


# ============================================================
# Rules -- NO hardcoded skip lists. Use LINTNORE in source.
# ============================================================

def check_pragma_once(filepath, source, source_bytes, tree, ignore, errors):
    """R001: .h files must have #pragma once."""
    if not filepath.endswith(".h"):
        return
    if is_ignored(1, ignore):
        return
    if b"#pragma once" not in source_bytes:
        errors.append(LintError(filepath, 1, "R001",
            "Header file missing #pragma once"))


def check_non_ascii(filepath, source, source_bytes, tree, ignore, errors):
    """R002: No non-ASCII characters in source files."""
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        for j, ch in enumerate(line):
            if ord(ch) > 127:
                errors.append(LintError(filepath, i, "R002",
                    f"Non-ASCII character U+{ord(ch):04X} at column {j+1}"))
                return


def check_balanced_macros(filepath, source, source_bytes, tree, ignore, errors):
    """R010-R012: Balanced macro pairs via text search."""
    pairs = [
        ("R010", r"\bMESSAGE_HANDLER_BEGIN\s*\(|"
                 r"\bMESSAGE_HANDLER_BEGIN_EXTERN\s*\(|"
                 r"\bSELF_MESSAGE_HANDLER_BEGIN\s*\(|"
                 r"\bSELF_MESSAGE_HANDLER_BEGIN_EXTERN\s*\(",
         r"\bMESSAGE_HANDLER_END\s*\(",
         "MESSAGE_HANDLER_BEGIN", "MESSAGE_HANDLER_END"),
        ("R011", r"\bCAN_RECEIVE_BEGIN\s*\(",
         r"\bCAN_RECEIVE_END\s*\(",
         "CAN_RECEIVE_BEGIN", "CAN_RECEIVE_END"),
        ("R012", r"\bRECEIVE_MESSAGE_BEGIN\s*\(",
         r"\bRECEIVE_MESSAGE_END\s*\(",
         "RECEIVE_MESSAGE_BEGIN", "RECEIVE_MESSAGE_END"),
    ]

    for rule, begin_pat, end_pat, begin_name, end_name in pairs:
        begins = find_text_occurrences(source, begin_pat)
        ends = find_text_occurrences(source, end_pat)

        def is_in_define(line_num):
            line = source.split("\n")[line_num - 1].strip()
            return line.startswith("#define")

        begins = [l for l in begins if not is_ignored(l, ignore) and not is_in_define(l)]
        ends = [l for l in ends if not is_ignored(l, ignore) and not is_in_define(l)]

        if len(begins) != len(ends):
            line = begins[0] if begins else 1
            errors.append(LintError(filepath, line, rule,
                f"{begin_name}/{end_name} mismatch "
                f"(found {len(begins)} begin, {len(ends)} end)"))


def check_type_define_undef(filepath, source, source_bytes, tree, ignore, errors):
    """R020: #define TYPE must have matching #undef TYPE in same file."""
    lines = source.split("\n")
    has_define = False
    has_undef = False
    define_line = 0

    for i, line in enumerate(lines, 1):
        if is_ignored(i, ignore):
            continue
        stripped = line.strip()
        if re.match(r"^#define\s+TYPE\s+\w+", stripped):
            has_define = True
            define_line = i
        if stripped == "#undef TYPE":
            has_undef = True

    if has_define and not has_undef:
        errors.append(LintError(filepath, define_line, "R020",
            "#define TYPE without #undef TYPE -- will pollute subsequent includes"))


def check_begin_class_classdef(filepath, source, source_bytes, tree, ignore, errors):
    """R021: BEGIN_CLASS must have CLASSDEF() or CLASSDEF_INHERITS() in same file."""
    begin_lines = find_text_occurrences(source, r"\bBEGIN_CLASS\s*\(")
    classdef_lines = find_text_occurrences(source, r"\bCLASSDEF(?:_INHERITS)?\s*\(")

    begin_lines = [l for l in begin_lines if not is_ignored(l, ignore)]
    classdef_lines = [l for l in classdef_lines if not is_ignored(l, ignore)]

    if begin_lines and not classdef_lines:
        errors.append(LintError(filepath, begin_lines[0], "R021",
            "BEGIN_CLASS without CLASSDEF() -- class will not be registerable"))


def check_can_receive_route_consistency(filepath, source, source_bytes, tree, ignore, errors):
    """R030-R031: CAN_RECEIVE_MID and RECEIVE_MESSAGE_ROUTE must match."""
    can_mids = {}   # mid -> line
    route_mids = {} # mid -> line

    patterns_can = [
        (re.compile(r"\bCAN_RECEIVE_MID\s*\(\s*(\w+)\s*\)"), ""),
        (re.compile(r"\bSELF_CAN_RECEIVE_MID\s*\(\s*(\w+)\s*\)"), "SELF_"),
        (re.compile(r"\bCAN_RECEIVE_MID_EXTERN\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)"), "EXTERN"),
        (re.compile(r"\bSELF_CAN_RECEIVE_MID_EXTERN\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)"), "SELF_EXTERN"),
    ]
    patterns_route = [
        (re.compile(r"\bRECEIVE_MESSAGE_ROUTE\s*\(\s*(\w+)\s*\)"), ""),
        (re.compile(r"\bSELF_RECEIVE_MESSAGE_ROUTE\s*\(\s*(\w+)\s*\)"), "SELF_"),
        (re.compile(r"\bRECEIVE_MESSAGE_ROUTE_EXTERN\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)"), "EXTERN"),
        (re.compile(r"\bSELF_RECEIVE_MESSAGE_ROUTE_EXTERN\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)"), "SELF_EXTERN"),
    ]

    def _make_key(kind, m):
        if kind == "":
            return m.group(1)
        elif kind == "SELF_":
            return "SELF_" + m.group(1)
        elif kind == "EXTERN":
            return f"EXTERN:{m.group(1)}:{m.group(2)}"
        elif kind == "SELF_EXTERN":
            return f"EXTERN:{m.group(1)}:SELF_{m.group(2)}"

    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        if line.strip().startswith("#define"):
            continue
        for pat, kind in patterns_can:
            for m in pat.finditer(line):
                can_mids[_make_key(kind, m)] = i
        for pat, kind in patterns_route:
            for m in pat.finditer(line):
                route_mids[_make_key(kind, m)] = i

    for mid in set(can_mids) - set(route_mids):
        errors.append(LintError(filepath, can_mids[mid], "R030",
            f"CAN_RECEIVE_MID({mid}) has no matching RECEIVE_MESSAGE_ROUTE"))

    for mid in set(route_mids) - set(can_mids):
        errors.append(LintError(filepath, route_mids[mid], "R031",
            f"RECEIVE_MESSAGE_ROUTE({mid}) has no matching CAN_RECEIVE_MID"))


def check_direct_refcount_assign(filepath, source, source_bytes, tree, ignore, errors):
    """R040: Direct assignment to internal_refs or external_refs is forbidden."""
    pattern = re.compile(r"->\s*(internal_refs|external_refs)\s*[=+\-]")
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        if line.strip().startswith("#define"):
            continue
        if pattern.search(line):
            errors.append(LintError(filepath, i, "R040",
                "Direct ref count manipulation -- use ObjectContainer_*Ref_From_* / UnRef_*"))


def check_direct_values_hashmap_access(filepath, source, source_bytes, tree, ignore, errors):
    """R050: Direct UnsafeVariedHashMap access on object values bypasses ObjectValueHeader."""
    patterns = [
        (r"UnsafeVariedHashMap_S(?:Set|Get|Has|Remove)\s*\(\s*Self_Values",
         "Direct hashmap access on Self_Values -- use Self_SetValue/Self_Get instead"),
        (r"UnsafeVariedHashMap_S(?:Set|Get|Has|Remove)\s*\(.+->data->values",
         "Direct hashmap access on ->data->values -- use _Object_StoreValue/_Object_GetValueData"),
    ]

    for pat, msg in patterns:
        regex = re.compile(pat)
        for i, line in enumerate(source.split("\n"), 1):
            if is_ignored(i, ignore):
                continue
            if regex.search(line):
                errors.append(LintError(filepath, i, "R050", msg))


def check_printf_usage(filepath, source, source_bytes, tree, ignore, errors):
    """R060: printf should not be used (use LOG_*)."""
    pattern = re.compile(r"\bprintf\s*\(")
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        if line.strip().startswith("//"):
            continue
        if pattern.search(line):
            errors.append(LintError(filepath, i, "R060",
                "Use LOG_INFO/LOG_WARNING/LOG_ERROR instead of printf"))


def check_handler_without_declare_mid(filepath, source, source_bytes, tree, ignore, errors):
    """R070: MESSAGE_HANDLER_BEGIN(X) should have a DECLARE_MID(X) in the same file."""
    declared = set()
    handled = {}  # mid -> line

    pat_declare = re.compile(r"\bDECLARE_MID\s*\(\s*(\w+)\s*\)")
    pat_declare_self = re.compile(r"\bDECLARE_SELF_MID\s*\(\s*(\w+)\s*\)")
    pat_handler = re.compile(r"\bMESSAGE_HANDLER_BEGIN\s*\(\s*(\w+)\s*\)")
    pat_handler_self = re.compile(r"\bSELF_MESSAGE_HANDLER_BEGIN\s*\(\s*(\w+)\s*\)")

    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        if line.strip().startswith("#define"):
            continue
        for m in pat_declare.finditer(line):
            declared.add(m.group(1))
        for m in pat_declare_self.finditer(line):
            declared.add("SELF_" + m.group(1))
        for m in pat_handler.finditer(line):
            handled[m.group(1)] = i
        for m in pat_handler_self.finditer(line):
            handled["SELF_" + m.group(1)] = i

    for mid in set(handled) - declared:
        errors.append(LintError(filepath, handled[mid], "R070",
            f"MESSAGE_HANDLER_BEGIN({mid}) but no DECLARE_MID({mid}) in this file"))


# ============================================================
# Reference safety rules (AST-based)
# ============================================================

REF_TYPES = {"ExternalReference", "ObjectReference", "TempObjectReference"}


def _find_declarations(tree, source_bytes, ignore):
    """Find all variable declarations with ref types. Returns list of (name, type, line)."""
    decls = []
    for node in walk_tree(tree):
        if node.type == "declaration":
            line = get_line(source_bytes, node.start_byte)
            if is_ignored(line, ignore):
                continue
            text = node.text.decode()
            for rt in REF_TYPES:
                # Match patterns like "ExternalReference var = ..." or "ExternalReference var;"
                m = re.match(rf"\b{rt}\s+(\w+)\b", text)
                if m:
                    decls.append((m.group(1), rt, line, node))
                    break
    return decls


def _function_body_text(tree, source_bytes, ignore):
    """Yield (function_name, body_text, start_line, end_line) for each function definition."""
    for node in walk_tree(tree):
        if node.type == "function_definition":
            line = get_line(source_bytes, node.start_byte)
            if is_ignored(line, ignore):
                continue
            decl = node.child_by_field_name("declarator")
            name = ""
            if decl:
                # Walk to find the identifier
                stack = [decl]
                while stack:
                    n = stack.pop()
                    if n.type == "identifier":
                        name = n.text.decode()
                        break
                    for c in n.children:
                        stack.append(c)
            body = node.child_by_field_name("body")
            if body:
                end_line = get_line(source_bytes, body.end_byte)
                yield (name, body.text.decode(), line, end_line)


def check_ref_cast(filepath, source, source_bytes, tree, ignore, errors):
    """R080: Direct cast to ExternalReference or ObjectReference bypasses ref counting."""
    # Match (ExternalReference) or (ObjectReference) casts but NOT sizeof(type)
    pat = re.compile(r"(?<!sizeof)\(\s*(ExternalReference|ObjectReference)\s*\)")
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        if line.strip().startswith("#define"):
            continue
        if line.strip().startswith("//"):
            continue
        if pat.search(line):
            if "ObjectContainer_TempFrom" in line:
                continue
            if "typedef" in line:
                continue
            errors.append(LintError(filepath, i, "R080",
                "Direct cast to ExternalReference/ObjectReference -- "
                "use ObjectContainer_*Ref_From_* conversion functions"))


def check_createref_unused(filepath, source, source_bytes, tree, ignore, errors):
    """R081: Object_CreateRef result must be stored (otherwise leaks an external ref)."""
    for node in walk_tree(tree):
        if node.type == "expression_statement":
            line = get_line(source_bytes, node.start_byte)
            if is_ignored(line, ignore):
                continue
            text = node.text.decode().strip()
            if re.match(r"Object_CreateRef\s*\(", text):
                errors.append(LintError(filepath, line, "R081",
                    "Object_CreateRef result not stored -- leaks an ExternalReference"))


def check_free_on_ref(filepath, source, source_bytes, tree, ignore, errors):
    """R082: free() on a reference variable -- use Object_Destroy or UnRef instead."""
    # Collect ref variable names declared in the file
    ref_vars = set()
    for name, rtype, line, node in _find_declarations(tree, source_bytes, ignore):
        ref_vars.add(name)

    if not ref_vars:
        return

    pat = re.compile(r"\bfree\s*\(\s*(\w+)\s*\)")
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        for m in pat.finditer(line):
            if m.group(1) in ref_vars:
                errors.append(LintError(filepath, i, "R082",
                    f"free({m.group(1)}) on a reference variable -- "
                    f"use Object_Destroy or ObjectContainer_UnRef_*"))


def check_ref_overwrite(filepath, source, source_bytes, tree, ignore, errors):
    """R083: ExternalReference variable reassigned without UnRef between assignments."""
    for func_name, body, start_line, end_line in _function_body_text(tree, source_bytes, ignore):
        # Track ExternalReference variables and their state
        ext_vars = {}  # name -> line of last assignment

        lines = body.split("\n")
        for offset, line in enumerate(lines):
            abs_line = start_line + offset
            if is_ignored(abs_line, ignore):
                continue
            stripped = line.strip()
            if stripped.startswith("//"):
                continue

            # Detect ExternalReference declarations with initializer
            decl_m = re.search(r"\bExternalReference\s+(\w+)\s*=", stripped)
            if decl_m:
                ext_vars[decl_m.group(1)] = abs_line
                continue

            # Detect UnRef_External -- clears the variable
            unref_m = re.search(r"ObjectContainer_UnRef_External\s*\(\s*&\s*(\w+)\s*\)", stripped)
            if unref_m and unref_m.group(1) in ext_vars:
                del ext_vars[unref_m.group(1)]
                continue

            # Detect reassignment to a tracked variable
            assign_m = re.search(r"\b(\w+)\s*=\s*(?:Object_CreateRef|ObjectContainer_ExternalRef)", stripped)
            if assign_m and assign_m.group(1) in ext_vars:
                errors.append(LintError(filepath, abs_line, "R083",
                    f"ExternalReference '{assign_m.group(1)}' reassigned without UnRef "
                    f"(first assigned at line {ext_vars[assign_m.group(1)]})"))
                ext_vars[assign_m.group(1)] = abs_line


def check_ref_never_unrefd(filepath, source, source_bytes, tree, ignore, errors):
    """R084: ExternalReference created but never UnRef'd or returned in the same function."""
    for func_name, body, start_line, end_line in _function_body_text(tree, source_bytes, ignore):
        ext_vars = {}  # name -> declaration line

        lines = body.split("\n")
        for offset, line in enumerate(lines):
            abs_line = start_line + offset
            if is_ignored(abs_line, ignore):
                continue

            # Track ExternalReference declarations
            decl_m = re.search(r"\bExternalReference\s+(\w+)\s*=", line.strip())
            if decl_m:
                ext_vars[decl_m.group(1)] = abs_line

        # Now check which are unrefd, returned, or passed by address
        for var_name in list(ext_vars.keys()):
            # Check for UnRef_External(&var)
            if re.search(rf"ObjectContainer_UnRef_External\s*\(\s*&\s*{re.escape(var_name)}\b", body):
                continue
            # Check for return var
            if re.search(rf"\breturn\s+{re.escape(var_name)}\s*;", body):
                continue
            # Check for passing &var to any function (ownership transfer)
            if re.search(rf"&\s*{re.escape(var_name)}\b", body):
                continue
            # Check for storing in an array (e.g., roots[i] = var)
            if re.search(rf"\w+\s*\[.*\]\s*=\s*{re.escape(var_name)}\b", body):
                continue

            errors.append(LintError(filepath, ext_vars[var_name], "R084",
                f"ExternalReference '{var_name}' never UnRef'd or returned "
                f"in function '{func_name}'"))


def check_create_stored_as_external(filepath, source, source_bytes, tree, ignore, errors):
    """R085: Object_Create returns TempObjectReference, not ExternalReference."""
    pat = re.compile(r"\bExternalReference\s+\w+\s*=\s*Object_Create\s*\(")
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        if pat.search(line):
            errors.append(LintError(filepath, i, "R085",
                "Object_Create returns TempObjectReference -- "
                "use Object_CreateRef for ExternalReference"))


# ============================================================
# Inheritance helpers
# ============================================================

def _parse_inherits_chain(filepath, source, ignore):
    """Parse INHERITS(X) and #define TYPE in a file.
    Returns (class_name, parent_name) or (class_name, None) if no INHERITS."""
    class_name = None
    parent_name = None
    pat_type = re.compile(r"^#define\s+TYPE\s+(\w+)")
    pat_inherits = re.compile(r"\bINHERITS\s*\(\s*(\w+)\s*\)")
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        m = pat_type.match(line.strip())
        if m:
            class_name = m.group(1)
        m = pat_inherits.search(line.strip())
        if m:
            parent_name = m.group(1)
    return (class_name, parent_name)


def _find_extern_handler_bodies(source, ignore):
    """Find extern handler bodies.
    Returns list of (extern_class, handler_name, start_line, end_line, body_text)."""
    handlers = []
    lines = source.split("\n")
    pat_begin = re.compile(
        r"\b(?:MESSAGE_HANDLER_BEGIN_EXTERN|SELF_MESSAGE_HANDLER_BEGIN_EXTERN)"
        r"\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)")
    pat_end = re.compile(r"\bMESSAGE_HANDLER_END\s*\(")
    i = 0
    while i < len(lines):
        line = lines[i]
        line_num = i + 1
        if is_ignored(line_num, ignore) or line.strip().startswith("#define"):
            i += 1
            continue
        m = pat_begin.search(line)
        if m:
            extern_class = m.group(1)
            handler_name = m.group(2)
            start = line_num
            body_lines = []
            i += 1
            while i < len(lines):
                if pat_end.search(lines[i]):
                    handlers.append((extern_class, handler_name, start, i + 1, "\n".join(body_lines)))
                    break
                body_lines.append(lines[i])
                i += 1
        i += 1
    return handlers


def _find_normal_handler_bodies(source, ignore):
    """Find non-extern handler bodies.
    Returns list of (handler_name, start_line, end_line, body_text)."""
    handlers = []
    lines = source.split("\n")
    pat_begin = re.compile(
        r"\b(?:MESSAGE_HANDLER_BEGIN|SELF_MESSAGE_HANDLER_BEGIN)\s*\(\s*(\w+)\s*\)")
    pat_extern = re.compile(r"_EXTERN\s*\(")
    pat_end = re.compile(r"\bMESSAGE_HANDLER_END\s*\(")
    i = 0
    while i < len(lines):
        line = lines[i]
        line_num = i + 1
        if is_ignored(line_num, ignore) or line.strip().startswith("#define"):
            i += 1
            continue
        m = pat_begin.search(line)
        if m and not pat_extern.search(line):
            handler_name = m.group(1)
            start = line_num
            body_lines = []
            i += 1
            while i < len(lines):
                if pat_end.search(lines[i]):
                    handlers.append((handler_name, start, i + 1, "\n".join(body_lines)))
                    break
                body_lines.append(lines[i])
                i += 1
        i += 1
    return handlers


# ============================================================
# Inheritance rules (R090-R102)
# ============================================================

def check_extern_without_inherit(filepath, source, source_bytes, tree, ignore, errors):
    """R090: Extern handler without inheritance."""
    class_name, parent_name = _parse_inherits_chain(filepath, source, ignore)
    if class_name is None:
        return
    pat_extern = re.compile(
        r"\b(?:MESSAGE_HANDLER_BEGIN_EXTERN|SELF_MESSAGE_HANDLER_BEGIN_EXTERN)"
        r"\s*\(\s*(\w+)\s*,")
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        if line.strip().startswith("#define"):
            continue
        for m in pat_extern.finditer(line):
            extern_class = m.group(1)
            if extern_class == class_name:
                continue
            if parent_name is None or extern_class != parent_name:
                errors.append(LintError(filepath, i, "R090",
                    f"Extern handler for '{extern_class}' but class '{class_name}' "
                    f"does not inherit from '{extern_class}'"))


def check_missing_call_or_ignore_base(filepath, source, source_bytes, tree, ignore, errors):
    """R091: Missing CALL_BASE or IGNORE_BASE in inherited extern handler."""
    class_name, parent_name = _parse_inherits_chain(filepath, source, ignore)
    if class_name is None or parent_name is None:
        return
    for extern_class, handler_name, start, end, body in _find_extern_handler_bodies(source, ignore):
        if extern_class != parent_name:
            continue
        if "CALL_BASE()" not in body and "IGNORE_BASE()" not in body:
            errors.append(LintError(filepath, start, "R091",
                f"Extern handler for {extern_class}.{handler_name} missing "
                f"CALL_BASE() or IGNORE_BASE()"))


def check_base_in_normal_handler(filepath, source, source_bytes, tree, ignore, errors):
    """R092: CALL_BASE/IGNORE_BASE in non-extern handler."""
    for handler_name, start, end, body in _find_normal_handler_bodies(source, ignore):
        if "CALL_BASE()" in body:
            errors.append(LintError(filepath, start, "R092",
                f"CALL_BASE() in non-extern handler {handler_name} -- "
                f"own MIDs have no base to call"))
        if "IGNORE_BASE()" in body:
            errors.append(LintError(filepath, start, "R092",
                f"IGNORE_BASE() in non-extern handler {handler_name} -- "
                f"own MIDs have no base to call"))


def check_both_call_and_ignore(filepath, source, source_bytes, tree, ignore, errors):
    """R093: Both CALL_BASE and IGNORE_BASE in same handler."""
    for extern_class, handler_name, start, end, body in _find_extern_handler_bodies(source, ignore):
        if "CALL_BASE()" in body and "IGNORE_BASE()" in body:
            errors.append(LintError(filepath, start, "R093",
                f"Both CALL_BASE() and IGNORE_BASE() in handler "
                f"{extern_class}.{handler_name} -- pick one"))


def check_circular_inheritance_per_file(filepath, source, source_bytes, tree, ignore, errors):
    """R094: Self-inheritance (per-file check; cross-file cycles checked in main)."""
    class_name, parent_name = _parse_inherits_chain(filepath, source, ignore)
    if class_name and parent_name and class_name == parent_name:
        pat_inherits = re.compile(r"\bINHERITS\s*\(")
        for i, line in enumerate(source.split("\n"), 1):
            if pat_inherits.search(line):
                errors.append(LintError(filepath, i, "R094",
                    f"Circular inheritance detected: {class_name} -> ... -> {class_name}"))
                break


def check_inherits_parent_cid_visible(filepath, source, source_bytes, tree, ignore, errors):
    """R095: Skipped -- cannot reliably check parent CID visibility without preprocessing includes."""
    pass


def check_inherits_unknown_class(filepath, source, source_bytes, tree, ignore, errors):
    """R096: Skipped -- cannot reliably check for unknown classes without preprocessing includes."""
    pass


def check_multiple_inherits(filepath, source, source_bytes, tree, ignore, errors):
    """R097: Multiple INHERITS in one class."""
    pat_inherits = re.compile(r"\bINHERITS\s*\(\s*(\w+)\s*\)")
    pat_type_def = re.compile(r"^#define\s+TYPE\s+")
    pat_type_undef = re.compile(r"^#undef\s+TYPE\s*$")
    in_class = False
    inherits_count = 0
    first_line = 0
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        stripped = line.strip()
        if pat_type_def.match(stripped):
            in_class = True
            inherits_count = 0
        elif pat_type_undef.match(stripped):
            in_class = False
        elif in_class and pat_inherits.search(stripped):
            inherits_count += 1
            if inherits_count == 1:
                first_line = i
            if inherits_count > 1:
                errors.append(LintError(filepath, i, "R097",
                    f"Multiple INHERITS in one class (first at line {first_line})"))


def check_inherits_before_begin_class(filepath, source, source_bytes, tree, ignore, errors):
    """R098: INHERITS before BEGIN_CLASS."""
    pat_inherits = re.compile(r"\bINHERITS\s*\(\s*(\w+)\s*\)")
    pat_begin = re.compile(r"\bBEGIN_CLASS\s*\(")
    has_begin = False
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        stripped = line.strip()
        if stripped.startswith("#define"):
            continue
        if pat_begin.search(stripped):
            has_begin = True
        if pat_inherits.search(stripped) and not has_begin:
            errors.append(LintError(filepath, i, "R098",
                "INHERITS before BEGIN_CLASS"))


def check_inherits_after_handler(filepath, source, source_bytes, tree, ignore, errors):
    """R099: INHERITS after handler declarations."""
    pat_inherits = re.compile(r"\bINHERITS\s*\(\s*(\w+)\s*\)")
    pat_handler = re.compile(
        r"\b(?:MESSAGE_HANDLER_BEGIN|SELF_MESSAGE_HANDLER_BEGIN|"
        r"MESSAGE_HANDLER_BEGIN_EXTERN|SELF_MESSAGE_HANDLER_BEGIN_EXTERN|"
        r"CAN_RECEIVE_BEGIN)\s*\(")
    has_handler = False
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        stripped = line.strip()
        if stripped.startswith("#define"):
            continue
        if pat_handler.search(stripped):
            has_handler = True
        if pat_inherits.search(stripped) and has_handler:
            errors.append(LintError(filepath, i, "R099",
                "INHERITS after handler declarations -- "
                "place INHERITS immediately after BEGIN_CLASS"))


def check_self_inherit(filepath, source, source_bytes, tree, ignore, errors):
    """R100: Self-inheritance."""
    class_name, parent_name = _parse_inherits_chain(filepath, source, ignore)
    if class_name and parent_name and class_name == parent_name:
        pat_inherits = re.compile(r"\bINHERITS\s*\(")
        for i, line in enumerate(source.split("\n"), 1):
            if pat_inherits.search(line):
                errors.append(LintError(filepath, i, "R100",
                    f"Self-inheritance: {class_name} inherits from itself"))
                break


def check_double_call_base(filepath, source, source_bytes, tree, ignore, errors):
    """R101: CALL_BASE used multiple times in one handler."""
    for extern_class, handler_name, start, end, body in _find_extern_handler_bodies(source, ignore):
        count = body.count("CALL_BASE()")
        if count > 1:
            errors.append(LintError(filepath, start, "R101",
                f"CALL_BASE() used {count} times in handler "
                f"{extern_class}.{handler_name} -- calling base twice is a bug"))


def check_ignore_base_on_lifecycle(filepath, source, source_bytes, tree, ignore, errors):
    """R102: IGNORE_BASE on SELF_Create or SELF_Destroy."""
    for extern_class, handler_name, start, end, body in _find_extern_handler_bodies(source, ignore):
        if handler_name in ("Create", "Destroy") and "IGNORE_BASE()" in body:
            errors.append(LintError(filepath, start, "R102",
                f"IGNORE_BASE() on SELF_{handler_name} -- "
                f"skipping parent lifecycle is dangerous"))


def check_circular_inheritance_cross_file(files, errors):
    """R094: Circular inheritance across files."""
    class_parents = {}
    pat_type = re.compile(r"^#define\s+TYPE\s+(\w+)")
    pat_inherits = re.compile(r"\bINHERITS\s*\(\s*(\w+)\s*\)")
    for filepath in files:
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            source = f.read()
        ignore = build_ignore_ranges(source)
        current_type = None
        for i, line in enumerate(source.split("\n"), 1):
            if is_ignored(i, ignore):
                continue
            m = pat_type.match(line.strip())
            if m:
                current_type = m.group(1)
            m = pat_inherits.search(line.strip())
            if m and current_type:
                class_parents[current_type] = (m.group(1), filepath, i)
    for class_name in class_parents:
        visited = set()
        walk = class_name
        while walk in class_parents:
            if walk in visited:
                parent_name, filepath, line = class_parents[class_name]
                errors.append(LintError(filepath, line, "R094",
                    f"Circular inheritance detected: {class_name} -> ... -> {walk}"))
                break
            visited.add(walk)
            walk = class_parents[walk][0]


def check_direct_consumed_key(filepath, source, source_bytes, tree, ignore, errors):
    """R110: Direct use of reserved __go_consumed__ key."""
    pattern = re.compile(r'\b(?:Payload_SetValue|Payload_Set|Payload_OverwriteValue|Payload_Overwrite)\s*\([^,]*,\s*"__go_consumed__"')
    for i, line in enumerate(source.split("\n"), 1):
        if is_ignored(i, ignore):
            continue
        if line.strip().startswith("#define"):
            continue
        if pattern.search(line):
            errors.append(LintError(filepath, i, "R110",
                'Direct use of reserved "__go_consumed__" key -- '
                'use SPREAD_CONSUME() macro instead'))


# ============================================================
# Runner
# ============================================================

ALL_RULES = [
    check_pragma_once,
    check_non_ascii,
    check_balanced_macros,
    check_type_define_undef,
    check_begin_class_classdef,
    check_can_receive_route_consistency,
    check_direct_refcount_assign,
    check_direct_values_hashmap_access,
    check_printf_usage,
    check_handler_without_declare_mid,
    check_ref_cast,
    check_createref_unused,
    check_free_on_ref,
    check_ref_overwrite,
    check_ref_never_unrefd,
    check_create_stored_as_external,
    check_extern_without_inherit,
    check_missing_call_or_ignore_base,
    check_base_in_normal_handler,
    check_both_call_and_ignore,
    check_circular_inheritance_per_file,
    check_multiple_inherits,
    check_inherits_before_begin_class,
    check_inherits_after_handler,
    check_self_inherit,
    check_double_call_base,
    check_ignore_base_on_lifecycle,
    check_direct_consumed_key,
]


def lint_file(filepath):
    errors = []
    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
        source = f.read()
    source_bytes = source.encode("utf-8")

    tree = PARSER.parse(source_bytes)
    ignore = build_ignore_ranges(source)

    for rule_fn in ALL_RULES:
        rule_fn(filepath, source, source_bytes, tree, ignore, errors)

    return errors


def main():
    if len(sys.argv) < 2:
        print("Usage: lint.py <src_dir> [--warn]")
        print("  --warn: exit 0 even on errors (warnings only)")
        sys.exit(1)

    src_dir = sys.argv[1]
    warn_only = "--warn" in sys.argv

    if not os.path.isdir(src_dir):
        print(f"Error: {src_dir} is not a directory")
        sys.exit(1)

    files = find_source_files(src_dir)
    total_errors = 0
    error_files = 0

    for filepath in files:
        errors = lint_file(filepath)
        if errors:
            error_files += 1
            total_errors += len(errors)
            for e in errors:
                print(f"  {e}")

    # Cross-file checks
    cross_errors = []
    check_circular_inheritance_cross_file(files, cross_errors)
    for e in cross_errors:
        print(f"  {e}")
    total_errors += len(cross_errors)
    if cross_errors:
        error_files += 1

    if total_errors > 0:
        print(f"\nLint: {total_errors} error(s) in {error_files} file(s) "
              f"({len(files)} files checked)")
        if not warn_only:
            sys.exit(1)
    else:
        print(f"Lint: OK ({len(files)} files checked)")


if __name__ == "__main__":
    main()

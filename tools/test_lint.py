#!/usr/bin/env python3
"""
Tests for the Enatrio macro linter.

Run via: uv run --with tree-sitter --with tree-sitter-c python3 tools/test_lint.py
"""

import os
import sys

# Add tools/ to path so we can import lint
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lint

FIXTURES = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test_fixtures")

passed = 0
failed = 0


def fixture(name):
    return os.path.join(FIXTURES, name)


def expect_rule(filename, rule_id, should_trigger=True):
    """Assert that linting a fixture file does/doesn't produce a specific rule."""
    global passed, failed
    path = fixture(filename)
    errors = lint.lint_file(path)
    triggered = [e for e in errors if e.rule == rule_id]

    label = f"{filename} -> {rule_id}"
    if should_trigger and not triggered:
        print(f"  FAIL: {label} -- expected trigger, got none")
        failed += 1
    elif not should_trigger and triggered:
        print(f"  FAIL: {label} -- expected clean, got: {triggered[0].msg}")
        failed += 1
    else:
        action = "triggered" if should_trigger else "clean"
        print(f"  PASS: {label} ({action})")
        passed += 1


def expect_clean(filename):
    """Assert that linting a fixture file produces zero errors."""
    global passed, failed
    path = fixture(filename)
    errors = lint.lint_file(path)
    label = f"{filename} -> clean"
    if errors:
        print(f"  FAIL: {label} -- got {len(errors)} error(s):")
        for e in errors:
            print(f"    {e}")
        failed += 1
    else:
        print(f"  PASS: {label}")
        passed += 1


def main():
    global passed, failed
    print("=== Linter Tests ===\n")

    # R001: #pragma once
    print("--- R001: #pragma once ---")
    expect_rule("r001_no_pragma.h", "R001", should_trigger=True)
    expect_rule("r001_ok.h", "R001", should_trigger=False)

    # R002: Non-ASCII
    print("--- R002: Non-ASCII ---")
    expect_rule("r002_non_ascii.h", "R002", should_trigger=True)
    expect_rule("r001_ok.h", "R002", should_trigger=False)

    # R010: Balanced MESSAGE_HANDLER
    print("--- R010: Balanced MESSAGE_HANDLER ---")
    expect_rule("r010_unbalanced_handler.h", "R010", should_trigger=True)
    expect_rule("all_ok.h", "R010", should_trigger=False)

    # R011: Balanced CAN_RECEIVE
    print("--- R011: Balanced CAN_RECEIVE ---")
    expect_rule("r011_unbalanced_canreceive.h", "R011", should_trigger=True)
    expect_rule("all_ok.h", "R011", should_trigger=False)

    # R012: Balanced RECEIVE_MESSAGE
    print("--- R012: Balanced RECEIVE_MESSAGE ---")
    expect_rule("r012_unbalanced_receive.h", "R012", should_trigger=True)
    expect_rule("all_ok.h", "R012", should_trigger=False)

    # R020: #define TYPE / #undef TYPE
    print("--- R020: #define/#undef TYPE ---")
    expect_rule("r020_no_undef_type.h", "R020", should_trigger=True)
    expect_rule("all_ok.h", "R020", should_trigger=False)

    # R021: BEGIN_CLASS without CLASSDEF
    print("--- R021: BEGIN_CLASS without CLASSDEF ---")
    expect_rule("r021_no_classdef.h", "R021", should_trigger=True)
    expect_rule("all_ok.h", "R021", should_trigger=False)

    # R030: CAN_RECEIVE without ROUTE
    print("--- R030: CAN_RECEIVE without ROUTE ---")
    expect_rule("r030_can_without_route.h", "R030", should_trigger=True)
    expect_rule("all_ok.h", "R030", should_trigger=False)

    # R031: ROUTE without CAN_RECEIVE
    print("--- R031: ROUTE without CAN_RECEIVE ---")
    expect_rule("r031_route_without_can.h", "R031", should_trigger=True)
    expect_rule("all_ok.h", "R031", should_trigger=False)

    # R040: Direct ref count manipulation
    print("--- R040: Direct refcount ---")
    expect_rule("r040_direct_refcount.c", "R040", should_trigger=True)
    expect_rule("all_ok.h", "R040", should_trigger=False)

    # R050: Direct hashmap access on values
    print("--- R050: Direct hashmap access ---")
    expect_rule("r050_direct_hashmap.c", "R050", should_trigger=True)
    expect_rule("all_ok.h", "R050", should_trigger=False)

    # R060: printf in game classes
    # The fixture is not under /system/ or /tests/ so it's treated as game code
    print("--- R060: printf in game classes ---")
    expect_rule("r060_printf_in_class.h", "R060", should_trigger=True)
    expect_rule("all_ok.h", "R060", should_trigger=False)

    # R070: Handler without DECLARE_MID
    print("--- R070: Handler without DECLARE_MID ---")
    expect_rule("r070_handler_no_declare.h", "R070", should_trigger=True)
    expect_rule("all_ok.h", "R070", should_trigger=False)

    # R080: Direct ref cast
    print("--- R080: Direct ref cast ---")
    expect_rule("r080_ref_cast.c", "R080", should_trigger=True)
    expect_rule("all_ok.h", "R080", should_trigger=False)

    # R081: Object_CreateRef result unused
    print("--- R081: CreateRef unused ---")
    expect_rule("r081_createref_unused.c", "R081", should_trigger=True)
    expect_rule("all_ok.h", "R081", should_trigger=False)

    # R082: free() on reference variable
    print("--- R082: free() on ref ---")
    expect_rule("r082_free_ref.c", "R082", should_trigger=True)
    expect_rule("all_ok.h", "R082", should_trigger=False)

    # R083: ExternalReference overwritten without UnRef
    print("--- R083: Ref overwrite without UnRef ---")
    expect_rule("r083_ref_overwrite.c", "R083", should_trigger=True)
    expect_rule("all_ok.h", "R083", should_trigger=False)

    # R084: ExternalReference never UnRef'd
    print("--- R084: Ref never UnRef'd ---")
    expect_rule("r084_ref_never_unrefd.c", "R084", should_trigger=True)
    expect_rule("r084_ok_unrefd.c", "R084", should_trigger=False)

    # R085: Object_Create stored as ExternalReference
    print("--- R085: Object_Create as ExternalReference ---")
    expect_rule("r085_create_as_external.c", "R085", should_trigger=True)
    expect_rule("all_ok.h", "R085", should_trigger=False)

    # LINTNORE suppression
    print("--- LINTNORE suppression ---")
    # The fixture has R040 inside LINTNORE (suppressed) and outside (not suppressed)
    errors = lint.lint_file(fixture("lintnore_suppression.h"))
    r040_errors = [e for e in errors if e.rule == "R040"]
    # Should have exactly 1 error (the one outside LINTNORE on line 12)
    if len(r040_errors) == 1 and r040_errors[0].line == 12:
        print(f"  PASS: lintnore_suppression.h -> R040 only outside LINTNORE")
        passed += 1
    else:
        print(f"  FAIL: lintnore_suppression.h -> expected 1 R040 at line 12, "
              f"got {len(r040_errors)}: {[e.line for e in r040_errors]}")
        failed += 1

    # R090: Extern handler without inheritance
    print("--- R090: Extern handler without inheritance ---")
    expect_rule("r090_extern_no_inherit.h", "R090", should_trigger=True)
    expect_rule("inherit_ok.h", "R090", should_trigger=False)

    # R091: Missing CALL_BASE or IGNORE_BASE
    print("--- R091: Missing CALL_BASE/IGNORE_BASE ---")
    expect_rule("r091_missing_call_base.h", "R091", should_trigger=True)
    expect_rule("inherit_ok.h", "R091", should_trigger=False)

    # R092: CALL_BASE/IGNORE_BASE in normal handler
    print("--- R092: CALL_BASE in non-extern handler ---")
    expect_rule("r092_call_base_in_normal.h", "R092", should_trigger=True)
    expect_rule("inherit_ok.h", "R092", should_trigger=False)

    # R093: Both CALL_BASE and IGNORE_BASE
    print("--- R093: Both CALL_BASE and IGNORE_BASE ---")
    expect_rule("r093_both_call_ignore.h", "R093", should_trigger=True)
    expect_rule("inherit_ok.h", "R093", should_trigger=False)

    # R094: Circular inheritance (cross-file -- test via single-file self-inherit)
    print("--- R094: Circular inheritance ---")
    expect_rule("r094_circular_inherit.h", "R094", should_trigger=True)

    # R097: Multiple INHERITS
    print("--- R097: Multiple INHERITS ---")
    expect_rule("r097_multiple_inherits.h", "R097", should_trigger=True)

    # R098: INHERITS before BEGIN_CLASS
    print("--- R098: INHERITS before BEGIN_CLASS ---")
    expect_rule("r098_inherits_before_begin.h", "R098", should_trigger=True)

    # R099: INHERITS after handler
    print("--- R099: INHERITS after handler ---")
    expect_rule("r099_inherits_after_handler.h", "R099", should_trigger=True)

    # R100: Self-inheritance
    print("--- R100: Self-inheritance ---")
    expect_rule("r100_self_inherit.h", "R100", should_trigger=True)

    # R101: Double CALL_BASE
    print("--- R101: Double CALL_BASE ---")
    expect_rule("r101_double_call_base.h", "R101", should_trigger=True)
    expect_rule("inherit_ok.h", "R101", should_trigger=False)

    # R102: IGNORE_BASE on lifecycle
    print("--- R102: IGNORE_BASE on SELF_Create/Destroy ---")
    expect_rule("r102_ignore_base_create.h", "R102", should_trigger=True)
    expect_rule("inherit_ok.h", "R102", should_trigger=False)

    # R110: Direct use of reserved consumed key
    print("--- R110: Direct use of __go_consumed__ key ---")
    expect_rule("r110_direct_consumed_key.h", "R110", should_trigger=True)
    expect_rule("r110_ok_spread_consume.h", "R110", should_trigger=False)

    # R120: Duplicate MID local ID within a class
    print("--- R120: Duplicate MID local ID ---")
    expect_rule("r120_duplicate_mid_id.h", "R120", should_trigger=True)
    expect_rule("inherit_ok.h", "R120", should_trigger=False)

    # all_ok.h should be completely clean
    print("--- Full clean file ---")
    expect_clean("all_ok.h")

    # Summary
    print(f"\n=== Linter Tests: {passed}/{passed + failed} passed ===")
    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()

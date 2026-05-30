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

    # all_ok.h should be completely clean
    print("--- Full clean file ---")
    expect_clean("all_ok.h")

    # Summary
    print(f"\n=== Linter Tests: {passed}/{passed + failed} passed ===")
    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()

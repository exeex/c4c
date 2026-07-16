Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Delete mutable LIR type text escape hatches in small packets

# Current Packet

## Just Finished

Completed the first `plan.md` Step 2 deletion packet by removing the mutable
non-const `LirTypeRef::str()` accessor. Updated the stale-display tests that
previously assigned through `LirTypeRef::str()` to use fresh typed
`LirTypeRef` assignments where possible, or the still-existing mutable string
conversion where a test must preserve private native facts while corrupting
display text.

## Suggested Next

Continue the Step 2 inventory with the next narrow mutable type-text escape
hatch candidate, likely the remaining mutable `operator std::string&()`, after
classifying all current callsites and choosing a focused proof.

## Watchouts

- Do not restore a mutable `LirTypeRef::str()` accessor or add a test-only
  backdoor.
- Do not delete const `str()`, `runtime_text`, factories, implicit conversions,
  or equality/classification helpers in the same packet.
- Remaining uses of `static_cast<std::string&>(...)` on `LirTypeRef` are the
  intentionally exposed next mutable escape hatch, not a replacement for
  `str()`.
- Remaining `.str() =` lines in the unowned
  `tests/frontend/frontend_lir_call_type_ref_test.cpp` are `LirOperand`
  presentation mutations, not `LirTypeRef` mutations.

## Proof

Proof run:
`cmake --build build` passed.
`ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure >
test_after.log 2>&1` passed.
Affected-test subset passed:
`ctest --test-dir build -R
'^(frontend_lir_extern_decl_type_ref|frontend_lir_global_type_ref|frontend_lir_function_signature_type_ref|backend_lir_to_bir_interface)$'
--output-on-failure`.
`git diff --check` passed.
Regression guard passed:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
--before test_before.log --after test_after.log --allow-non-decreasing-passed`.

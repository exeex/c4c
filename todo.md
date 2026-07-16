Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Delete mutable LIR type text escape hatches in small packets

# Current Packet

## Just Finished

Completed the next `plan.md` Step 2 deletion packet by removing
`LirTypeRef::operator std::string&()`. Removed the remaining tests that used the
mutable conversion as a stale-display injector; those corruptions are no longer
publicly constructible after the accessor and conversion deletions. Kept the
native invalid-layout and native-authority checks that remain meaningful.

## Suggested Next

Continue the Step 2 inventory with the next narrow `LirTypeRef` compatibility
surface. Candidate categories are the remaining implicit const conversions,
textual equality/classification helpers, and deprecated runtime-text factories;
classify current callsites before selecting one deletion packet.

## Watchouts

- Do not restore a mutable `LirTypeRef::str()` accessor or add a test-only
  backdoor.
- Do not restore mutable `operator std::string&()` or replace it with another
  mutable text escape hatch.
- Do not delete const `str()`, `runtime_text`, factories, const implicit
  conversions, or equality/classification helpers in the same packet.
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
Searches showed no remaining `static_cast<std::string&>(...)` use in `src` or
`tests`, and no remaining mutable `operator std::string&()` on `LirTypeRef`;
`LirOperand` still has its own mutable operand-text conversion.
Regression guard passed:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
--before test_before.log --after test_after.log --allow-non-decreasing-passed`.

# Active Todo

Status: Active
Source Idea: ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md
Source Plan: plan.md

## Active Item

- Step 1: audit the typed type boundary and backend ownership seams
- Current slice: move the next typed-lowering pass back into
  `src/backend/lowering/lir_to_bir.cpp` and replace the remaining
  instruction-local raw `i8`/`i32`/`i64` checks that still bypass semantic
  `LirTypeRef` inspection

## Completed

- Activated `plan.md` from
  `ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md`
- Reset active execution tracking for the new plan
- Audited the current typed-type boundary and backend ownership seams:
  - `src/codegen/lir/types.hpp` still stores only text plus coarse
    `LirTypeKind`
  - `src/codegen/lir/verify.cpp` only validates emptiness and `void` exclusion
    for `LirTypeRef`
  - `src/backend/lowering/lir_to_bir.cpp` still uses
    `lower_scalar_type_text(std::string_view)` at the main scalar cast/binop/cmp
    /select/return lowering sites
  - `src/backend/lowering/lir_to_bir.cpp` still parses function-signature
    parameter text in the fallback path
  - LIR-owned backend analysis remains live in `src/backend/liveness.cpp`,
    `src/backend/regalloc.cpp`, `src/backend/generation.cpp`, and
    `src/backend/stack_layout/*.cpp`
  - target-local phi cleanup is still present in
    `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp`
- Added exact integer-width payload support to `src/codegen/lir/types.hpp`
  through typed `LirTypeRef` integer metadata plus `LirTypeRef::integer(...)`
- Taught `src/codegen/lir/verify.cpp` to reject integer typed/text drift such
  as width/text mismatches
- Switched the first straight-line scalar lowering sites in
  `src/backend/lowering/lir_to_bir.cpp` from string decoding to typed
  `LirTypeRef` inspection for cast/binop/cmp/select/ret lowering
- Added focused backend tests that cover:
  - typed integer verifier mismatch rejection
  - typed `LirTypeRef::integer(8)` lowering through the existing i8 multiply
    BIR slice
- Converted the conditional select compare/cast lowering paths in
  `src/backend/lowering/lir_to_bir.cpp` from string-based integer decoding to
  typed `LirTypeRef` inspection
- Moved the two-parameter unsigned-char select-phi fixture in
  `tests/backend/backend_bir_test_support.cpp` onto explicit typed
  `LirTypeRef::integer(...)` constructors for its cast/cmp/phi/ret type refs
- Added a focused typed `i8` select regression in
  `tests/backend/backend_bir_lowering_tests.cpp` and kept it on the verified
  BIR path with `c4c::codegen::lir::verify_module(...)`
- Captured full-suite before/after logs in `test_fail_before.log` and
  `test_fail_after.log`
- Passed the regression guard check with no new failures and no pass-count drop
  (`2841 -> 2841`)
- Replaced the remaining `lower_function_params(...)` signature-parameter
  scalar fallback from raw `i8`/`i32`/`i64` string matching to
  `LirTypeRef`-based typed lowering
- Moved the nearby one-parameter `widen_unsigned` `i8` zext BIR fixture onto
  explicit typed `LirTypeRef::integer(...)` cast construction
- Added focused typed zext lowering coverage for the one-parameter `i8`
  straight-line slice and kept it on the verified LIR path
- Re-ran `backend_bir_tests`, nearby backend route tests, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `try_lower_minimal_folded_two_arg_direct_call_module(...)` in
  `src/backend/lowering/lir_to_bir.cpp` to accept structured `LirFunction`
  helper return/param metadata when present instead of hard-requiring raw
  `"i32"` helper signature parameter text
- Replaced the folded-helper binop type check in
  `src/backend/lowering/lir_to_bir.cpp` from raw `"i32"` string comparison to
  typed `LirTypeRef` scalar inspection
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the folded
  two-argument direct-call path working when typed helper params disagree with
  stale signature param text
- Re-ran the focused direct-call backend BIR coverage plus the full suite and
  kept the regression guard passing (`2841 -> 2841`)
- Switched `parse_backend_single_identity_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the identity and
  dual-identity direct-call lowering paths working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `parse_backend_single_add_imm_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the add-immediate
  and call-crossing direct-call lowering paths working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests` plus the full suite and kept the regression guard
  passing (`2841 -> 2841`)
- Switched `parse_backend_two_param_add_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the minimal
  two-argument direct-call lowering path working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `parse_backend_single_param_slot_add_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the slot-based
  add-immediate direct-call lowering path working when the typed helper param
  disagrees with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched the remaining direct-call helper recognizer typed-operation checks
  in `src/backend/lowering/call_decode.hpp` from raw `"i32"` string matching to
  semantic `LirTypeRef` integer-width inspection for loads, stores, allocas,
  and helper add-rewrite operations
- Added focused shared-util regressions in
  `tests/backend/backend_shared_util_tests.cpp` that keep the single-add,
  slot-add, identity, and two-param helper recognizers working when helper
  operations are built from typed `LirTypeRef::integer(32)` metadata while the
  signature text stays stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched the remaining direct-call typed-operation checks in
  `src/backend/lowering/lir_to_bir.cpp` from raw `"i32"` string matching to
  semantic `LirTypeRef` integer-width inspection for direct-call local slots,
  caller stores, dual-identity subtraction, and call-crossing add chains
- Tightened the direct-call typed-operation regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_shared_util_tests.cpp` so the helper/caller
  `LirTypeRef` objects carry semantic `i32` metadata while their stored text is
  intentionally stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Completed the active helper-recognizer audit:
  - no remaining minimal direct-call helper recognizers still rely on
    signature-text parameter typing where structured helper metadata already
    exists
  - pointer payload support is not required for the next lowering slice
    because the remaining typed-lowering/direct-call seams in scope are still
    integer and signature-shape driven; pointer handling remains confined to
    declared-extern parsing and can stay deferred until the first
    pointer-backed BIR consumer is converted

## Next

- move the next typed-lowering slice back to
  `src/backend/lowering/lir_to_bir.cpp` and audit the remaining
  instruction-local raw text checks for `i8`/`i32`/`i64` that still bypass
  semantic `LirTypeRef` inspection
- keep pointer payload support deferred until a concrete pointer-backed BIR
  lowering consumer appears, then add only the narrow typed payload that
  consumer needs

## Blockers

- none recorded

## Resume Notes

- Repo lifecycle state was `no active plan`; this todo starts from activation.
- The source idea fixes migration order as:
  1. typed LIR type ownership
  2. canonical BIR phi/control-flow surface
  3. MIR-owned liveness
  4. MIR-owned regalloc and stack-layout integration
  5. delete temporary LIR-side backend shims
- Use `todo.md` as the canonical execution state even though the reusable skill
  text mentions `plan_todo.md`.
- Immediate validating targets:
  `test_lir_verify_rejects_typed_integer_text_mismatch` and
  `test_bir_lowering_accepts_typed_two_param_u8_select_ne_phi_slice` in
  `tests/backend/backend_bir_lowering_tests.cpp`.
- Latest validating targets for the direct-call helper seam:
  `test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_typed_helper_param`
  `test_bir_lowering_accepts_minimal_direct_call_add_imm_slot_lir_module_with_typed_helper_param`
  and
  `test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_typed_helper_param`
  plus
  `test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module_with_typed_helper_params`
  in `tests/backend/backend_bir_lowering_tests.cpp`.
- Latest validating targets for the typed helper-operation seam:
  `test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper`
  `test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper`
  `test_backend_shared_call_decode_accepts_typed_i32_identity_helper`
  and
  `test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper`
  in `tests/backend/backend_shared_util_tests.cpp`.
- Full-suite before/after comparison for this slice should use
  `test_fail_before.log` and `test_fail_after.log` with the regression-guard
  checker script.

# Active Todo

Status: Active
Source Idea: ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md
Source Plan: plan.md

## Active Item

- Step 1: audit the typed type boundary and backend ownership seams
- Current slice: extend the typed scalar seam beyond the first straight-line
  integer lowering helpers by covering remaining typed integer consumers in
  `src/backend/lowering/lir_to_bir.cpp` and then start moving construction
  sites onto explicit typed `LirTypeRef` helpers

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
- Captured full-suite before/after logs in `test_fail_before.log` and
  `test_fail_after.log`
- Passed the regression guard check with no new failures and no pass-count drop
  (`2841 -> 2841`)

## Next

- convert the remaining narrow integer-only `lower_scalar_type_text(...)`
  consumers in `src/backend/lowering/lir_to_bir.cpp` that are still part of
  the typed-type migration path
- start moving LIR construction sites and test scaffolds onto explicit typed
  integer constructors instead of relying on reparsing text at use sites
- decide whether pointer payload support is needed immediately for the next
  lowering slice or should stay deferred until the first pointer-backed BIR
  consumer is converted

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
  `test_bir_lowering_accepts_i8_return_mul` in
  `tests/backend/backend_bir_lowering_tests.cpp`.
- Full-suite before/after comparison for this slice should use
  `test_fail_before.log` and `test_fail_after.log` with the regression-guard
  checker script.
